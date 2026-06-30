#!/usr/bin/env bash
set -euo pipefail

# Builds a BIOS/Legacy USB/HDD raw disk image (MBR + GRUB BIOS boot).
# The kernel is stored at a fixed raw LBA (1 MiB) so GRUB never reads ext2 on
# legacy BIOS USB (avoids "attempt to read or write outside of disk 'hd0'").
# Use this for flash drives — NOT the ISO (hybrid ISO9660 images fail similarly).

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"
KERNEL_BIN="${BUILD_DIR}/kernel.bin"
KERNEL_ELF="${BUILD_DIR}/kernel.elf"
IMG_PATH="${BUILD_DIR}/akoya-boot.img"
ETCHER_LINK="${BUILD_DIR}/akoya-etcher.img"
LOG_FILE="${BUILD_DIR}/usb-build.log"
IMG_SIZE_MB="${AKOYA_USB_IMAGE_SIZE_MB:-64}"

GRUB_MKIMAGE="${AKOYA_GRUB_MKIMAGE:-grub-mkimage}"
GRUB_INSTALL="${AKOYA_GRUB_INSTALL:-grub-install}"
GENEXT2FS="${AKOYA_GENEXT2FS:-genext2fs}"
GRUB_PLATFORM_DIR="${AKOYA_GRUB_PLATFORM_DIR:-/usr/lib/grub/i386-pc}"

PART_OFFSET_BYTES=$((1024 * 1024))
KERNEL_LBA=$((PART_OFFSET_BYTES / 512))

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

    log "Kernel missing; running scripts/build.sh"
    if ! bash "${ROOT_DIR}/scripts/build.sh"; then
        fail "kernel build failed"
    fi
    if [[ ! -f "${KERNEL_BIN}" || ! -f "${KERNEL_ELF}" ]]; then
        fail "kernel build did not produce ${KERNEL_BIN} and ${KERNEL_ELF}"
    fi
}

write_grub_cfg() {
    local cfg_path="$1"
    local build_id="$2"
    cat > "${cfg_path}" <<EOF
set timeout=5
set default=0
terminal_input console
terminal_output console
if serial --unit=0 --speed=115200 --word=8 --parity=no --stop=1; then
    terminal_input --append serial
    terminal_output --append serial
fi

menuentry "Akoya Unikernel ${build_id}" {
    multiboot /boot/kernel.elf
    boot
}
EOF
}

write_grub_early_cfg() {
    local cfg_path="$1"
    local kernel_sectors="$2"
    cat > "${cfg_path}" <<EOF
terminal_input console
terminal_output console
set timeout=5
menuentry "Akoya Unikernel" {
    multiboot (hd0)${KERNEL_LBA}+${kernel_sectors}
    boot
}
EOF
}

install_grub_with_mkimage() {
    local disk_img="$1"
    local early_cfg="$2"

    if ! command -v "${GRUB_MKIMAGE}" >/dev/null 2>&1; then
        fail "grub-mkimage not found; install grub-pc-bin (apt install grub-pc-bin)"
    fi
    if [[ ! -f "${GRUB_PLATFORM_DIR}/boot.img" ]]; then
        fail "GRUB i386-pc boot.img not found under ${GRUB_PLATFORM_DIR}; install grub-pc-bin"
    fi

    local core_img
    core_img="$(mktemp /tmp/akoya-usb-core.XXXXXX.img)"

    log "Embedding GRUB core (raw LBA ${KERNEL_LBA}+N multiboot, no ext2 read) for ${disk_img}"
    "${GRUB_MKIMAGE}" \
        -c "${early_cfg}" \
        -d "${GRUB_PLATFORM_DIR}" \
        -O i386-pc \
        -o "${core_img}" \
        -p "(hd0)" \
        biosdisk boot multiboot serial normal >>"${LOG_FILE}" 2>&1

    dd if="${GRUB_PLATFORM_DIR}/boot.img" of="${disk_img}" conv=notrunc bs=440 count=1 status=none >>"${LOG_FILE}" 2>&1
    dd if="${core_img}" of="${disk_img}" conv=notrunc bs=512 seek=1 status=none >>"${LOG_FILE}" 2>&1
    rm -f "${core_img}"
}

