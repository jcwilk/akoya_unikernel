#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"
LOG_FILE="${BUILD_DIR}/qemu-smoke.log"
AKOYA_PROFILE="${ROOT_DIR}/target/akoya.profile"

# shellcheck source=target/akoya.profile
source "${AKOYA_PROFILE}"

QEMU_BIN="${AKOYA_QEMU_BIN:-qemu-system-i386}"
TIMEOUT_SEC="${AKOYA_QEMU_TIMEOUT_SEC:-300}"
BOOT_MESSAGE="${AKOYA_BOOTSTRAP_MESSAGE:-akoya_unikernel bootstrap ok}"
CHAT_HOST="${AKOYA_CHAT_HOST_IP:-192.168.1.110}"
CHAT_PORT="${AKOYA_CHAT_PORT:-11435}"
DEFAULT_CHAT_SCRIPT_FILE="${ROOT_DIR}/scripts/fixtures/multi-turn-pong.akoya-script"
DEFAULT_CHAT_SCRIPT="h i ret w h a t ret q u i t ret"
CHAT_SCRIPT="${AKOYA_CHAT_SCRIPT:-${DEFAULT_CHAT_SCRIPT}}"
MONITOR_SOCK="${BUILD_DIR}/qemu-monitor.sock"
GUEST_MAC="${AKOYA_QEMU_GUEST_MAC:-52:54:00:12:34:56}"
MACVTAP_IF="${AKOYA_QEMU_TAP_IF:-akoya-qemu0}"
LAN_IF="${AKOYA_QEMU_LAN_IF:-enx00e04c6801e8}"
STATE_FILE="${AKOYA_QEMU_STATE_FILE:-/tmp/akoya-qemu-bridge.state}"
AUTO_LAN="${AKOYA_AUTO_LAN:-${AKOYA_AUTO_BRIDGE:-1}}"
LAN_LIBEXEC="${AKOYA_LAN_LIBEXEC:-${AKOYA_BRIDGE_LIBEXEC:-/usr/local/libexec/akoya}}"

# shellcheck source=scripts/inference-preflight.sh
source "${ROOT_DIR}/scripts/inference-preflight.sh"
# shellcheck source=scripts/chat-script-harness.sh
source "${ROOT_DIR}/scripts/chat-script-harness.sh"

MODE=""
IMAGE_PATH=""
LOGICAL_ID=""
BOOT_ISO_PATH=""
BOOT_DISK_PATH=""
ISO_SMOKE=0
SCRIPT_FILE=""
SCRIPT_INJECT_STATUS=0
# Empty = use mode default; 1 = exit when guest triggers shutdown; 0 = hold QEMU open after guest halts
EXIT_ON_GUEST_DONE=""

usage() {
    cat >&2 <<EOF
Usage: $0 (--headful|--headless) [--image PATH | --logical NAME | --boot-iso PATH | --boot-disk PATH] [--script FILE] [--exit-on-guest-done | --hold]

  --headful     Interactive SDL display window
  --headless    No display; smoke-test timeout and bootstrap/network assertions
  --image PATH  Boot image to run (default: auto-select when exactly one logical image exists)
  --logical NAME  Select logical image stem (e.g. kernel, transport-test, chat-regression-test)
  --boot-iso PATH  Boot from BIOS/Legacy optical media (mutually exclusive with --image / --logical / --boot-disk)
  --boot-disk PATH Boot from raw MBR disk image as first hard disk (USB/HDD image from make usb)
  --script FILE Headless scripted chat interaction with output assertions (*.akoya-script)

  --exit-on-guest-done  QEMU exits when the guest finishes (default for --headless)
  --hold                Keep QEMU open after the guest halts; close the window or Ctrl+C (default for --headful)

Display mode is mandatory: specify exactly one of --headful or --headless.
Every run attaches an RTL8139 NIC on the workstation LAN via ephemeral macvtap on ${LAN_IF}
with fixed guest MAC ${GUEST_MAC}.

Environment:
  AKOYA_QEMU_LAN_IF       Wired interface for macvtap parent (default: enx00e04c6801e8)
  AKOYA_QEMU_GUEST_MAC    Fixed guest MAC (default: 52:54:00:12:34:56)
  AKOYA_QEMU_TIMEOUT_SEC  Headless timeout (default: 300)
  AKOYA_CHAT_HOST_IP       Chat endpoint host for pre-flight (default: 192.168.1.110)
  AKOYA_CHAT_PORT          Chat endpoint port for pre-flight (default: 11435)
  AKOYA_CHAT_SCRIPT        Legacy space-separated sendkey names when AKOYA_USE_KEYBOARD_SCRIPT=1
  AKOYA_USE_KEYBOARD_SCRIPT 1 = use AKOYA_CHAT_SCRIPT instead of default multi-turn *.akoya-script
  AKOYA_AUTO_LAN          1 = macvtap up/down around each run (default: 1)
  AKOYA_LAN_LIBEXEC       Installed helper scripts with CAP_NET_ADMIN (default: /usr/local/libexec/akoya)
  AKOYA_SKIP_INFERENCE_PREFLIGHT  1 = skip chat-endpoint pre-flight (ISO smoke / verify-boot-iso)
  AKOYA_QEMU_CPU                  Override QEMU -cpu model (default from AKOYA_CPU_CLASS in target/akoya.profile)

ISO boot (--boot-iso) uses BIOS/Legacy optical media. Disk boot (--boot-disk) uses -hda
(boot order c) for akoya-boot.img from make usb. Both skip inference pre-flight in
headless mode and assert bootstrap + connectivity probe only.

Install ${LAN_LIBEXEC}/qemu-bridge-{up,down} via scripts/install-bridge-libexec.sh (one-time admin); see README.

Requires: qemu-system-i386 (qemu-system-x86 package on Debian/Ubuntu).
EOF
}

