#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"
ISO_PATH="${BUILD_DIR}/akoya-boot.iso"
LOG_FILE="${BUILD_DIR}/verify-boot-iso.log"
BOOT_MESSAGE="${AKOYA_BOOTSTRAP_MESSAGE:-akoya_unikernel bootstrap ok}"
CHAT_HOST="${AKOYA_CHAT_HOST_IP:-192.168.1.2}"
PROBE_LABEL="${AKOYA_PROBE_HOST:-google.com}"

usage() {
    cat >&2 <<EOF
Usage: $0

Ensure build/akoya-boot.iso exists (packages if needed), then run a headless
QEMU boot-from-ISO smoke test without inference-endpoint pre-flight.

Pass requires:
  - bootstrap diagnostic message (${BOOT_MESSAGE})
  - successful outbound connectivity probe in captured serial output

Environment: same LAN/macvtap variables as scripts/run-qemu.sh.
EOF
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        -h|--help)
            usage
            exit 0
            ;;
        *)
            echo "verify-boot-iso.sh: unknown argument: $1" >&2
            usage
            exit 1
            ;;
    esac
done

if [[ ! -f "${ISO_PATH}" ]]; then
    echo "verify-boot-iso.sh: ISO not found; packaging..." >&2
    if ! bash "${ROOT_DIR}/scripts/build-boot-iso.sh"; then
        echo "verify-boot-iso.sh: ISO packaging failed" >&2
        exit 1
    fi
fi

if [[ ! -f "${ISO_PATH}" ]]; then
    echo "verify-boot-iso.sh: ISO still missing after packaging: ${ISO_PATH}" >&2
    exit 1
fi

mkdir -p "${BUILD_DIR}"
: > "${LOG_FILE}"

echo "verify-boot-iso.sh: booting ISO ${ISO_PATH}" | tee -a "${LOG_FILE}"
echo "verify-boot-iso.sh: probe label (build-time default): ${PROBE_LABEL}" | tee -a "${LOG_FILE}"

set +e
AKOYA_SKIP_INFERENCE_PREFLIGHT=1 \
    bash "${ROOT_DIR}/scripts/run-qemu.sh" --headless --boot-iso "${ISO_PATH}" 2>&1 | tee -a "${LOG_FILE}"
run_status=${PIPESTATUS[0]}
set -e

if ! grep -Fq "${BOOT_MESSAGE}" "${LOG_FILE}"; then
    echo "verify-boot-iso.sh: bootstrap message not found in ${LOG_FILE}" >&2
    exit 1
fi

if grep -Fq "${CHAT_HOST} reachable" "${LOG_FILE}"; then
    echo "verify-boot-iso.sh: connectivity probe succeeded (${CHAT_HOST} reachable)" | tee -a "${LOG_FILE}"
    echo "verify-boot-iso.sh: ISO verification passed" | tee -a "${LOG_FILE}"
    exit 0
fi

if grep -Eq 'net_ping=.*status=ok' "${LOG_FILE}"; then
    echo "verify-boot-iso.sh: connectivity probe succeeded (net_ping status=ok)" | tee -a "${LOG_FILE}"
    echo "verify-boot-iso.sh: ISO verification passed" | tee -a "${LOG_FILE}"
    exit 0
fi

if grep -Fq " reachable" "${LOG_FILE}"; then
    echo "verify-boot-iso.sh: connectivity probe succeeded (reachable line observed)" | tee -a "${LOG_FILE}"
    echo "verify-boot-iso.sh: ISO verification passed" | tee -a "${LOG_FILE}"
    exit 0
fi

if [[ ${run_status} -ne 0 ]]; then
    echo "verify-boot-iso.sh: run-qemu.sh exited ${run_status} (see ${LOG_FILE})" >&2
    exit "${run_status}"
fi

echo "verify-boot-iso.sh: connectivity probe success not found in ${LOG_FILE}" >&2
exit 1
