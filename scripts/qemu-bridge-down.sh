#!/usr/bin/env bash
# Tear down ephemeral macvtap used for QEMU LAN passthrough.
# MUST run as root (sudo). Never modifies WiFi or NetworkManager profiles.
set -euo pipefail

STATE_FILE="${AKOYA_QEMU_STATE_FILE:-/tmp/akoya-qemu-bridge.state}"
MACVTAP_IF="${AKOYA_QEMU_TAP_IF:-akoya-qemu0}"

log() {
    printf 'qemu-bridge-down.sh: %s\n' "$*" >&2
}

if [[ "${EUID}" -ne 0 ]]; then
    log "ERROR: must run as root (e.g. sudo $0)"
    exit 1
fi

if [[ -f "${STATE_FILE}" ]]; then
    # shellcheck disable=SC1090
    source "${STATE_FILE}"
    MACVTAP_IF="${MACVTAP_IF:-akoya-qemu0}"
fi

if [[ -n "${WIFI_IF:-}" ]] && bridge link show 2>/dev/null | grep -q "${WIFI_IF}.*master"; then
    log "ERROR: ${WIFI_IF} is enslaved to a bridge; manual intervention required"
    exit 1
fi

if ip link show "${MACVTAP_IF}" >/dev/null 2>&1; then
    log "removing macvtap ${MACVTAP_IF}"
    ip link del "${MACVTAP_IF}" 2>/dev/null || log "warning: failed to delete ${MACVTAP_IF}"
else
    log "macvtap ${MACVTAP_IF} not present"
fi

rm -f "${STATE_FILE}"

log "macvtap teardown complete (${LAN_IF:-wired parent} unchanged)"