install_grub_to_disk_image() {
    local disk_img="$1"
    local early_cfg="$2"

    install_grub_with_mkimage "${disk_img}" "${early_cfg}"
}

main() {
    mkdir -p "${BUILD_DIR}"

    if ! command -v "${GENEXT2FS}" >/dev/null 2>&1; then
        fail "genext2fs not found; install genext2fs (apt install genext2fs)"
    fi
    if ! command -v sfdisk >/dev/null 2>&1; then
        fail "sfdisk required (util-linux package; apt install util-linux)"
    fi
    if ! command -v dd >/dev/null 2>&1; then
        fail "dd required (coreutils package)"
    fi

    : > "${LOG_FILE}"

    ensure_kernel

    local build_id staging_dir part_img part_size_bytes early_cfg kernel_sectors
    build_id="$(resolve_build_id)"
    part_size_bytes=$((IMG_SIZE_MB * 1024 * 1024 - PART_OFFSET_BYTES))
    kernel_sectors=$(( ($(stat -c%s "${KERNEL_ELF}") + 511) / 512 ))

    staging_dir="$(mktemp -d /tmp/akoya-usb-staging.XXXXXX)"
    part_img="$(mktemp /tmp/akoya-usb-part.XXXXXX.img)"
    early_cfg="$(mktemp /tmp/akoya-usb-early.XXXXXX.cfg)"
    trap 'rm -rf "${staging_dir:-}" "${part_img:-}" "${early_cfg:-}"' EXIT

    mkdir -p "${staging_dir}/boot/grub"
    cp "${KERNEL_ELF}" "${staging_dir}/boot/kernel.elf"
    write_grub_cfg "${staging_dir}/boot/grub/grub.cfg" "${build_id}"
    write_grub_early_cfg "${early_cfg}" "${kernel_sectors}"

    log "Building ext2 partition image (${part_size_bytes} bytes) with genext2fs"
    "${GENEXT2FS}" \
        -L "AKOYA-${build_id:0:24}" \
        -b $((part_size_bytes / 1024)) \
        -d "${staging_dir}" \
        "${part_img}" >>"${LOG_FILE}" 2>&1

    log "Creating ${IMG_SIZE_MB} MiB disk image: ${IMG_PATH}"
    truncate -s "${IMG_SIZE_MB}M" "${IMG_PATH}"

    printf 'label: dos\nstart=1MiB, type=83, bootable\n' | sfdisk "${IMG_PATH}" >>"${LOG_FILE}" 2>&1

    log "Writing partition into disk image at 1 MiB offset"
    dd if="${part_img}" of="${IMG_PATH}" bs=1M seek=1 conv=notrunc status=none >>"${LOG_FILE}" 2>&1

    log "Writing kernel.elf at raw LBA ${KERNEL_LBA} (${kernel_sectors} sectors) for legacy BIOS USB"
    dd if="${KERNEL_ELF}" of="${IMG_PATH}" bs=512 seek="${KERNEL_LBA}" conv=notrunc status=none >>"${LOG_FILE}" 2>&1

    install_grub_to_disk_image "${IMG_PATH}" "${early_cfg}"

    ln -sf "$(basename "${IMG_PATH}")" "${ETCHER_LINK}"

    log "USB/HDD image ready: ${IMG_PATH} (${IMG_SIZE_MB} MiB MBR + raw kernel @ LBA ${KERNEL_LBA} + GRUB)"
    log "Etcher: flash ${ETCHER_LINK} or ${IMG_PATH} (Balena Etcher → Flash from file → select the .img)"
    log "Do not flash build/akoya-boot.iso on legacy BIOS USB — use this .img instead (ISO is optical/QEMU only)."
    log "Alternative: dd if=${IMG_PATH} of=/dev/sdX bs=4M status=progress conv=fsync (requires access to the USB block device)"
    emit_result "success" "usb-image-ready;etcher=${ETCHER_LINK}"
}

main "$@"
