#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
if [[ -d "${ROOT_DIR}/tools/bin" ]]; then
    export PATH="${ROOT_DIR}/tools/bin:${PATH}"
fi
BUILD_DIR="${ROOT_DIR}/build"
LOG_FILE="${BUILD_DIR}/build.log"
KERNEL_BIN="${BUILD_DIR}/kernel.bin"
TRANSPORT_BIN="${BUILD_DIR}/transport-test.bin"
CHAT_REGRESSION_BIN="${BUILD_DIR}/chat-regression-test.bin"
PROFILE_FILE="${ROOT_DIR}/target/akoya.profile"

CC_PREFIX="${AKOYA_CROSS_PREFIX:-i686-elf-}"
CC="${CC_PREFIX}gcc"
LD="${CC_PREFIX}ld"
OBJCOPY="${CC_PREFIX}objcopy"

if ! command -v "${CC}" >/dev/null 2>&1 && [[ -z "${AKOYA_CROSS_PREFIX:-}" ]] && command -v gcc >/dev/null 2>&1; then
    CC_PREFIX=""
    CC="gcc"
    LD="ld"
    OBJCOPY="objcopy"
fi

MEM_LIMIT_MB="${AKOYA_BUILD_MEM_LIMIT_MB:-4096}"
MEM_LIMIT_KB=$((MEM_LIMIT_MB * 1024))
MEM_LIMIT_BYTES=$((MEM_LIMIT_MB * 1024 * 1024))
PROBE_HOST="${AKOYA_PROBE_HOST:-google.com}"
CHAT_HOST_IP="${AKOYA_CHAT_HOST_IP:-192.168.1.110}"
CHAT_PATH="${AKOYA_CHAT_PATH:-/v1/chat/completions}"
CHAT_USER_MSG="${AKOYA_CHAT_USER_MSG:-hi}"
CHAT_MODEL="${AKOYA_CHAT_MODEL:-fast-text-qwen3-8b}"
CHAT_TIMEOUT_MS="${AKOYA_CHAT_TIMEOUT_MS:-60000}"
CHAT_PORT="${AKOYA_CHAT_PORT:-11435}"
CHAT_MAX_TOKENS="${AKOYA_CHAT_MAX_TOKENS:-500}"
CHAT_REGRESSION_TURNS="${AKOYA_CHAT_REGRESSION_TURNS:-3}"
CHAT_REGRESSION_GAP_MS="${AKOYA_CHAT_REGRESSION_GAP_MS:-5000}"

log() {
    printf '%s\n' "$*" | tee -a "${LOG_FILE}"
}

emit_result() {
    local status="$1"
    local message="$2"
    printf 'AKOYA_BUILD_RESULT=status=%s;kernel=%s;transport=%s;chat_regression=%s;log=%s;message=%s\n' \
        "${status}" "${KERNEL_BIN}" "${TRANSPORT_BIN}" "${CHAT_REGRESSION_BIN}" "${LOG_FILE}" "${message}"
}

resolve_build_id() {
    if command -v git >/dev/null 2>&1 && git -C "${ROOT_DIR}" rev-parse --is-inside-work-tree >/dev/null 2>&1; then
        if git -C "${ROOT_DIR}" describe --tags --always --dirty 2>/dev/null; then
            return 0
        fi
        git -C "${ROOT_DIR}" rev-parse --short HEAD
        return 0
    fi

    date -u +%Y%m%dT%H%M%SZ
}

resolve_probe_target() {
    local ip=""
    if command -v getent >/dev/null 2>&1; then
        ip="$(getent ahostsv4 "${PROBE_HOST}" 2>/dev/null | awk 'NR==1 {print $1; exit}')"
    fi
    if [[ -z "${ip}" ]] && command -v python3 >/dev/null 2>&1; then
        ip="$(python3 - <<PY
import socket
try:
    print(socket.gethostbyname("${PROBE_HOST}"))
except OSError:
    pass
PY
)"
    fi
    if [[ -z "${ip}" ]]; then
        ip="8.8.8.8"
    fi
    printf '%s' "${ip}"
}

toolchain_available() {
    command -v "${CC}" >/dev/null 2>&1 && command -v "${LD}" >/dev/null 2>&1
}

