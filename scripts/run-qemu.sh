#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"
LOG_FILE="${BUILD_DIR}/qemu-smoke.log"

QEMU_BIN="${AKOYA_QEMU_BIN:-qemu-system-i386}"
TIMEOUT_SEC="${AKOYA_QEMU_TIMEOUT_SEC:-90}"
BOOT_MESSAGE="${AKOYA_BOOTSTRAP_MESSAGE:-akoya_unikernel bootstrap ok}"
GUEST_MAC="${AKOYA_QEMU_GUEST_MAC:-52:54:00:12:34:56}"
MACVTAP_IF="${AKOYA_QEMU_TAP_IF:-akoya-qemu0}"
LAN_IF="${AKOYA_QEMU_LAN_IF:-enx00e04c6801e8}"
STATE_FILE="${AKOYA_QEMU_STATE_FILE:-/tmp/akoya-qemu-bridge.state}"
AUTO_LAN="${AKOYA_AUTO_LAN:-${AKOYA_AUTO_BRIDGE:-1}}"
LAN_LIBEXEC="${AKOYA_LAN_LIBEXEC:-${AKOYA_BRIDGE_LIBEXEC:-/usr/local/libexec/akoya}}"

MODE=""
IMAGE_PATH=""
# Empty = use mode default; 1 = exit when guest triggers shutdown; 0 = hold QEMU open after guest halts
EXIT_ON_GUEST_DONE=""

usage() {
    cat >&2 <<EOF
Usage: $0 (--headful|--headless) [--image PATH] [--exit-on-guest-done | --hold]

  --headful     Interactive SDL display window
  --headless    No display; smoke-test timeout and bootstrap/network assertions
  --image PATH  Boot image to run (default: auto-select when exactly one logical image exists)

  --exit-on-guest-done  QEMU exits when the guest finishes (default for --headless)
  --hold                Keep QEMU open after the guest halts; close the window or Ctrl+C (default for --headful)

Display mode is mandatory: specify exactly one of --headful or --headless.
Every run attaches an RTL8139 NIC on the workstation LAN via ephemeral macvtap on ${LAN_IF}
with fixed guest MAC ${GUEST_MAC}.

Environment:
  AKOYA_QEMU_LAN_IF       Wired interface for macvtap parent (default: enx00e04c6801e8)
  AKOYA_QEMU_GUEST_MAC    Fixed guest MAC (default: 52:54:00:12:34:56)
  AKOYA_QEMU_TIMEOUT_SEC  Headless timeout (default: 90)
  AKOYA_AUTO_LAN          1 = macvtap up/down around each run (default: 1)
  AKOYA_LAN_LIBEXEC       Installed helper scripts for passwordless sudo (default: /usr/local/libexec/akoya)

Install ${LAN_LIBEXEC}/qemu-bridge-{up,down}.sh with matching sudoers; see README.

Requires: qemu-system-i386 (qemu-system-x86 package on Debian/Ubuntu).
EOF
}

fail() {
    echo "run-qemu.sh: $*" >&2
    exit 1
}

lan_script_path() {
    local name="$1"
    local libexec="${LAN_LIBEXEC%/}/${name}"
    if [[ -x "${libexec}" ]]; then
        echo "${libexec}"
        return 0
    fi
    local repo_script="${ROOT_DIR}/scripts/${name}"
    if [[ -x "${repo_script}" ]]; then
        echo "${repo_script}"
        return 0
    fi
    return 1
}

run_lan_script() {
    local script="$1"

    if [[ "${EUID}" -eq 0 ]]; then
        AKOYA_QEMU_LAN_IF="${LAN_IF}" \
            AKOYA_QEMU_WIFI_IF="${AKOYA_QEMU_WIFI_IF:-wlp82s0}" \
            AKOYA_QEMU_TAP_IF="${MACVTAP_IF}" \
            AKOYA_QEMU_STATE_FILE="${STATE_FILE}" \
            "${script}"
        return $?
    fi

    if ! command -v sudo >/dev/null 2>&1; then
        return 127
    fi

    sudo -n "${script}"
}

lan_up() {
    if [[ "${AUTO_LAN}" != "1" ]]; then
        return 0
    fi

    local up_script down_script
    if ! up_script="$(lan_script_path qemu-bridge-up.sh)"; then
        fail "missing LAN setup script (install ${LAN_LIBEXEC}/qemu-bridge-up.sh or use repo scripts/)"
    fi
    down_script="$(lan_script_path qemu-bridge-down.sh)" || true

    run_lan_script "${down_script}" 2>/dev/null || true

    printf 'GUEST_MAC=%s\n' "${GUEST_MAC}" > /tmp/akoya-qemu-guest-mac

    local attempt
    for attempt in 1 2; do
        if run_lan_script "${up_script}" && [[ -f "${STATE_FILE}" ]]; then
            return 0
        fi
        if [[ "${attempt}" -lt 2 ]]; then
            echo "run-qemu.sh: macvtap setup attempt ${attempt} failed; retrying..." >&2
            run_lan_script "${down_script}" 2>/dev/null || true
            sleep 1
        fi
    done

    fail "ephemeral macvtap setup failed. See ${up_script} output above.
Install libexec scripts + sudoers (README), or run manually:
  sudo -n ${LAN_LIBEXEC}/qemu-bridge-up.sh
Or set AKOYA_AUTO_LAN=0 if macvtap is already configured."
}

