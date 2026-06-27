#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
if [[ -d "${ROOT_DIR}/tools/bin" ]]; then
    export PATH="${ROOT_DIR}/tools/bin:${PATH}"
fi
BUILD_DIR="${ROOT_DIR}/build"
LOG_FILE="${BUILD_DIR}/build.log"
KERNEL_BIN="${BUILD_DIR}/kernel.bin"
PROFILE_FILE="${ROOT_DIR}/target/akoya.profile"

CC_PREFIX="${AKOYA_CROSS_PREFIX:-i686-elf-}"
CC="${CC_PREFIX}gcc"
LD="${CC_PREFIX}ld"
OBJCOPY="${CC_PREFIX}objcopy"

MEM_LIMIT_MB="${AKOYA_BUILD_MEM_LIMIT_MB:-4096}"
MEM_LIMIT_KB=$((MEM_LIMIT_MB * 1024))
MEM_LIMIT_BYTES=$((MEM_LIMIT_MB * 1024 * 1024))

DOCKER_IMAGE="${AKOYA_BUILD_DOCKER_IMAGE:-qsrahmans/i686-elf-gcc:dev}"

log() {
    printf '%s\n' "$*" | tee -a "${LOG_FILE}"
}

emit_result() {
    local status="$1"
    local message="$2"
  printf 'AKOYA_BUILD_RESULT=status=%s;kernel=%s;log=%s;message=%s\n' \
        "${status}" "${KERNEL_BIN}" "${LOG_FILE}" "${message}"
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

run_build_in_docker() {
    docker run --rm \
        -v "${ROOT_DIR}:/work" \
        -w /work \
        -e AKOYA_CROSS_PREFIX="${CC_PREFIX}" \
        -e AKOYA_BUILD_MEM_LIMIT_MB="${MEM_LIMIT_MB}" \
        -e AKOYA_BUILD_USE_DOCKER=0 \
        "${DOCKER_IMAGE}" \
        bash scripts/build.sh
}

main() {
    mkdir -p "${BUILD_DIR}"
    : > "${LOG_FILE}"

    if [[ "${AKOYA_BUILD_USE_DOCKER:-1}" == "1" ]] && ! toolchain_available; then
        log "Local ${CC} not found; using Docker image ${DOCKER_IMAGE}"
        if ! command -v docker >/dev/null 2>&1; then
            log "ERROR: cross compiler missing and docker unavailable"
            log "Install i686-elf-gcc/binutils or qemu-system-i386 prerequisites documented in README.md"
            emit_result "failure" "missing-toolchain"
            exit 1
        fi
        run_build_in_docker
        exit $?
    fi

    if ! toolchain_available; then
        log "ERROR: required cross toolchain not found (${CC}, ${LD})"
        log "Install i686-elf-gcc and i686-elf-binutils, or set AKOYA_CROSS_PREFIX"
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
    )

    local objects=(
        "${BUILD_DIR}/entry.o"
        "${BUILD_DIR}/console.o"
        "${BUILD_DIR}/main.o"
    )

    log "Compiling with ${CC} (memory limit ${MEM_LIMIT_MB} MB)"
    if ! run_with_memory_limit "${CC}" "${cflags[@]}" -c "${ROOT_DIR}/kernel/boot/entry.S" -o "${BUILD_DIR}/entry.o" 2>&1 | tee -a "${LOG_FILE}"; then
        emit_result "failure" "compile-error"
        exit 1
    fi
    if ! run_with_memory_limit "${CC}" "${cflags[@]}" -c "${ROOT_DIR}/kernel/console/console.c" -o "${BUILD_DIR}/console.o" 2>&1 | tee -a "${LOG_FILE}"; then
        emit_result "failure" "compile-error"
        exit 1
    fi
    if ! run_with_memory_limit "${CC}" "${cflags[@]}" -c "${ROOT_DIR}/kernel/main.c" -o "${BUILD_DIR}/main.o" 2>&1 | tee -a "${LOG_FILE}"; then
        emit_result "failure" "compile-error"
        exit 1
    fi

    log "Linking ${KERNEL_BIN}"
    if ! run_with_memory_limit "${LD}" -m elf_i386 -T "${ROOT_DIR}/linker.ld" -o "${BUILD_DIR}/kernel.elf" "${objects[@]}" 2>&1 | tee -a "${LOG_FILE}"; then
        emit_result "failure" "link-error"
        exit 1
    fi

    if ! run_with_memory_limit "${OBJCOPY}" -O binary "${BUILD_DIR}/kernel.elf" "${KERNEL_BIN}" 2>&1 | tee -a "${LOG_FILE}"; then
        emit_result "failure" "objcopy-error"
        exit 1
    fi

    log "Build succeeded: ${KERNEL_BIN}"
    emit_result "success" "build-complete"
}

main "$@"
