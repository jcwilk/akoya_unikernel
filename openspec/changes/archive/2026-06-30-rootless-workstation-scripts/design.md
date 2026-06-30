## Context

`make iso` already runs unprivileged (`grub-mkrescue` + `xorriso`). `make usb` requires root because `build-boot-usb.sh` uses `losetup`, `mount`, and `grub-install` on a loop device — a convenience implementation, not a platform requirement.

`run-qemu.sh` shells out to `sudo -n /usr/local/libexec/akoya/qemu-bridge-{up,down}.sh` for macvtap. Creating macvtap needs `CAP_NET_ADMIN`, not necessarily uid 0, but the current wiring assumes passwordless sudo.

One-time install scripts (`install-bridge-libexec.sh`) copy helpers system-wide; that remains an operator-initiated admin action and is **not** invoked by `make test`, `make run`, or packaging targets.

## Goals / Non-Goals

**Goals:**

- Every `make` target and every script under `scripts/` that is part of build → package → verify → emulate runs entirely as the invoking user.
- USB disk image output unchanged in purpose: `build/akoya-boot.img` for Etcher/legacy BIOS (not the hybrid ISO).
- LAN-attached QEMU smoke tests keep working when workstation one-time setup has been done.

**Non-Goals:**

- Rewriting Linux macvtap to avoid `CAP_NET_ADMIN` entirely.
- Removing the need for admin privilege during **one-time** libexec install on a new workstation (only removing admin from recurring script runs).
- Changing operator-side flashing (`dd`, Etcher) — those tools manage removable devices separately.

## Decisions

### Rootless USB disk image assembly

Replace loop-mount + `grub-install` on `/dev/loopN` with user-space assembly:

1. Stage `boot/kernel.elf` and `boot/grub/grub.cfg` in a temp directory.
2. Build an ext2 partition image with **`genext2fs -d staging`** (no mount).
3. Write an MBR partition table into a fixed-size disk image with **`sfdisk`** on the file (already works unprivileged).
4. **`dd`** the partition image into the partition offset.
5. Install BIOS boot code with **`grub-install --target=i386-pc --boot-directory=<staging> --force <disk.img>`** targeting the **plain disk image file** (supported by GRUB when the boot directory lives on the host staging tree — no loop mount).

**Alternative rejected:** keep loop mount and document `sudo` — contradicts intent.

**Alternative rejected:** `grub-mkstandalone` embedding the full kernel — core image exceeded BIOS size limits during prior apply.

Host dependency adds **`genext2fs`** package (Debian/Ubuntu: `genext2fs`).

### Verification scripts

`verify-boot-usb.sh` calls `build-boot-usb.sh` directly with no `sudo` wrapper.

### QEMU LAN without `sudo` in the runner

After one-time install:

1. `install-bridge-libexec.sh` copies helpers and applies **`setcap cap_net_admin+ep`** to `qemu-bridge-up.sh` and `qemu-bridge-down.sh` (or a tiny wrapper binary).
2. `run-qemu.sh` invokes the libexec path **directly** — remove all `sudo` / `sudo -n` calls.
3. If the helper lacks capability or macvtap cannot be created, fail fast with actionable output pointing at the one-time install command (admin runs install once; not part of every test).

**Alternative rejected:** passwordless sudoers from runner — still privilege elevation inside scripts.

Drop or de-emphasize sudoers example in favor of setcap in install script; keep sudoers as optional legacy note in README appendix if needed.

### Error message hygiene

Scripts suggest `apt install` without prefixing `sudo` on the command the script itself runs. Install commands in logs may mention "install package X" without implying the build script will run as root.

## Risks / Trade-offs

| Risk | Mitigation |
|------|------------|
| `grub-install` to plain file behaves differently across GRUB versions | Pin tested distro note; verify with `make verify-usb` in apply |
| `setcap` blocked by filesystem `nosuid` on `/usr/local` | Document install path requirements; fail fast with clear error |
| genext2fs not installed | Fail fast like other missing deps |

## Migration Plan

1. Refactor `build-boot-usb.sh` (remove `require_root`, add genext2fs path).
2. Fix `verify-boot-usb.sh`, README, Makefile help text.
3. Update bridge install + `run-qemu.sh` to drop sudo.
4. Run `make usb`, `make verify-usb`, `make test` as unprivileged user in apply evidence.

## Open Questions

- None blocking — setcap vs sudoers for bridge helpers resolves at apply time with install script update.