lan_down() {
    if [[ "${AUTO_LAN}" != "1" ]]; then
        return 0
    fi

    local down_script
    if ! down_script="$(lan_script_path qemu-bridge-down.sh)"; then
        return 0
    fi

    run_lan_script "${down_script}" || true
}

check_lan_prerequisites() {
    if [[ ! -d "/sys/class/net/${LAN_IF}" ]]; then
        fail "wired LAN interface '${LAN_IF}' not found (set AKOYA_QEMU_LAN_IF)"
    fi

    if [[ ! -d "/sys/class/net/${MACVTAP_IF}" ]]; then
        fail "macvtap '${MACVTAP_IF}' not found after setup.
Run: sudo -n ${LAN_LIBEXEC}/qemu-bridge-up.sh"
    fi

    if [[ ! -f "${STATE_FILE}" ]]; then
        fail "state file '${STATE_FILE}' missing after setup.
Run: sudo -n ${LAN_LIBEXEC}/qemu-bridge-up.sh"
    fi

    # shellcheck disable=SC1090
    source "${STATE_FILE}"

    if [[ -z "${TAP_DEV:-}" || ! -e "${TAP_DEV}" ]]; then
        fail "tap device '${TAP_DEV:-}' not available after macvtap setup"
    fi
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --headful)
            if [[ -n "${MODE}" ]]; then
                fail "specify exactly one display mode (--headful or --headless), not both"
            fi
            MODE="headful"
            shift
            ;;
        --headless)
            if [[ -n "${MODE}" ]]; then
                fail "specify exactly one display mode (--headful or --headless), not both"
            fi
            MODE="headless"
            shift
            ;;
        --image)
            IMAGE_PATH="$2"
            shift 2
            ;;
        --exit-on-guest-done)
            if [[ "${EXIT_ON_GUEST_DONE}" == "0" ]]; then
                fail "specify only one of --exit-on-guest-done or --hold"
            fi
            EXIT_ON_GUEST_DONE=1
            shift
            ;;
        --hold)
            if [[ "${EXIT_ON_GUEST_DONE}" == "1" ]]; then
                fail "specify only one of --exit-on-guest-done or --hold"
            fi
            EXIT_ON_GUEST_DONE=0
            shift
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            fail "unknown argument: $1 (see --help)"
            ;;
    esac
done

if [[ -z "${MODE}" ]]; then
    fail "display mode is required: specify --headful or --headless"
fi

if [[ -z "${EXIT_ON_GUEST_DONE}" ]]; then
    if [[ "${MODE}" == "headless" ]]; then
        EXIT_ON_GUEST_DONE=1
    else
        EXIT_ON_GUEST_DONE=0
    fi
fi

if ! command -v "${QEMU_BIN}" >/dev/null 2>&1; then
    fail "${QEMU_BIN} not found; install qemu-system-x86 (e.g. sudo apt install qemu-system-x86)"
fi

trap lan_down EXIT INT TERM
lan_up
check_lan_prerequisites

