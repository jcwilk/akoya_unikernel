#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"
KERNEL_BIN="${BUILD_DIR}/kernel.bin"
KERNEL_ELF="${BUILD_DIR}/kernel.elf"
ISO_PATH="${BUILD_DIR}/akoya-boot.iso"
LOG_FILE="${BUILD_DIR}/iso-build.log"

GRUB_MKRESCUE="${AKOYA_GRUB_MKRESCUE:-grub-mkrescue}"
XORRISO="${AKOYA_XORRISO_BIN:-xorriso}"

emit_result() {
    local status="$1"
    local message="$2"
    printf 'AKOYA_ISO_RESULT=status=%s;iso=%s;log=%s;message=%s\n' \
        "${status}" "${ISO_PATH}" "${LOG_FILE}" "${message}"
}

log() {
    printf '%s\n' "$*" | tee -a "${LOG_FILE}"
}

fail() {
    log "ERROR: $*"
    emit_result "failure" "$*"
    exit 1
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

suggest_package() {
    local pkg="$1"
    if command -v apt-get >/dev/null 2>&1; then
        printf 'sudo apt install %s' "${pkg}"
    else
        printf 'install %s using your distribution package manager' "${pkg}"
    fi
}

check_host_dependencies() {
    local missing=()
    local tool pkg

    while [[ $# -gt 0 ]]; do
        tool="$1"
        pkg="$2"
        shift 2
        if ! command -v "${tool}" >/dev/null 2>&1; then
            missing+=("${tool} (install package: ${pkg})")
        fi
    done

    if [[ ${#missing[@]} -eq 0 ]]; then
        return 0
    fi

    log "ERROR: missing ISO packaging dependencies:"
    local item
    for item in "${missing[@]}"; do
        log "  - ${item}"
    done
    log ""
    log "Install on Debian/Ubuntu:"
    log "  sudo apt install grub-pc-bin xorriso"
    log ""
    log "Then re-run:"
    log "  make iso"

    emit_result "failure" "missing ISO packaging dependencies; run: sudo apt install grub-pc-bin xorriso"
    exit 1
}

resolve_grub_mod_dir() {
    local candidates=(
        "${AKOYA_GRUB_I386_PC_DIR:-}"
        "/usr/lib/grub/i386-pc"
        "/usr/lib/grub-pc"
    )
    local dir
    for dir in "${candidates[@]}"; do
        [[ -n "${dir}" && -f "${dir}/multiboot.mod" ]] || continue
        printf '%s' "${dir}"
        return 0
    done
    return 1
}

ensure_kernel() {
    if [[ -f "${KERNEL_BIN}" ]]; then
        log "Using existing kernel: ${KERNEL_BIN}"
        return 0
    fi

    log "Kernel not found at ${KERNEL_BIN}; running scripts/build.sh"
    if ! bash "${ROOT_DIR}/scripts/build.sh"; then
        fail "kernel build failed"
    fi
    if [[ ! -f "${KERNEL_BIN}" || ! -f "${KERNEL_ELF}" ]]; then
        fail "kernel build did not produce ${KERNEL_BIN} and ${KERNEL_ELF}"
    fi
}

main() {
    if [[ "${1:-}" == "--check-deps-only" ]]; then
        mkdir -p "${BUILD_DIR}"
        : > "${LOG_FILE}"
        check_host_dependencies \
            "${GRUB_MKRESCUE}" "grub-pc-bin" \
            "${XORRISO}" "xorriso"
        local grub_mod_dir
        if ! grub_mod_dir="$(resolve_grub_mod_dir)"; then
            fail "GRUB i386-pc modules not found (missing multiboot.mod); $(suggest_package "grub-pc-bin")"
        fi
        log "ISO packaging dependencies OK (GRUB modules: ${grub_mod_dir})"
        exit 0
    fi

    mkdir -p "${BUILD_DIR}"
    : > "${LOG_FILE}"

    check_host_dependencies \
        "${GRUB_MKRESCUE}" "grub-pc-bin" \
        "${XORRISO}" "xorriso"

    local grub_mod_dir
    if ! grub_mod_dir="$(resolve_grub_mod_dir)"; then
        fail "GRUB i386-pc modules not found (missing multiboot.mod); $(suggest_package "grub-pc-bin")"
    fi
    log "GRUB i386-pc modules: ${grub_mod_dir}"

    ensure_kernel

    local build_id volume_label staging_dir
    build_id="$(resolve_build_id)"
    volume_label="AKOYA-${build_id}"
  # ISO 9660 volume identifiers are limited to 32 characters.
    volume_label="${volume_label:0:32}"

    staging_dir="$(mktemp -d "${BUILD_DIR}/iso-staging.XXXXXX")"
    trap 'rm -rf "${AKOYA_ISO_STAGING_DIR:-}"' EXIT
    AKOYA_ISO_STAGING_DIR="${staging_dir}"

    mkdir -p "${staging_dir}/boot/grub"
    cp "${KERNEL_ELF}" "${staging_dir}/boot/kernel.elf"

    cat > "${staging_dir}/boot/grub/grub.cfg" <<EOF
set timeout=0
set default=0
serial --speed=115200 --unit=0 --word=8 --parity=no --stop=1
terminal_input serial console
terminal_output serial console

menuentry "Akoya Unikernel ${build_id}" {
    multiboot /boot/kernel.elf
    boot
}
EOF

    log "Staging ISO tree at ${staging_dir}"
    log "Build identity: ${build_id}"
    log "Volume label: ${volume_label}"

    if ! "${GRUB_MKRESCUE}" \
        --directory="${grub_mod_dir}" \
        --compress=xz \
        -o "${ISO_PATH}" \
        -V "${volume_label}" \
        "${staging_dir}" 2>&1 | tee -a "${LOG_FILE}"; then
        fail "grub-mkrescue failed"
    fi

    if [[ ! -f "${ISO_PATH}" ]]; then
        fail "ISO artifact missing after grub-mkrescue"
    fi

    log "ISO packaging succeeded: ${ISO_PATH}"
    emit_result "success" "iso-ready"
}

main "$@"
