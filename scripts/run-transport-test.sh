#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"
TRANSPORT_BIN="${BUILD_DIR}/transport-test.bin"
TRANSPORT_ELF="${BUILD_DIR}/transport-test.elf"

# shellcheck source=scripts/inference-preflight.sh
source "${ROOT_DIR}/scripts/inference-preflight.sh"

usage() {
    cat >&2 <<EOF
Usage: $0 [--image PATH]

Build (if needed) and run the transport-test boot image headlessly on the workstation LAN.
Exits zero when captured output contains 'transport-test: ALL PASS'.

Options:
  --image PATH  Boot image (default: ${TRANSPORT_BIN} if present, else ${TRANSPORT_ELF})

Environment: same as scripts/run-qemu.sh (AKOYA_CHAT_HOST_IP, AKOYA_CHAT_PORT, etc.)
EOF
}

IMAGE_PATH=""

while [[ $# -gt 0 ]]; do
    case "$1" in
        --image)
            IMAGE_PATH="$2"
            shift 2
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            echo "run-transport-test.sh: unknown argument: $1" >&2
            usage
            exit 1
            ;;
    esac
done

if [[ -z "${IMAGE_PATH}" ]]; then
    if [[ -f "${TRANSPORT_ELF}" ]]; then
        IMAGE_PATH="${TRANSPORT_ELF}"
    elif [[ -f "${TRANSPORT_BIN}" ]]; then
        IMAGE_PATH="${TRANSPORT_BIN}"
    else
        echo "run-transport-test.sh: transport-test image not found; building..." >&2
        bash "${ROOT_DIR}/scripts/build.sh"
        if [[ -f "${TRANSPORT_ELF}" ]]; then
            IMAGE_PATH="${TRANSPORT_ELF}"
        elif [[ -f "${TRANSPORT_BIN}" ]]; then
            IMAGE_PATH="${TRANSPORT_BIN}"
        else
            echo "run-transport-test.sh: build did not produce transport-test image" >&2
            exit 1
        fi
    fi
fi

if [[ ! -f "${IMAGE_PATH}" ]]; then
    echo "run-transport-test.sh: image not found: ${IMAGE_PATH}" >&2
    exit 1
fi

if ! inference_preflight "run-transport-test.sh"; then
    exit 1
fi

LOG_FILE="${BUILD_DIR}/transport-test.log"
: > "${LOG_FILE}"

set +e
bash "${ROOT_DIR}/scripts/run-qemu.sh" --headless --image "${IMAGE_PATH}" 2>&1 | tee "${LOG_FILE}"
run_status=${PIPESTATUS[0]}
set -e

if grep -Fq "transport-test: ALL PASS" "${LOG_FILE}"; then
    echo "run-transport-test.sh: transport test passed" | tee -a "${LOG_FILE}"
    exit 0
fi

if grep -Fq "transport-test: FAILED" "${LOG_FILE}"; then
    echo "run-transport-test.sh: transport test failed (see ${LOG_FILE})" >&2
    exit 1
fi

if [[ ${run_status} -ne 0 ]]; then
    echo "run-transport-test.sh: run-qemu.sh exited ${run_status}" >&2
    exit "${run_status}"
fi

echo "run-transport-test.sh: aggregate pass line not found in ${LOG_FILE}" >&2
exit 1