fail() {
    echo "run-qemu.sh: $*" >&2
    exit 1
}

chat_endpoint_reachable() {
    inference_endpoint_reachable "${CHAT_HOST}" "${CHAT_PORT}"
}

send_monitor_command() {
    local cmd="$1"

    if command -v socat >/dev/null 2>&1; then
        printf '%s\n' "${cmd}" | socat - "UNIX-CONNECT:${MONITOR_SOCK}" >/dev/null 2>&1
        return $?
    fi

    if command -v nc >/dev/null 2>&1; then
        printf '%s\n' "${cmd}" | nc -U "${MONITOR_SOCK}" >/dev/null 2>&1
        return $?
    fi

    if command -v python3 >/dev/null 2>&1; then
        python3 - "${cmd}" "${MONITOR_SOCK}" <<'PY'
import socket
import sys

cmd = sys.argv[1]
sock_path = sys.argv[2]
s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
s.settimeout(2)
s.connect(sock_path)
s.sendall((cmd + "\n").encode())
s.close()
PY
        return $?
    fi

    return 1
}

inject_chat_script() {
    local waited=0

    while [[ ! -S "${MONITOR_SOCK}" && ${waited} -lt 240 ]]; do
        sleep 0.25
        waited=$((waited + 1))
    done
    if [[ ! -S "${MONITOR_SOCK}" ]]; then
        echo "run-qemu.sh: monitor socket not ready for keyboard injection" >&2
        return 1
    fi

    waited=0
    while ! grep -Fq "${CHAT_HOST} reachable" "${LOG_FILE}" && [[ ${waited} -lt 600 ]]; do
        sleep 0.5
        waited=$((waited + 1))
    done
    if ! grep -Fq "${CHAT_HOST} reachable" "${LOG_FILE}"; then
        echo "run-qemu.sh: reachability line not observed before keyboard injection" >&2
        return 1
    fi

    waited=0
    while ! grep -Fq "> " "${LOG_FILE}" && [[ ${waited} -lt 600 ]]; do
        sleep 0.5
        waited=$((waited + 1))
    done
    if ! grep -Fq "> " "${LOG_FILE}"; then
        echo "run-qemu.sh: chat prompt not observed before keyboard injection" >&2
        return 1
    fi

    sleep 0.5
    local key
    for key in ${CHAT_SCRIPT}; do
        send_monitor_command "sendkey ${key}" || return 1
        if [[ "${key}" == "ret" ]]; then
            local start_lines
            start_lines="$(wc -l < "${LOG_FILE}")"
            local turn_waited=0
            while [[ ${turn_waited} -lt 240 ]]; do
                if grep -Fq "chat_session=exit" "${LOG_FILE}"; then
                    break
                fi
                local now_lines
                now_lines="$(wc -l < "${LOG_FILE}")"
                if [[ ${now_lines} -gt ${start_lines} ]]; then
                    sleep 0.5
                    break
                fi
                sleep 0.25
                turn_waited=$((turn_waited + 1))
            done
            sleep 0.5
        else
            sleep 0.05
        fi
    done
    return 0
}

