#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"
CHAT_REGRESSION_BIN="${BUILD_DIR}/chat-regression-test.bin"
CHAT_REGRESSION_ELF="${BUILD_DIR}/chat-regression-test.elf"

# shellcheck source=scripts/inference-preflight.sh
source "${ROOT_DIR}/scripts/inference-preflight.sh"

usage() {
    cat >&2 <<EOF
Usage: $0 [--image PATH]

Build (if needed) and run the timed-gap chat regression boot image headlessly on the
workstation LAN. Exits zero when captured output contains 'timed-gap-chat-regression: ALL PASS'.

Options:
  --image PATH  Boot image (default: ${CHAT_REGRESSION_BIN} if present, else ${CHAT_REGRESSION_ELF})

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
            echo "run-chat-regression-test.sh: unknown argument: $1" >&2
            usage
            exit 1
            ;;
    esac
done

if [[ -z "${IMAGE_PATH}" ]]; then
    if [[ -f "${CHAT_REGRESSION_ELF}" ]]; then
        IMAGE_PATH="${CHAT_REGRESSION_ELF}"
    elif [[ -f "${CHAT_REGRESSION_BIN}" ]]; then
        IMAGE_PATH="${CHAT_REGRESSION_BIN}"
    else
        echo "run-chat-regression-test.sh: chat-regression-test image not found; building..." >&2
        bash "${ROOT_DIR}/scripts/build.sh"
        if [[ -f "${CHAT_REGRESSION_ELF}" ]]; then
            IMAGE_PATH="${CHAT_REGRESSION_ELF}"
        elif [[ -f "${CHAT_REGRESSION_BIN}" ]]; then
            IMAGE_PATH="${CHAT_REGRESSION_BIN}"
        else
            echo "run-chat-regression-test.sh: build did not produce chat-regression-test image" >&2
            exit 1
        fi
    fi
fi

if [[ ! -f "${IMAGE_PATH}" ]]; then
    echo "run-chat-regression-test.sh: image not found: ${IMAGE_PATH}" >&2
    exit 1
fi

if ! inference_preflight "run-chat-regression-test.sh"; then
    exit 1
fi

LOG_FILE="${BUILD_DIR}/chat-regression-test.log"
: > "${LOG_FILE}"

set +e
bash "${ROOT_DIR}/scripts/run-qemu.sh" --headless --logical chat-regression-test 2>&1 | tee "${LOG_FILE}"
run_status=${PIPESTATUS[0]}
set -e

if grep -Fq "timed-gap-chat-regression: ALL PASS" "${LOG_FILE}"; then
    if grep -q '^chat failed:' "${LOG_FILE}"; then
        echo "run-chat-regression-test.sh: unexpected chat failed line in passing run (see ${LOG_FILE})" >&2
        exit 1
    fi
    local_turns="$(grep -c ': PASS$' "${LOG_FILE}" || true)"
    if [[ "${local_turns}" -lt 3 ]]; then
        echo "run-chat-regression-test.sh: expected at least 3 turn PASS lines, got ${local_turns}" >&2
        exit 1
    fi
    echo "run-chat-regression-test.sh: timed-gap chat regression passed" | tee -a "${LOG_FILE}"
    exit 0
fi

if grep -Fq "timed-gap-chat-regression: FAILED" "${LOG_FILE}"; then
    echo "run-chat-regression-test.sh: timed-gap chat regression failed (see ${LOG_FILE})" >&2
    exit 1
fi

if [[ ${run_status} -ne 0 ]]; then
    echo "run-chat-regression-test.sh: run-qemu.sh exited ${run_status}" >&2
    exit "${run_status}"
fi

echo "run-chat-regression-test.sh: aggregate pass line not found in ${LOG_FILE}" >&2
exit 1
