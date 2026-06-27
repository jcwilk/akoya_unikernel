#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"
LOG_FILE="${BUILD_DIR}/qemu-smoke.log"

QEMU_BIN="${AKOYA_QEMU_BIN:-qemu-system-i386}"
TIMEOUT_SEC="${AKOYA_QEMU_TIMEOUT_SEC:-30}"
DOCKER_TIMEOUT_SEC="${AKOYA_QEMU_DOCKER_TIMEOUT_SEC:-180}"
BOOT_MESSAGE="${AKOYA_BOOTSTRAP_MESSAGE:-akoya_unikernel bootstrap ok}"
DOCKER_IMAGE="${AKOYA_QEMU_DOCKER_IMAGE:-ubuntu:24.04}"

MODE=""
HEADFUL_VNC=0
IMAGE_PATH=""

usage() {
    cat >&2 <<EOF
Usage: $0 (--headful|--headless) [--image PATH] [--vnc]

  --headful     Interactive display (SDL/GTK window, or --vnc for remote viewer)
  --headless    No display; smoke-test timeout and bootstrap message assertion
  --image PATH  Boot image to run (default: auto-select when exactly one logical image exists)
  --vnc         Headful via VNC on localhost:5900 (useful with Docker / headless workstations)

Display mode is mandatory: specify exactly one of --headful or --headless.
When --image is omitted, runnable images (*.elf, *.bin) in the build output directory
are grouped by stem (e.g. kernel.elf + kernel.bin count as one logical "kernel" image).
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
        --vnc)
            HEADFUL_VNC=1
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

if [[ "${HEADFUL_VNC}" -eq 1 && "${MODE}" != "headful" ]]; then
    fail "--vnc is only valid with --headful"
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

    # Deduplicate stems while preserving order.
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

qemu_available() {
    command -v "${QEMU_BIN}" >/dev/null 2>&1
}

main() {
    mkdir -p "${BUILD_DIR}"

    local kernel_image
    kernel_image="$(resolve_kernel_image)"

    : > "${LOG_FILE}"

    local display_args=(-display none)
    local run_timeout=("${TIMEOUT_SEC}")
    if [[ "${MODE}" == "headful" ]]; then
        if [[ "${HEADFUL_VNC}" -eq 1 ]]; then
            display_args=(-display "vnc=127.0.0.1:0")
            echo "Running QEMU headfully with VNC at vnc://127.0.0.1:5900 (close QEMU or Ctrl+C to exit)" | tee -a "${LOG_FILE}"
        else
            display_args=(-display sdl)
            echo "Running QEMU headfully (close the window to exit)" | tee -a "${LOG_FILE}"
        fi
        run_timeout=()
    else
        echo "Running QEMU headless smoke test (timeout ${TIMEOUT_SEC}s)" | tee -a "${LOG_FILE}"
    fi

    echo "Boot image: ${kernel_image}" | tee -a "${LOG_FILE}"

    local -a qemu_args=(
        -m 512
        -cpu pentium3
        -smp 1
        "${display_args[@]}"
        -serial stdio
        -no-reboot
        -device isa-debug-exit,iobase=0xf4,iosize=0x04
        -kernel "${kernel_image}"
    )

    local kernel_relpath="${kernel_image#${ROOT_DIR}/}"
    local -a docker_display=()
    if [[ "${MODE}" == "headful" && -n "${DISPLAY:-}" ]]; then
        docker_display=(-e "DISPLAY=${DISPLAY}" -v /tmp/.X11-unix:/tmp/.X11-unix)
    fi
    local -a docker_publish=()
    if [[ "${HEADFUL_VNC}" -eq 1 ]]; then
        docker_publish=(-p 5900:5900)
    fi
    local -a docker_cmd=(
        docker run --rm -i
        "${docker_display[@]}"
        "${docker_publish[@]}"
        -e QEMU_X11_NO_MITSHM=1
        -e SDL_VIDEODRIVER=x11
        -v "${ROOT_DIR}:/work"
        -w /work
        "${DOCKER_IMAGE}"
        bash -lc "apt-get update -qq && DEBIAN_FRONTEND=noninteractive apt-get install -y -qq qemu-system-x86 libsdl2-2.0-0 >/dev/null && qemu-system-i386 -m 512 -cpu pentium3 -smp 1 ${display_args[*]} -serial stdio -no-reboot -device isa-debug-exit,iobase=0xf4,iosize=0x04 -kernel /work/${kernel_relpath}"
    )

    set +e
    if qemu_available; then
        if [[ ${#run_timeout[@]} -gt 0 ]]; then
            timeout "${run_timeout[@]}" "${QEMU_BIN}" "${qemu_args[@]}" 2>&1 | tee -a "${LOG_FILE}"
        else
            "${QEMU_BIN}" "${qemu_args[@]}" 2>&1 | tee -a "${LOG_FILE}"
        fi
    elif command -v docker >/dev/null 2>&1; then
        echo "Native ${QEMU_BIN} not found; using Docker image ${DOCKER_IMAGE}" | tee -a "${LOG_FILE}"
        if [[ ${#run_timeout[@]} -gt 0 ]]; then
            timeout "${DOCKER_TIMEOUT_SEC}" "${docker_cmd[@]}" 2>&1 | tee -a "${LOG_FILE}"
        else
            "${docker_cmd[@]}" 2>&1 | tee -a "${LOG_FILE}"
        fi
    else
        fail "qemu-system-i386 not found and docker unavailable"
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