inject_akoya_script() {
    local waited=0
    local status=0

    while [[ ! -S "${MONITOR_SOCK}" && ${waited} -lt 240 ]]; do
        sleep 0.25
        waited=$((waited + 1))
    done
    if [[ ! -S "${MONITOR_SOCK}" ]]; then
        echo "run-qemu.sh: monitor socket not ready for scripted keyboard injection" >&2
        return 1
    fi

    if ! chat_script_wait_for_reachability "${LOG_FILE}"; then
        send_monitor_command "quit" || true
        return 1
    fi

    if ! chat_script_execute_interactive_steps "${LOG_FILE}" send_monitor_command; then
        send_monitor_command "quit" || true
        return 1
    fi

    return 0
}

count_plain_reply_lines() {
    awk -v host="${CHAT_HOST}" '
        $0 ~ ("^" host " reachable$") { found = 1; next }
        !found { next }
        /^> / { next }
        /^chat_session=/ { next }
        /^chat failed:/ { next }
        /^net_/ { next }
        /^run-qemu\.sh:/ { next }
        /^Running QEMU/ { next }
        /^Boot image:/ { next }
        /^LAN:/ { next }
        /^QEMU smoke test/ { next }
        /^$/ { next }
        { count++ }
        END { print count + 0 }
    ' "${LOG_FILE}"
}

lan_script_path() {
    local name="$1"
    local base="${name%.sh}"
    local libexec="${LAN_LIBEXEC%/}"

    if [[ -x "${libexec}/${base}" ]]; then
        echo "${libexec}/${base}"
        return 0
    fi
    if [[ -x "${libexec}/${name}" ]]; then
        echo "${libexec}/${name}"
        return 0
    fi
    if [[ -x "${ROOT_DIR}/scripts/${name}" ]]; then
        echo "${ROOT_DIR}/scripts/${name}"
        return 0
    fi
    return 1
}

run_lan_script() {
    local script="$1"

    AKOYA_QEMU_LAN_IF="${LAN_IF}" \
        AKOYA_QEMU_WIFI_IF="${AKOYA_QEMU_WIFI_IF:-wlp82s0}" \
        AKOYA_QEMU_TAP_IF="${MACVTAP_IF}" \
        AKOYA_QEMU_STATE_FILE="${STATE_FILE}" \
        "${script}"
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
Install libexec helpers once (README: scripts/install-bridge-libexec.sh), or run manually:
  ${LAN_LIBEXEC}/qemu-bridge-up
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
Run: ${LAN_LIBEXEC}/qemu-bridge-up"
    fi

    if [[ ! -f "${STATE_FILE}" ]]; then
        fail "state file '${STATE_FILE}' missing after setup.
Run: ${LAN_LIBEXEC}/qemu-bridge-up"
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
        --logical)
            LOGICAL_ID="$2"
            shift 2
            ;;
        --boot-iso)
            BOOT_ISO_PATH="$2"
            ISO_SMOKE=1
            shift 2
            ;;
        --boot-disk)
            BOOT_DISK_PATH="$2"
            ISO_SMOKE=1
            shift 2
            ;;
        --script)
            SCRIPT_FILE="$2"
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

if [[ -n "${BOOT_ISO_PATH}" ]]; then
    if [[ -n "${IMAGE_PATH}" || -n "${LOGICAL_ID}" || -n "${BOOT_DISK_PATH}" ]]; then
        fail "specify only one of --boot-iso, --boot-disk, --image, or --logical"
    fi
    if [[ ! -f "${BOOT_ISO_PATH}" ]]; then
        fail "boot ISO not found: ${BOOT_ISO_PATH}"
    fi
    ISO_SMOKE=1
fi

if [[ -n "${BOOT_DISK_PATH}" ]]; then
    if [[ -n "${IMAGE_PATH}" || -n "${LOGICAL_ID}" || -n "${BOOT_ISO_PATH}" ]]; then
        fail "specify only one of --boot-iso, --boot-disk, --image, or --logical"
    fi
    if [[ ! -f "${BOOT_DISK_PATH}" ]]; then
        fail "boot disk image not found: ${BOOT_DISK_PATH}"
    fi
    ISO_SMOKE=1
fi

