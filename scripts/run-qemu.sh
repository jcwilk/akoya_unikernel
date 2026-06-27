#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"
KERNEL_BIN="${BUILD_DIR}/kernel.bin"
KERNEL_ELF="${BUILD_DIR}/kernel.elf"
LOG_FILE="${BUILD_DIR}/qemu-smoke.log"

QEMU_BIN="${AKOYA_QEMU_BIN:-qemu-system-i386}"
TIMEOUT_SEC="${AKOYA_QEMU_TIMEOUT_SEC:-30}"
DOCKER_TIMEOUT_SEC="${AKOYA_QEMU_DOCKER_TIMEOUT_SEC:-180}"
BOOT_MESSAGE="${AKOYA_BOOTSTRAP_MESSAGE:-akoya_unikernel bootstrap ok}"
DOCKER_IMAGE="${AKOYA_QEMU_DOCKER_IMAGE:-ubuntu:24.04}"

usage() {
    echo "Usage: $0 [--kernel PATH]" >&2
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --kernel)
            KERNEL_BIN="$2"
            KERNEL_ELF="${2%.bin}.elf"
            if [[ "${2}" == *.elf ]]; then
                KERNEL_ELF="$2"
                KERNEL_BIN="${2%.elf}.bin"
            fi
            shift 2
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            echo "Unknown argument: $1" >&2
            usage
            exit 1
            ;;
    esac
done

fail() {
    echo "QEMU smoke test failed: $*" >&2
    exit 1
}

qemu_available() {
    command -v "${QEMU_BIN}" >/dev/null 2>&1
}

main() {
    mkdir -p "${BUILD_DIR}"

    if [[ ! -f "${KERNEL_BIN}" && ! -f "${KERNEL_ELF}" ]]; then
        fail "missing kernel artifact (${KERNEL_ELF} or ${KERNEL_BIN}); run make build first"
    fi

    : > "${LOG_FILE}"

    echo "Running QEMU smoke test (timeout ${TIMEOUT_SEC}s)" | tee -a "${LOG_FILE}"

    local kernel_image="${KERNEL_ELF}"
    if [[ ! -f "${kernel_image}" ]]; then
        kernel_image="${KERNEL_BIN}"
    fi

    set +e
    if qemu_available; then
        timeout "${TIMEOUT_SEC}" \
            "${QEMU_BIN}" \
                -m 512 \
                -cpu pentium3 \
                -smp 1 \
                -display none \
                -serial stdio \
                -no-reboot \
                -device isa-debug-exit,iobase=0xf4,iosize=0x04 \
                -kernel "${kernel_image}" 2>&1 | tee -a "${LOG_FILE}"
    elif command -v docker >/dev/null 2>&1; then
        echo "Native ${QEMU_BIN} not found; using Docker image ${DOCKER_IMAGE}" | tee -a "${LOG_FILE}"
        local kernel_relpath="${kernel_image#${ROOT_DIR}/}"
        timeout "${DOCKER_TIMEOUT_SEC}" docker run --rm -i \
            -v "${ROOT_DIR}:/work" \
            -w /work \
            "${DOCKER_IMAGE}" \
            bash -lc "apt-get update -qq && DEBIAN_FRONTEND=noninteractive apt-get install -y -qq qemu-system-x86 >/dev/null && qemu-system-i386 -m 512 -cpu pentium3 -smp 1 -display none -serial stdio -no-reboot -device isa-debug-exit,iobase=0xf4,iosize=0x04 -kernel /work/${kernel_relpath}" 2>&1 | tee -a "${LOG_FILE}"
    else
        fail "qemu-system-i386 not found and docker unavailable"
    fi
    local qemu_status=${PIPESTATUS[0]}
    set -e

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
