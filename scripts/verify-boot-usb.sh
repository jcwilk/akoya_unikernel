#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"
IMG_PATH="${BUILD_DIR}/akoya-boot.img"
LOG_FILE="${BUILD_DIR}/verify-boot-usb.log"
BOOT_MESSAGE="${AKOYA_BOOTSTRAP_MESSAGE:-akoya_unikernel bootstrap ok}"
CHAT_HOST="${AKOYA_CHAT_HOST_IP:-192.168.1.110}"

usage() {
    cat >&2 <<EOF
Usage: $0

Ensure build/akoya-boot.img exists (runs build-boot-usb.sh if needed),
then boot it under QEMU as a legacy hard disk (bootstrap + connectivity probe).
EOF
}

if [[ "${1:-}" == "-h" || "${1:-}" == "--help" ]]; then
    usage
    exit 0
fi

if [[ $# -gt 0 ]]; then
    echo "verify-boot-usb.sh: unknown argument: $1" >&2
    exit 1
fi

if [[ ! -f "${IMG_PATH}" ]]; then
    echo "verify-boot-usb.sh: disk image not found; building..." >&2
    bash "${ROOT_DIR}/scripts/build-boot-usb.sh" || exit 1
fi

if [[ ! -f "${IMG_PATH}" ]]; then
    echo "verify-boot-usb.sh: disk image still missing: ${IMG_PATH}" >&2
    exit 1
fi

: > "${LOG_FILE}"
echo "verify-boot-usb.sh: booting disk image ${IMG_PATH}" | tee -a "${LOG_FILE}"

set +e
bash "${ROOT_DIR}/scripts/run-qemu.sh" --headless --boot-disk "${IMG_PATH}" 2>&1 | tee -a "${LOG_FILE}"
run_status=${PIPESTATUS[0]}
set -e

if ! grep -Fq "${BOOT_MESSAGE}" "${LOG_FILE}"; then
    echo "verify-boot-usb.sh: bootstrap message not found (exit ${run_status})" >&2
    exit 1
fi

if grep -Fq "${CHAT_HOST} reachable" "${LOG_FILE}" \
    || grep -Eq 'net_ping=.*status=ok' "${LOG_FILE}" \
    || grep -Fq " reachable" "${LOG_FILE}"; then
    echo "verify-boot-usb.sh: USB disk image verification passed" | tee -a "${LOG_FILE}"
    exit 0
fi

echo "verify-boot-usb.sh: connectivity probe success not found (exit ${run_status})" >&2
exit 1