if [[ -z "${SCRIPT_FILE}" && "${MODE}" == "headless" && "${ISO_SMOKE}" -eq 0 && "${AKOYA_USE_KEYBOARD_SCRIPT:-0}" != "1" ]]; then
    if [[ -z "${LOGICAL_ID}" || "${LOGICAL_ID}" == "kernel" ]]; then
        SCRIPT_FILE="${DEFAULT_CHAT_SCRIPT_FILE}"
    fi
fi

if [[ -n "${SCRIPT_FILE}" && "${MODE}" != "headless" ]]; then
    fail "--script requires --headless (headful script injection is not supported)"
fi

if [[ -n "${SCRIPT_FILE}" && ! -f "${SCRIPT_FILE}" ]]; then
    fail "script file not found: ${SCRIPT_FILE}"
fi

if [[ -n "${SCRIPT_FILE}" ]]; then
    if ! chat_script_parse_file "${SCRIPT_FILE}"; then
        fail "failed to parse script: ${SCRIPT_FILE}"
    fi
fi

if [[ -z "${EXIT_ON_GUEST_DONE}" ]]; then
    if [[ "${MODE}" == "headless" ]]; then
        EXIT_ON_GUEST_DONE=1
    else
        EXIT_ON_GUEST_DONE=0
    fi
fi

if ! command -v "${QEMU_BIN}" >/dev/null 2>&1; then
    fail "${QEMU_BIN} not found; install qemu-system-x86 (apt install qemu-system-x86)"
fi

if [[ "${MODE}" == "headless" && "${AKOYA_SKIP_INFERENCE_PREFLIGHT:-0}" != "1" && "${ISO_SMOKE}" -eq 0 ]]; then
    if ! inference_preflight "run-qemu.sh"; then
        exit 1
    fi
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
    if [[ -n "${IMAGE_PATH}" && -n "${LOGICAL_ID}" ]]; then
        fail "specify only one of --image PATH or --logical NAME"
    fi

    if [[ -n "${LOGICAL_ID}" ]]; then
        resolve_image_for_stem "${LOGICAL_ID}" || fail "logical image '${LOGICAL_ID}' not found in ${BUILD_DIR}"
        return
    fi

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

image_is_transport_test() {
    local image_path="$1"
    [[ "$(basename "${image_path}")" == transport-test.bin || "$(basename "${image_path}")" == transport-test.elf ]]
}

image_is_chat_regression_test() {
    local image_path="$1"
    [[ "$(basename "${image_path}")" == chat-regression-test.bin || "$(basename "${image_path}")" == chat-regression-test.elf ]]
}

resolve_qemu_cpu() {
    if [[ -n "${AKOYA_QEMU_CPU:-}" ]]; then
        echo "${AKOYA_QEMU_CPU}"
        return 0
    fi

    case "${AKOYA_CPU_CLASS:-}" in
        pentium-m|pentium_m)
            # Closest faithful 32-bit model on typical QEMU (qemu-system-i386 -cpu help).
            echo "qemu32"
            ;;
        pentium3)
            echo "pentium3"
            ;;
        pentium|pentium2)
            echo "${AKOYA_CPU_CLASS}"
            ;;
        *)
            echo "qemu32"
            ;;
    esac
}

