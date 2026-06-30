#!/usr/bin/env bash
# One-time admin install: copy macvtap helpers and bind CAP_NET_ADMIN for unprivileged runs.
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DEST="${AKOYA_LAN_LIBEXEC:-/usr/local/libexec/akoya}"
CAP_EXEC="${DEST}/bridge-cap-exec"
CAP_SRC="${ROOT_DIR}/scripts/bridge-cap-exec.c"

if [[ "${EUID}" -ne 0 ]]; then
    echo "install-bridge-libexec.sh: run as root once per workstation (e.g. sudo bash $0)" >&2
    exit 1
fi

mkdir -p "${DEST}"
install -m 0755 "${ROOT_DIR}/scripts/qemu-bridge-up.sh" "${DEST}/qemu-bridge-up.sh"
install -m 0755 "${ROOT_DIR}/scripts/qemu-bridge-down.sh" "${DEST}/qemu-bridge-down.sh"

if ! command -v gcc >/dev/null 2>&1; then
    echo "install-bridge-libexec.sh: gcc required to build bridge-cap-exec (apt install build-essential)" >&2
    exit 1
fi
if [[ ! -f /usr/include/sys/capability.h ]]; then
    echo "install-bridge-libexec.sh: libcap headers missing (apt install libcap-dev)" >&2
    exit 1
fi
if ! command -v setcap >/dev/null 2>&1; then
    echo "install-bridge-libexec.sh: setcap required (libcap2-bin package; apt install libcap2-bin)" >&2
    exit 1
fi

gcc -O2 -o "${CAP_EXEC}" "${CAP_SRC}" -lcap
chmod 0755 "${CAP_EXEC}"
if ! setcap cap_net_admin,cap_chown+ep "${CAP_EXEC}"; then
    echo "install-bridge-libexec.sh: setcap failed on ${CAP_EXEC}" >&2
    echo "Ensure ${DEST} is not mounted nosuid and libcap2-bin is installed." >&2
    exit 1
fi

cat > "${DEST}/qemu-bridge-up" <<EOF
#!/usr/bin/env bash
set -euo pipefail
exec "${CAP_EXEC}" "${DEST}/qemu-bridge-up.sh" "\$@"
EOF
cat > "${DEST}/qemu-bridge-down" <<EOF
#!/usr/bin/env bash
set -euo pipefail
exec "${CAP_EXEC}" "${DEST}/qemu-bridge-down.sh" "\$@"
EOF
chmod 0755 "${DEST}/qemu-bridge-up" "${DEST}/qemu-bridge-down"

echo "Installed ${DEST}/qemu-bridge-{up,down}.sh with ${CAP_EXEC} (cap_net_admin,cap_chown+ep)"
echo "Run make test / make run as your normal user after this one-time install."
echo "Optional legacy path: scripts/sudoers.d/akoya-qemu-bridge.example (not required when setcap install succeeds)."
