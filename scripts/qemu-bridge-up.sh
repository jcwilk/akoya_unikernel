#!/usr/bin/env bash
# Ephemeral macvtap setup for QEMU LAN passthrough on a designated wired interface.
# The physical parent keeps its host IP; no bridge enslavement or NM profile changes.
# MUST run as root (sudo). Never uses or modifies WiFi.
set -euo pipefail

STATE_FILE="${AKOYA_QEMU_STATE_FILE:-/tmp/akoya-qemu-bridge.state}"

LAN_IF="${AKOYA_QEMU_LAN_IF:-enx00e04c6801e8}"
WIFI_IF="${AKOYA_QEMU_WIFI_IF:-wlp82s0}"
MACVTAP_IF="${AKOYA_QEMU_TAP_IF:-akoya-qemu0}"
MACVTAP_MODE="${AKOYA_QEMU_MACVTAP_MODE:-bridge}"
GUEST_MAC="${AKOYA_QEMU_GUEST_MAC:-52:54:00:12:34:56}"
GUEST_MAC_FILE="${AKOYA_QEMU_GUEST_MAC_FILE:-/tmp/akoya-qemu-guest-mac}"

log() {
    printf 'qemu-bridge-up.sh: %s\n' "$*" >&2
}

fail() {
    log "ERROR: $*"
    exit 1
}

if [[ "${EUID}" -ne 0 ]]; then
    fail "must run as root (e.g. sudo $0)"
fi

if [[ "${LAN_IF}" == "${WIFI_IF}" ]]; then
    fail "LAN interface must not be the WiFi interface (${WIFI_IF})"
fi

if [[ ! -d "/sys/class/net/${LAN_IF}" ]]; then
    fail "LAN interface '${LAN_IF}' not found (set AKOYA_QEMU_LAN_IF)"
fi

if [[ ! -d "/sys/class/net/${WIFI_IF}" ]]; then
    fail "WiFi interface '${WIFI_IF}' not found (set AKOYA_QEMU_WIFI_IF)"
fi

lan_type="$(cat "/sys/class/net/${LAN_IF}/type" 2>/dev/null || echo 0)"
if [[ "${lan_type}" == "801" ]]; then
    fail "${LAN_IF} is wireless; choose a wired interface with AKOYA_QEMU_LAN_IF"
fi

if [[ -f "${GUEST_MAC_FILE}" ]]; then
    # shellcheck disable=SC1090
    source "${GUEST_MAC_FILE}"
    GUEST_MAC="${GUEST_MAC:-52:54:00:12:34:56}"
fi

if [[ -f "${STATE_FILE}" ]]; then
    # shellcheck disable=SC1090
    source "${STATE_FILE}"
    if [[ -d "/sys/class/net/${MACVTAP_IF:-}" ]]; then
        log "macvtap already configured (state file present)"
        exit 0
    fi
    log "stale state file without ${MACVTAP_IF}; removing"
    rm -f "${STATE_FILE}"
fi

if ip link show "${MACVTAP_IF}" >/dev/null 2>&1; then
    log "removing stale macvtap ${MACVTAP_IF} from a prior run"
    ip link del "${MACVTAP_IF}" 2>/dev/null || fail "failed to delete stale macvtap ${MACVTAP_IF}"
fi

if ! ip link show dev "${LAN_IF}" up 2>/dev/null | grep -q 'state UP'; then
    ip link set "${LAN_IF}" up 2>/dev/null || true
fi

if ! ip link add link "${LAN_IF}" name "${MACVTAP_IF}" type macvtap mode "${MACVTAP_MODE}"; then
    fail "failed to create macvtap ${MACVTAP_IF} on ${LAN_IF}"
fi

if ! ip link set dev "${MACVTAP_IF}" address "${GUEST_MAC}"; then
    ip link del "${MACVTAP_IF}" 2>/dev/null || true
    fail "failed to set macvtap MAC ${GUEST_MAC}"
fi

if ! ip link set "${MACVTAP_IF}" up; then
    ip link del "${MACVTAP_IF}" 2>/dev/null || true
    fail "failed to bring up macvtap ${MACVTAP_IF}"
fi

TAP_USER="${SUDO_USER:-${AKOYA_QEMU_USER:-root}}"
if [[ "${TAP_USER}" == "root" ]] && command -v logname >/dev/null 2>&1; then
    TAP_USER="$(logname 2>/dev/null || echo root)"
fi

TAP_INDEX="$(cat "/sys/class/net/${MACVTAP_IF}/ifindex")"
TAP_DEV="/dev/tap${TAP_INDEX}"

if [[ ! -e "${TAP_DEV}" ]]; then
    ip link del "${MACVTAP_IF}" 2>/dev/null || true
    fail "macvtap device node ${TAP_DEV} not found"
fi

if [[ "${TAP_USER}" != "root" ]]; then
    chown "${TAP_USER}:${TAP_USER}" "${TAP_DEV}" 2>/dev/null || \
        log "warning: could not chown ${TAP_DEV} to ${TAP_USER}; run QEMU as root or adjust permissions"
fi

cat > "${STATE_FILE}" <<EOF
LAN_IF=${LAN_IF}
WIFI_IF=${WIFI_IF}
MACVTAP_IF=${MACVTAP_IF}
MACVTAP_MODE=${MACVTAP_MODE}
GUEST_MAC=${GUEST_MAC}
TAP_DEV=${TAP_DEV}
TAP_USER=${TAP_USER}
EOF

log "macvtap ${MACVTAP_IF} ready on ${LAN_IF} (mode ${MACVTAP_MODE}; MAC ${GUEST_MAC}; ${TAP_DEV} for ${TAP_USER}; ${WIFI_IF} untouched)"
log "host keeps IP on ${LAN_IF}; guest uses separate MAC on the LAN"