run_with_memory_limit() {
    if command -v prlimit >/dev/null 2>&1; then
        prlimit --as="${MEM_LIMIT_BYTES}" -- "$@"
    elif command -v ulimit >/dev/null 2>&1; then
        (
            ulimit -v "${MEM_LIMIT_KB}" || true
            "$@"
        )
    else
        "$@"
    fi
}

compile_source() {
    local source_path="$1"
    local object_path="$2"
    if ! run_with_memory_limit "${CC}" "${cflags[@]}" -c "${source_path}" -o "${object_path}" 2>&1 | tee -a "${LOG_FILE}"; then
        emit_result "failure" "compile-error"
        exit 1
    fi
}

link_image() {
    local elf_path="$1"
    local bin_path="$2"
    shift 2
    local -a objects=("$@")

    log "Linking ${elf_path}"
    if ! run_with_memory_limit "${LD}" -m elf_i386 -T "${ROOT_DIR}/linker.ld" -o "${elf_path}" "${objects[@]}" 2>&1 | tee -a "${LOG_FILE}"; then
        emit_result "failure" "link-error"
        exit 1
    fi

    if ! run_with_memory_limit "${OBJCOPY}" -O binary "${elf_path}" "${bin_path}" 2>&1 | tee -a "${LOG_FILE}"; then
        emit_result "failure" "objcopy-error"
        exit 1
    fi

    log "Build succeeded: ${bin_path}"
}