main() {
    mkdir -p "${BUILD_DIR}"

    local kernel_image=""
    local transport_mode=0
    local chat_regression_mode=0
    if [[ -n "${BOOT_ISO_PATH}" ]]; then
        kernel_image="${BOOT_ISO_PATH}"
    elif [[ -n "${BOOT_DISK_PATH}" ]]; then
        kernel_image="${BOOT_DISK_PATH}"
    else
        kernel_image="$(resolve_kernel_image)"
        if image_is_transport_test "${kernel_image}"; then
            transport_mode=1
        elif image_is_chat_regression_test "${kernel_image}"; then
            chat_regression_mode=1
        fi
    fi

    : > "${LOG_FILE}"

    local qemu_cpu
    qemu_cpu="$(resolve_qemu_cpu)"

    local display_args=(-display none)
    local vga_args=()
    local run_timeout=("${TIMEOUT_SEC}")
    if [[ "${MODE}" == "headful" ]]; then
        display_args=(-display sdl)
        vga_args=(-vga std)
        if [[ "${EXIT_ON_GUEST_DONE}" -eq 0 ]]; then
            echo "Running QEMU headfully (SDL keyboard uses emulated i8042 PS/2 path; close window or Ctrl+C when done)" | tee -a "${LOG_FILE}"
        else
            echo "Running QEMU headfully (SDL keyboard uses emulated i8042 PS/2 path)" | tee -a "${LOG_FILE}"
        fi
        run_timeout=()
    else
        if [[ "${EXIT_ON_GUEST_DONE}" -eq 0 ]]; then
            echo "Running QEMU headless with --hold (no auto-exit on guest shutdown)" | tee -a "${LOG_FILE}"
        elif [[ "${ISO_SMOKE}" -eq 1 && -n "${BOOT_DISK_PATH}" ]]; then
            echo "Running QEMU headless disk smoke test (timeout ${TIMEOUT_SEC}s, boot disk: ${BOOT_DISK_PATH})" | tee -a "${LOG_FILE}"
        elif [[ "${ISO_SMOKE}" -eq 1 ]]; then
            echo "Running QEMU headless ISO smoke test (timeout ${TIMEOUT_SEC}s, boot ISO: ${BOOT_ISO_PATH})" | tee -a "${LOG_FILE}"
        elif [[ "${chat_regression_mode}" -eq 1 ]]; then
            echo "Running QEMU headless timed-gap chat regression (timeout ${TIMEOUT_SEC}s)" | tee -a "${LOG_FILE}"
        elif [[ -n "${SCRIPT_FILE}" ]]; then
            echo "Running QEMU headless scripted chat test (timeout ${TIMEOUT_SEC}s, script: ${SCRIPT_FILE})" | tee -a "${LOG_FILE}"
        else
            echo "Running QEMU headless smoke test (timeout ${TIMEOUT_SEC}s, keyboard script: ${CHAT_SCRIPT})" | tee -a "${LOG_FILE}"
        fi
    fi

    if [[ "${ISO_SMOKE}" -eq 1 && -n "${BOOT_ISO_PATH}" ]]; then
        echo "Boot ISO: ${BOOT_ISO_PATH}" | tee -a "${LOG_FILE}"
    elif [[ "${ISO_SMOKE}" -eq 1 && -n "${BOOT_DISK_PATH}" ]]; then
        echo "Boot disk: ${BOOT_DISK_PATH}" | tee -a "${LOG_FILE}"
    else
        echo "Boot image: ${kernel_image}" | tee -a "${LOG_FILE}"
    fi
    echo "LAN: macvtap ${MACVTAP_IF} on ${LAN_IF}, guest MAC: ${GUEST_MAC}" | tee -a "${LOG_FILE}"
    echo "QEMU CPU: ${qemu_cpu} (profile class: ${AKOYA_CPU_CLASS:-unset})" | tee -a "${LOG_FILE}"

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
        -cpu "${qemu_cpu}"
        -smp 1
        "${display_args[@]}"
        "${vga_args[@]}"
        -serial stdio
        -no-reboot
        -netdev "tap,fd=3,id=net0"
        -device "rtl8139,netdev=net0,mac=${GUEST_MAC}"
    )
    if [[ "${EXIT_ON_GUEST_DONE}" -eq 1 ]]; then
        qemu_args+=(-device isa-debug-exit,iobase=0xf4,iosize=0x04)
    fi

    local inject_pid=""
    if [[ "${MODE}" == "headless" && "${transport_mode}" -eq 0 && "${chat_regression_mode}" -eq 0 && "${ISO_SMOKE}" -eq 0 ]]; then
        rm -f "${MONITOR_SOCK}"
        qemu_args+=(-monitor "unix:${MONITOR_SOCK},server,nowait")
        if [[ -n "${SCRIPT_FILE}" ]]; then
            inject_akoya_script &
            inject_pid=$!
        else
            inject_chat_script &
            inject_pid=$!
        fi
    fi

    if [[ "${ISO_SMOKE}" -eq 1 && -n "${BOOT_ISO_PATH}" ]]; then
        qemu_args+=(-boot order=d -cdrom "${BOOT_ISO_PATH}")
    elif [[ "${ISO_SMOKE}" -eq 1 && -n "${BOOT_DISK_PATH}" ]]; then
        qemu_args+=(-boot order=c -hda "${BOOT_DISK_PATH}")
    else
        qemu_args+=(-kernel "${kernel_image}")
    fi

    set +e
    if [[ ${#run_timeout[@]} -gt 0 ]]; then
        timeout "${run_timeout[@]}" "${QEMU_BIN}" "${qemu_args[@]}" 2>&1 | tee -a "${LOG_FILE}"
    else
        "${QEMU_BIN}" "${qemu_args[@]}" 2>&1 | tee -a "${LOG_FILE}"
    fi
    local qemu_status=${PIPESTATUS[0]}
    exec 3>&- 2>/dev/null || true
    if [[ -n "${inject_pid}" ]]; then
        local inject_status=0
        wait "${inject_pid}" 2>/dev/null || inject_status=$?
        if [[ ${inject_status} -ne 0 ]]; then
            fail "headless keyboard/script injection failed (exit ${inject_status})"
        fi
    fi
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

    if [[ "${transport_mode}" -eq 1 ]]; then
        if grep -Fq "transport-test: ALL PASS" "${LOG_FILE}"; then
            echo "QEMU transport-test passed" | tee -a "${LOG_FILE}"
            exit 0
        fi
        if grep -Fq "transport-test: FAILED" "${LOG_FILE}"; then
            fail "transport-test aggregate failure reported in captured output"
        fi
        fail "transport-test aggregate result not found in captured output"
    fi

    if [[ "${chat_regression_mode}" -eq 1 ]]; then
        if grep -Fq "timed-gap-chat-regression: ALL PASS" "${LOG_FILE}"; then
            if grep -q '^chat failed:' "${LOG_FILE}"; then
                fail "chat failed detected during timed-gap chat regression"
            fi
            local turn_pass_count
            turn_pass_count="$(grep -c ': PASS$' "${LOG_FILE}" || true)"
            if [[ "${turn_pass_count}" -lt 3 ]]; then
                fail "expected at least 3 turn PASS lines in timed-gap chat regression, got ${turn_pass_count}"
            fi
            echo "QEMU timed-gap chat regression passed" | tee -a "${LOG_FILE}"
            exit 0
        fi
        if grep -Fq "timed-gap-chat-regression: FAILED" "${LOG_FILE}"; then
            fail "timed-gap chat regression aggregate failure reported in captured output"
        fi
        fail "timed-gap chat regression aggregate result not found in captured output"
    fi

    if [[ "${ISO_SMOKE}" -eq 1 ]]; then
        if ! grep -Eq '^net_ip=[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+$' "${LOG_FILE}"; then
            fail "expected net_ip= line not found in captured output"
        fi

        if grep -Fq "${CHAT_HOST} reachable" "${LOG_FILE}"; then
            echo "QEMU ISO smoke test passed (bootstrap + connectivity probe)" | tee -a "${LOG_FILE}"
            exit 0
        fi

        if grep -Eq 'net_ping=.*status=ok' "${LOG_FILE}"; then
            echo "QEMU ISO smoke test passed (bootstrap + connectivity probe)" | tee -a "${LOG_FILE}"
            exit 0
        fi

        if grep -Fq " reachable" "${LOG_FILE}"; then
            echo "QEMU ISO smoke test passed (bootstrap + connectivity probe)" | tee -a "${LOG_FILE}"
            exit 0
        fi

        fail "expected connectivity probe success not found in captured output after ISO boot"
    fi

    if ! grep -Eq '^net_ip=[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+$' "${LOG_FILE}"; then
        fail "expected net_ip= line not found in captured output"
    fi

    if ! grep -Fq "${CHAT_HOST} reachable" "${LOG_FILE}"; then
        fail "expected ${CHAT_HOST} reachable line not found in captured output"
    fi

    if grep -qE '^chat_completion=' "${LOG_FILE}"; then
        fail "old chat_completion= diagnostic framing detected in captured output"
    fi

    if [[ -n "${SCRIPT_FILE}" ]]; then
        if ! chat_script_run_assertions "${LOG_FILE}"; then
            fail "scripted interaction assertions failed"
        fi
        echo "QEMU scripted chat test passed" | tee -a "${LOG_FILE}"
        exit 0
    fi

    echo "run-qemu.sh: chat endpoint ${CHAT_HOST}:${CHAT_PORT} reachable; asserting plain assistant replies" | tee -a "${LOG_FILE}"

    local reply_count
    reply_count="$(count_plain_reply_lines)"
    if [[ "${reply_count}" -lt 2 ]]; then
        fail "expected at least 2 plain assistant reply lines (multi-turn), got ${reply_count}"
    fi
    if grep -q '^chat failed:' "${LOG_FILE}"; then
        fail "chat failed detected during multi-turn session"
    fi

    echo "QEMU smoke test passed" | tee -a "${LOG_FILE}"
}

main "$@"
