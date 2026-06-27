#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"
LOG_FILE="${BUILD_DIR}/qemu-smoke.log"

QEMU_BIN="${AKOYA_QEMU_BIN:-qemu-system-i386}"
TIMEOUT_SEC="${AKOYA_QEMU_TIMEOUT_SEC:-30}"
BOOT_MESSAGE="${AKOYA_BOOTSTRAP_MESSAGE:-akoya_unikernel bootstrap ok}"

MODE=""
IMAGE_PATH=""
# Empty = use mode default; 1 = exit when guest triggers shutdown; 0 = hold QEMU open after guest halts
EXIT_ON_GUEST_DONE=""

usage() {
    cat >&2 <<EOF
Usage: $0 (--headful|--headless) [--image PATH] [--exit-on-guest-done | --hold]

  --headful     Interactive SDL display window
  --headless    No display; smoke-test timeout and bootstrap message assertion
  --image PATH  Boot image to run (default: auto-select when exactly one logical image exists)

  --exit-on-guest-done  QEMU exits when the guest finishes (default for --headless)
  --hold                Keep QEMU open after the guest halts; close the window or Ctrl+C (default for --headful)

Display mode is mandatory: specify exactly one of --headful or --headless.
When --image is omitted, runnable images (*.elf, *.bin) in the build output directory
are grouped by stem (e.g. kernel.elf + kernel.bin count as one logical "kernel" image).

Requires: qemu-system-i386 (qemu-system-x86 package on Debian/Ubuntu).
EOF
}

fail() {
    echo "run-qemu.sh: $*" >&2
    exit 1
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --headful)
            if [[ -n "${MODE}" ]]; then
                fail "specify exactly one display mode (--headful or --headless), not both"
            fi
            MODE="headful"
            shift
            ;;
        --headless)
            if [[ -n "${MODE}" ]]; then
                fail "specify exactly one display mode (--headful or --headless), not both"
            fi
            MODE="headless"
            shift
            ;;
        --image)
            IMAGE_PATH="$2"
            shift 2
            ;;
        --exit-on-guest-done)
            if [[ "${EXIT_ON_GUEST_DONE}" == "0" ]]; then
                fail "specify only one of --exit-on-guest-done or --hold"
            fi
            EXIT_ON_GUEST_DONE=1
            shift
            ;;
        --hold)
            if [[ "${EXIT_ON_GUEST_DONE}" == "1" ]]; then
                fail "specify only one of --exit-on-guest-done or --hold"
            fi
            EXIT_ON_GUEST_DONE=0
            shift
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            fail "unknown argument: $1 (see --help)"
            ;;
    esac
done

if [[ -z "${MODE}" ]]; then
    fail "display mode is required: specify --headful or --headless"
fi

if [[ -z "${EXIT_ON_GUEST_DONE}" ]]; then
    if [[ "${MODE}" == "headless" ]]; then
        EXIT_ON_GUEST_DONE=1
    else
        EXIT_ON_GUEST_DONE=0
    fi
fi

if ! command -v "${QEMU_BIN}" >/dev/null 2>&1; then
    fail "${QEMU_BIN} not found; install qemu-system-x86 (e.g. sudo apt install qemu-system-x86)"
fi