# Collect stems of runnable images (*.elf, *.bin) directly under BUILD_DIR.
discover_logical_stems() {
    local stems=()
    local f stem
    shopt -s nullglob
    for f in "${BUILD_DIR}"/*.elf "${BUILD_DIR}"/*.bin; do
        [[ -f "${f}" ]] || continue
        stem="$(basename "${f}")"
        stem="${stem%.elf}"
        stem="${stem%.bin}"
        stems+=("${stem}")
    done
    shopt -u nullglob

    local unique=() seen
    for stem in "${stems[@]}"; do
        seen=0
        for u in "${unique[@]:-}"; do
            if [[ "${u}" == "${stem}" ]]; then
                seen=1
                break
            fi
        done
        if [[ "${seen}" -eq 0 ]]; then
            unique+=("${stem}")
        fi
    done
    if [[ ${#unique[@]} -gt 0 ]]; then
        printf '%s\n' "${unique[@]}"
    fi
}

resolve_image_for_stem() {
    local stem="$1"
    local elf="${BUILD_DIR}/${stem}.elf"
    local bin="${BUILD_DIR}/${stem}.bin"
    if [[ -f "${elf}" ]]; then
        echo "${elf}"
    elif [[ -f "${bin}" ]]; then
        echo "${bin}"
    else
        return 1
    fi
}

resolve_kernel_image() {
    if [[ -n "${IMAGE_PATH}" ]]; then
        if [[ ! -f "${IMAGE_PATH}" ]]; then
            fail "image not found: ${IMAGE_PATH}"
        fi
        echo "${IMAGE_PATH}"
        return
    fi

    mapfile -t stems < <(discover_logical_stems)

    if [[ ${#stems[@]} -eq 0 ]]; then
        fail "no runnable image found in ${BUILD_DIR}; run 'make build' first"
    fi

    if [[ ${#stems[@]} -gt 1 ]]; then
        local stem_list
        stem_list="$(printf '%s, ' "${stems[@]}")"
        stem_list="${stem_list%, }"
        fail "multiple logical images in ${BUILD_DIR} (${stem_list}); specify one with --image PATH"
    fi

    resolve_image_for_stem "${stems[0]}"
}

main() {
    mkdir -p "${BUILD_DIR}"

    local kernel_image
    kernel_image="$(resolve_kernel_image)"

    : > "${LOG_FILE}"

    local display_args=(-display none)
    local run_timeout=("${TIMEOUT_SEC}")
    if [[ "${MODE}" == "headful" ]]; then
        display_args=(-display sdl)
        if [[ "${EXIT_ON_GUEST_DONE}" -eq 0 ]]; then
            echo "Running QEMU headfully (guest will halt; close the window or Ctrl+C when done)" | tee -a "${LOG_FILE}"
        else
            echo "Running QEMU headfully" | tee -a "${LOG_FILE}"
        fi
        run_timeout=()
    else
        if [[ "${EXIT_ON_GUEST_DONE}" -eq 0 ]]; then
            echo "Running QEMU headless with --hold (no auto-exit on guest shutdown)" | tee -a "${LOG_FILE}"
        else
            echo "Running QEMU headless smoke test (timeout ${TIMEOUT_SEC}s)" | tee -a "${LOG_FILE}"
        fi
    fi

    echo "Boot image: ${kernel_image}" | tee -a "${LOG_FILE}"
    echo "LAN: macvtap ${MACVTAP_IF} on ${LAN_IF}, guest MAC: ${GUEST_MAC}" | tee -a "${LOG_FILE}"

    # shellcheck disable=SC1090
    source "${STATE_FILE}"

    if [[ -f "/sys/class/net/${MACVTAP_IF}/address" ]]; then
        local actual_mac
        actual_mac="$(cat "/sys/class/net/${MACVTAP_IF}/address")"
        if [[ "${actual_mac}" != "${GUEST_MAC}" ]]; then
            echo "run-qemu.sh: macvtap MAC ${actual_mac} differs from configured ${GUEST_MAC}; using interface MAC for this run (reinstall libexec to pin guest MAC)" | tee -a "${LOG_FILE}"
            GUEST_MAC="${actual_mac}"
        fi
    fi

    exec 3<>"${TAP_DEV}"

    local -a qemu_args=(
        -m 512
        -cpu pentium3
        -smp 1
        "${display_args[@]}"
        -serial stdio
        -no-reboot
        -netdev "tap,fd=3,id=net0"
        -device "rtl8139,netdev=net0,mac=${GUEST_MAC}"
    )
    if [[ "${EXIT_ON_GUEST_DONE}" -eq 1 ]]; then
        qemu_args+=(-device isa-debug-exit,iobase=0xf4,iosize=0x04)
    fi
    qemu_args+=(-kernel "${kernel_image}")

    set +e
    if [[ ${#run_timeout[@]} -gt 0 ]]; then
        timeout "${run_timeout[@]}" "${QEMU_BIN}" "${qemu_args[@]}" 2>&1 | tee -a "${LOG_FILE}"
    else
        "${QEMU_BIN}" "${qemu_args[@]}" 2>&1 | tee -a "${LOG_FILE}"
    fi
    local qemu_status=${PIPESTATUS[0]}
    exec 3>&- 2>/dev/null || true
    set -e

    if [[ "${MODE}" == "headful" ]]; then
        echo "QEMU headful session ended (exit ${qemu_status})" | tee -a "${LOG_FILE}"
        exit "${qemu_status}"
    fi

    if [[ ${qemu_status} -eq 124 ]] && ! grep -Fq "${BOOT_MESSAGE}" "${LOG_FILE}"; then
        fail "timed out waiting for bootstrap output"
    fi

    if [[ ${qemu_status} -ne 0 ]] && ! grep -Fq "${BOOT_MESSAGE}" "${LOG_FILE}"; then
        fail "qemu exited with status ${qemu_status}"
    fi

    if ! grep -Fq "${BOOT_MESSAGE}" "${LOG_FILE}"; then
        fail "expected bootstrap message not found in captured output"
    fi

    if ! grep -Eq '^net_ip=[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+$' "${LOG_FILE}"; then
        fail "expected net_ip= line not found in captured output"
    fi

    if ! grep -Eq '^net_ping=.* status=ok rtt_ms=[0-9]+$' "${LOG_FILE}"; then
        fail "expected successful net_ping= line not found in captured output"
    fi

    echo "QEMU smoke test passed" | tee -a "${LOG_FILE}"
}

main "$@"
