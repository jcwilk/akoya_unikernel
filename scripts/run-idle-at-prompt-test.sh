#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"
KERNEL_BIN="${BUILD_DIR}/kernel.bin"
KERNEL_ELF="${BUILD_DIR}/kernel.elf"
SCRIPT_FILE="${ROOT_DIR}/scripts/fixtures/idle-at-prompt.akoya-script"

# shellcheck source=scripts/inference-preflight.sh
source "${ROOT_DIR}/scripts/inference-preflight.sh"

usage() {
    cat >&2 <<EOF
Usage: $0 [--image PATH] [--script FILE]

Build (if needed) and run the main chat kernel headlessly with the idle-at-prompt
script gate. Exits zero when the scripted multi-turn chat completes without
connection-failure lines.

Options:
  --image PATH    Boot image (default: ${KERNEL_ELF} if present, else ${KERNEL_BIN})
  --script FILE   Akoya script fixture (default: ${SCRIPT_FILE})

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
        --script)
            SCRIPT_FILE="$2"
            shift 2
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            echo "run-idle-at-prompt-test.sh: unknown argument: $1" >&2
            usage
            exit 1
            ;;
    esac
done

if [[ -z "${IMAGE_PATH}" ]]; then
    if [[ -f "${KERNEL_ELF}" ]]; then
        IMAGE_PATH="${KERNEL_ELF}"
    elif [[ -f "${KERNEL_BIN}" ]]; then
        IMAGE_PATH="${KERNEL_BIN}"
    else
        echo "run-idle-at-prompt-test.sh: kernel image not found; building..." >&2
        bash "${ROOT_DIR}/scripts/build.sh"
        if [[ -f "${KERNEL_ELF}" ]]; then
            IMAGE_PATH="${KERNEL_ELF}"
        elif [[ -f "${KERNEL_BIN}" ]]; then
            IMAGE_PATH="${KERNEL_BIN}"
        else
            echo "run-idle-at-prompt-test.sh: build did not produce kernel image" >&2
            exit 1
        fi
    fi
fi

if [[ ! -f "${IMAGE_PATH}" ]]; then
    echo "run-idle-at-prompt-test.sh: image not found: ${IMAGE_PATH}" >&2
    exit 1
fi

if [[ ! -f "${SCRIPT_FILE}" ]]; then
    echo "run-idle-at-prompt-test.sh: script not found: ${SCRIPT_FILE}" >&2
    exit 1
fi

if ! inference_preflight "run-idle-at-prompt-test.sh"; then
    exit 1
fi

LOG_FILE="${BUILD_DIR}/idle-at-prompt-test.log"
: > "${LOG_FILE}"

set +e
bash "${ROOT_DIR}/scripts/run-qemu.sh" --headless --image "${IMAGE_PATH}" --script "${SCRIPT_FILE}" 2>&1 | tee "${LOG_FILE}"
run_status=${PIPESTATUS[0]}
set -e

if grep -q '^chat failed:' "${LOG_FILE}"; then
    echo "run-idle-at-prompt-test.sh: connection failure line detected (see ${LOG_FILE})" >&2
    exit 1
fi

if grep -Fq "chat_session=exit" "${LOG_FILE}"; then
    reply_lines="$(awk '
        /^chat failed:/ { next }
        /^> / { next }
        /^chat_session=/ { next }
        /^net_/ { next }
        /^run-qemu\.sh:/ { next }
        /^Running QEMU/ { next }
        /^Boot image:/ { next }
        /^LAN:/ { next }
        /^chat-script-harness:/ { next }
        /^idle-at-prompt/ { next }
        /^$/ { next }
        { count++ }
        END { print count + 0 }
    ' "${LOG_FILE}")"
    if [[ "${reply_lines}" -lt 2 ]]; then
        echo "run-idle-at-prompt-test.sh: expected at least 2 assistant reply lines, got ${reply_lines}" >&2
        exit 1
    fi
    echo "run-idle-at-prompt-test.sh: idle-at-prompt gate passed" | tee -a "${LOG_FILE}"
    exit 0
fi

if [[ ${run_status} -ne 0 ]]; then
    echo "run-idle-at-prompt-test.sh: run-qemu.sh exited ${run_status}" >&2
    exit "${run_status}"
fi

echo "run-idle-at-prompt-test.sh: idle-at-prompt gate did not complete (see ${LOG_FILE})" >&2
exit 1