# Collect stems of runnable images (*.elf, *.bin) directly under BUILD_DIR.
discover_logical_stems() {
    local stems=()
    local f stem
    shopt -s nullglob
    for f in "${BUILD_DIR}"/*.elf "${BUILD_DIR}"/*.bin; do
        [[ -f "${f}" ]] || continue
        stem="$(basename "${f}")"
        stem="${stem%.elf}"
        stem="${stem%.bin}"
        stems+=("${stem}")
    done
    shopt -u nullglob

    local unique=() seen
    for stem in "${stems[@]}"; do
        seen=0
        for u in "${unique[@]:-}"; do
            if [[ "${u}" == "${stem}" ]]; then
                seen=1
                break
            fi
        done
        if [[ "${seen}" -eq 0 ]]; then
            unique+=("${stem}")
        fi
    done
    if [[ ${#unique[@]} -gt 0 ]]; then
        printf '%s\n' "${unique[@]}"
    fi
}

resolve_image_for_stem() {
    local stem="$1"
    local elf="${BUILD_DIR}/${stem}.elf"
    local bin="${BUILD_DIR}/${stem}.bin"
    if [[ -f "${elf}" ]]; then
        echo "${elf}"
    elif [[ -f "${bin}" ]]; then
        echo "${bin}"
    else
        return 1
    fi
}

resolve_kernel_image() {
    if [[ -n "${IMAGE_PATH}" ]]; then
        if [[ ! -f "${IMAGE_PATH}" ]]; then
            fail "image not found: ${IMAGE_PATH}"
        fi
        echo "${IMAGE_PATH}"
        return
    fi

    mapfile -t stems < <(discover_logical_stems)

    if [[ ${#stems[@]} -eq 0 ]]; then
        fail "no runnable image found in ${BUILD_DIR}; run 'make build' first"
    fi

    if [[ ${#stems[@]} -gt 1 ]]; then
        local stem_list
        stem_list="$(printf '%s, ' "${stems[@]}")"
        stem_list="${stem_list%, }"
        fail "multiple logical images in ${BUILD_DIR} (${stem_list}); specify one with --image PATH"
    fi

    resolve_image_for_stem "${stems[0]}"
}

main() {
    mkdir -p "${BUILD_DIR}"

    local kernel_image
    kernel_image="$(resolve_kernel_image)"

    : > "${LOG_FILE}"

    local display_args=(-display none)
    local run_timeout=("${TIMEOUT_SEC}")
    if [[ "${MODE}" == "headful" ]]; then
        display_args=(-display sdl)
        if [[ "${EXIT_ON_GUEST_DONE}" -eq 0 ]]; then
            echo "Running QEMU headfully (guest will halt; close the window or Ctrl+C when done)" | tee -a "${LOG_FILE}"
        else
            echo "Running QEMU headfully" | tee -a "${LOG_FILE}"
        fi
        run_timeout=()
    else
        if [[ "${EXIT_ON_GUEST_DONE}" -eq 0 ]]; then
            echo "Running QEMU headless with --hold (no auto-exit on guest shutdown)" | tee -a "${LOG_FILE}"
        else
            echo "Running QEMU headless smoke test (timeout ${TIMEOUT_SEC}s)" | tee -a "${LOG_FILE}"
        fi
    fi

    echo "Boot image: ${kernel_image}" | tee -a "${LOG_FILE}"

    local -a qemu_args=(
        -m 512
        -cpu pentium3
        -smp 1
        "${display_args[@]}"
        -serial stdio
        -no-reboot
    )
    if [[ "${EXIT_ON_GUEST_DONE}" -eq 1 ]]; then
        qemu_args+=(-device isa-debug-exit,iobase=0xf4,iosize=0x04)
    fi
    qemu_args+=(-kernel "${kernel_image}")

    set +e
    if [[ ${#run_timeout[@]} -gt 0 ]]; then
        timeout "${run_timeout[@]}" "${QEMU_BIN}" "${qemu_args[@]}" 2>&1 | tee -a "${LOG_FILE}"
    else
        "${QEMU_BIN}" "${qemu_args[@]}" 2>&1 | tee -a "${LOG_FILE}"
    fi
    local qemu_status=${PIPESTATUS[0]}
    set -e

    if [[ "${MODE}" == "headful" ]]; then
        echo "QEMU headful session ended (exit ${qemu_status})" | tee -a "${LOG_FILE}"
        exit "${qemu_status}"
    fi

    if [[ ${qemu_status} -eq 124 ]] && ! grep -Fq "${BOOT_MESSAGE}" "${LOG_FILE}"; then
        fail "timed out waiting for bootstrap output"
    fi

    if [[ ${qemu_status} -ne 0 ]] && ! grep -Fq "${BOOT_MESSAGE}" "${LOG_FILE}"; then
        fail "qemu exited with status ${qemu_status}"
    fi

    if ! grep -Fq "${BOOT_MESSAGE}" "${LOG_FILE}"; then
        fail "expected bootstrap message not found in captured output"
    fi

    echo "QEMU smoke test passed" | tee -a "${LOG_FILE}"
}

main "$@"