main() {
    mkdir -p "${BUILD_DIR}"
    : > "${LOG_FILE}"

    if ! toolchain_available; then
        log "ERROR: required cross toolchain not found (${CC}, ${LD})"
        log "Install i686-elf-gcc and i686-elf-binutils, set AKOYA_CROSS_PREFIX, or use tools/bin in this repo"
        emit_result "failure" "missing-toolchain"
        exit 1
    fi

    if [[ -f "${PROFILE_FILE}" ]]; then
        # shellcheck disable=SC1090
        source "${PROFILE_FILE}"
        log "Target profile: ${AKOYA_TARGET_NAME:-unknown} (${AKOYA_ARCH:-i686})"
    fi

    local build_id
    build_id="$(resolve_build_id)"
    log "Build identity: ${build_id}"

    local probe_ip probe_label
    probe_ip="$(resolve_probe_target)"
    probe_label="${PROBE_HOST}"
    log "Probe target: ${probe_label} -> ${probe_ip}"

    IFS='.' read -r probe_ip0 probe_ip1 probe_ip2 probe_ip3 <<< "${probe_ip}"

    local chat_ip0 chat_ip1 chat_ip2 chat_ip3
    IFS='.' read -r chat_ip0 chat_ip1 chat_ip2 chat_ip3 <<< "${CHAT_HOST_IP}"

    local cflags=(
        -std=gnu99
        -ffreestanding
        -fno-stack-protector
        -fno-pic
        -fno-pie
        -m32
        -march=pentium3
        -mtune=pentium-m
        -Wall
        -Wextra
        -O2
        -I"${ROOT_DIR}/kernel"
        -DAKOYA_BUILD_ID=\"${build_id}\"
        -DAKOYA_PROBE_TARGET_IP0="${probe_ip0}"
        -DAKOYA_PROBE_TARGET_IP1="${probe_ip1}"
        -DAKOYA_PROBE_TARGET_IP2="${probe_ip2}"
        -DAKOYA_PROBE_TARGET_IP3="${probe_ip3}"
        -DAKOYA_PROBE_TARGET_LABEL=\"${probe_label}\"
        -DAKOYA_CHAT_HOST_IP0="${chat_ip0}"
        -DAKOYA_CHAT_HOST_IP1="${chat_ip1}"
        -DAKOYA_CHAT_HOST_IP2="${chat_ip2}"
        -DAKOYA_CHAT_HOST_IP3="${chat_ip3}"
        -DAKOYA_CHAT_PATH=\"${CHAT_PATH}\"
        -DAKOYA_CHAT_USER_MSG=\"${CHAT_USER_MSG}\"
        -DAKOYA_CHAT_MODEL=\"${CHAT_MODEL}\"
        -DAKOYA_CHAT_TIMEOUT_MS="${CHAT_TIMEOUT_MS}"
        -DAKOYA_CHAT_PORT="${CHAT_PORT}"
        -DAKOYA_CHAT_MAX_TOKENS="${CHAT_MAX_TOKENS}"
        -DAKOYA_CHAT_REGRESSION_TURNS="${CHAT_REGRESSION_TURNS}"
        -DAKOYA_CHAT_REGRESSION_GAP_MS="${CHAT_REGRESSION_GAP_MS}U"
    )

    local shared_sources=(
        "${ROOT_DIR}/kernel/boot/entry.S"
        "${ROOT_DIR}/kernel/arch/isr.S"
        "${ROOT_DIR}/kernel/arch/idt.c"
        "${ROOT_DIR}/kernel/arch/pic.c"
        "${ROOT_DIR}/kernel/console/console.c"
        "${ROOT_DIR}/kernel/event/device.c"
        "${ROOT_DIR}/kernel/event/runtime.c"
        "${ROOT_DIR}/kernel/pci/pci.c"
        "${ROOT_DIR}/kernel/time/time.c"
        "${ROOT_DIR}/kernel/time/timer.c"
        "${ROOT_DIR}/kernel/net/eth/eth.c"
        "${ROOT_DIR}/kernel/net/eth/rtl8139.c"
        "${ROOT_DIR}/kernel/net/eth/nic_device.c"
        "${ROOT_DIR}/kernel/net/link/link.c"
        "${ROOT_DIR}/kernel/net/ipv4/ipv4.c"
        "${ROOT_DIR}/kernel/net/dhcp/dhcp.c"
        "${ROOT_DIR}/kernel/net/icmp/icmp.c"
        "${ROOT_DIR}/kernel/net/tcp/tcp.c"
    )

    local kernel_only_sources=(
        "${ROOT_DIR}/kernel/input/ps2_keyboard.c"
        "${ROOT_DIR}/kernel/input/ps2_readline.c"
        "${ROOT_DIR}/kernel/net/http/http_chat.c"
        "${ROOT_DIR}/kernel/net/net_async.c"
        "${ROOT_DIR}/kernel/main.c"
    )

    local transport_only_sources=(
        "${ROOT_DIR}/kernel/net/transport_test_main.c"
        "${ROOT_DIR}/kernel/transport_main.c"
    )

    local chat_regression_only_sources=(
        "${ROOT_DIR}/kernel/net/http/http_chat.c"
        "${ROOT_DIR}/kernel/net/net_async.c"
        "${ROOT_DIR}/kernel/input/ps2_keyboard.c"
        "${ROOT_DIR}/kernel/input/ps2_readline.c"
        "${ROOT_DIR}/kernel/chat_regression_entry.c"
    )

    local shared_objects=()
    local kernel_objects=()
    local transport_objects=()
    local chat_regression_objects=()
    local source object base

    log "Compiling with ${CC} (memory limit ${MEM_LIMIT_MB} MB)"
    for source in "${shared_sources[@]}"; do
        base="$(basename "${source}")"
        base="${base%.*}"
        object="${BUILD_DIR}/shared-${base}.o"
        shared_objects+=("${object}")
        compile_source "${source}" "${object}"
    done

    for source in "${kernel_only_sources[@]}"; do
        base="$(basename "${source}")"
        base="${base%.*}"
        object="${BUILD_DIR}/kernel-${base}.o"
        kernel_objects+=("${object}")
        compile_source "${source}" "${object}"
    done

    for source in "${transport_only_sources[@]}"; do
        base="$(basename "${source}")"
        base="${base%.*}"
        object="${BUILD_DIR}/transport-${base}.o"
        transport_objects+=("${object}")
        compile_source "${source}" "${object}"
    done

    for source in "${chat_regression_only_sources[@]}"; do
        base="$(basename "${source}")"
        base="${base%.*}"
        object="${BUILD_DIR}/chat-regression-${base}.o"
        chat_regression_objects+=("${object}")
        compile_source "${source}" "${object}"
    done

    link_image "${BUILD_DIR}/kernel.elf" "${KERNEL_BIN}" \
        "${shared_objects[@]}" "${kernel_objects[@]}"

    link_image "${BUILD_DIR}/transport-test.elf" "${TRANSPORT_BIN}" \
        "${shared_objects[@]}" "${transport_objects[@]}"

    link_image "${BUILD_DIR}/chat-regression-test.elf" "${CHAT_REGRESSION_BIN}" \
        "${shared_objects[@]}" "${chat_regression_objects[@]}"

    emit_result "success" "build-complete"
}

main "$@"
