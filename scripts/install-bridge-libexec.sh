#!/usr/bin/env bash
# Install macvtap helper scripts for passwordless sudo (see scripts/sudoers.d/).
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DEST="${AKOYA_LAN_LIBEXEC:-/usr/local/libexec/akoya}"

if [[ "${EUID}" -ne 0 ]]; then
    echo "install-bridge-libexec.sh: run as root (e.g. sudo $0)" >&2
    exit 1
fi

mkdir -p "${DEST}"
install -m 0755 "${ROOT_DIR}/scripts/qemu-bridge-up.sh" "${DEST}/qemu-bridge-up.sh"
install -m 0755 "${ROOT_DIR}/scripts/qemu-bridge-down.sh" "${DEST}/qemu-bridge-down.sh"
echo "Installed ${DEST}/qemu-bridge-{up,down}.sh"
echo "Add scripts/sudoers.d/akoya-qemu-bridge.example to /etc/sudoers.d/ (visudo) for passwordless make test."
