#!/usr/bin/env bash
set -euo pipefail

# Builds a BIOS/Legacy USB/HDD raw disk image (MBR + ext2 + grub-install).
# Use this for flash drives — NOT the ISO (hybrid ISO9660 images often fail on
# legacy BIOS with "attempt to read or write outside of disk 'hd0'").

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"
KERNEL_BIN="${BUILD_DIR}/kernel.bin"
KERNEL_ELF="${BUILD_DIR}/kernel.elf"
IMG_PATH="${BUILD_DIR}/akoya-boot.img"
LOG_FILE="${BUILD_DIR}/usb-build.log"
IMG_SIZE_MB="${AKOYA_USB_IMAGE_SIZE_MB:-64}"

GRUB_INSTALL="${AKOYA_GRUB_INSTALL:-grub-install}"

emit_result() {
    local status="$1"
    local message="$2"
    printf 'AKOYA_USB_RESULT=status=%s;img=%s;log=%s;message=%s\n' \
        "${status}" "${IMG_PATH}" "${LOG_FILE}" "${message}"
}

log() {
    printf '%s\n' "$*" | tee -a "${LOG_FILE}"
}

fail() {
    log "ERROR: $*"
    emit_result "failure" "$*"
    exit 1
}

require_root() {
    if [[ $EUID -ne 0 ]]; then
        echo "build-boot-usb.sh: requires root for loop mount and grub-install" >&2
        echo "" >&2
        echo "  sudo make usb" >&2
        echo "  sudo bash scripts/build-boot-usb.sh" >&2
        echo "" >&2
        echo "Do not use 'make iso' output for USB sticks on legacy BIOS — use this image." >&2
        exit 1
    fi
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

ensure_kernel() {
    if [[ -f "${KERNEL_BIN}" && -f "${KERNEL_ELF}" ]]; then
        log "Using existing kernel: ${KERNEL_ELF}"
        return 0
    fi

    log "Kernel missing; running scripts/build.sh as ${SUDO_USER:-root}"
    local -a build_cmd=(bash "${ROOT_DIR}/scripts/build.sh")
    if [[ -n "${SUDO_USER:-}" ]]; then
        build_cmd=(sudo -u "${SUDO_USER}" -H bash "${ROOT_DIR}/scripts/build.sh")
    fi
    if ! "${build_cmd[@]}"; then
        fail "kernel build failed"
    fi
    if [[ ! -f "${KERNEL_BIN}" || ! -f "${KERNEL_ELF}" ]]; then
        fail "kernel build did not produce ${KERNEL_BIN} and ${KERNEL_ELF}"
    fi
}

restore_ownership() {
    if [[ -n "${SUDO_USER:-}" ]]; then
        chown "${SUDO_USER}:${SUDO_USER}" "${IMG_PATH}" "${LOG_FILE}" 2>/dev/null || true
        chown -R "${SUDO_USER}:${SUDO_USER}" "${BUILD_DIR}" 2>/dev/null || true
    fi
}

write_grub_cfg() {
    local cfg_path="$1"
    local build_id="$2"
    cat > "${cfg_path}" <<EOF
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
}

main() {
    require_root

    if ! command -v "${GRUB_INSTALL}" >/dev/null 2>&1; then
        fail "grub-install not found; install grub-pc-bin (sudo apt install grub-pc-bin)"
    fi
    if ! command -v losetup >/dev/null 2>&1 || ! command -v mkfs.ext2 >/dev/null 2>&1; then
        fail "losetup and mkfs.ext2 required (e2fsprogs package)"
    fi
    if ! command -v sfdisk >/dev/null 2>&1; then
        fail "sfdisk required (util-linux package)"
    fi

    mkdir -p "${BUILD_DIR}"
    : > "${LOG_FILE}"

    ensure_kernel

    local build_id mount_dir loop_dev part_dev
    build_id="$(resolve_build_id)"

    mount_dir="$(mktemp -d /tmp/akoya-usb-mount.XXXXXX)"
    trap 'umount -f "${mount_dir}" 2>/dev/null || true; losetup -d "${loop_dev:-}" 2>/dev/null || true; rm -rf "${mount_dir}"; restore_ownership' EXIT

    log "Creating ${IMG_SIZE_MB} MiB disk image: ${IMG_PATH}"
    truncate -s "${IMG_SIZE_MB}M" "${IMG_PATH}"

    printf 'label: dos\nstart=1MiB, type=83, bootable\n' | sfdisk "${IMG_PATH}" >>"${LOG_FILE}" 2>&1

    loop_dev="$(losetup -fP --show "${IMG_PATH}")"
    part_dev="${loop_dev}p1"
    if [[ ! -e "${part_dev}" ]]; then
        part_dev="${loop_dev}1"
    fi
    if [[ ! -e "${part_dev}" ]]; then
        fail "partition device not found after losetup (tried ${loop_dev}p1 and ${loop_dev}1)"
    fi

    log "Formatting ext2 on ${part_dev}"
    mkfs.ext2 -F -L "AKOYA-${build_id:0:24}" "${part_dev}" >>"${LOG_FILE}" 2>&1

    mount "${part_dev}" "${mount_dir}"
    mkdir -p "${mount_dir}/boot/grub"
    cp "${KERNEL_ELF}" "${mount_dir}/boot/kernel.elf"
    write_grub_cfg "${mount_dir}/boot/grub/grub.cfg" "${build_id}"

    log "Installing GRUB to MBR on ${loop_dev}"
    "${GRUB_INSTALL}" \
        --target=i386-pc \
        --boot-directory="${mount_dir}/boot" \
        --modules="multiboot serial normal configfile part_msdos ext2 biosdisk" \
        --force \
        "${loop_dev}" >>"${LOG_FILE}" 2>&1

    umount "${mount_dir}"
    losetup -d "${loop_dev}"
    loop_dev=""
    rm -rf "${mount_dir}"
    mount_dir=""

    restore_ownership

    log "USB/HDD image ready: ${IMG_PATH} (${IMG_SIZE_MB} MiB MBR + ext2 + GRUB)"
    log "Write to whole USB device: sudo dd if=${IMG_PATH} of=/dev/sdX bs=4M status=progress conv=fsync"
    emit_result "success" "usb-image-ready"
}

main "$@"
