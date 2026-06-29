## Required for this change

## 1. Rootless USB disk image packaging

- [x] 1.1 Refactor `scripts/build-boot-usb.sh`: remove `require_root`, loop mount, and mount-based `grub-install`; use user-space ext2 assembly and GRUB installation into a plain disk image file
- [x] 1.2 Add fail-fast when `genext2fs` (or chosen rootless tooling) is missing; error output names the package to install without instructing `sudo make usb`
- [x] 1.3 Remove `sudo` / `SUDO_USER` handling from kernel build delegation inside packaging script

## 2. Verification and docs

- [x] 2.1 Remove `sudo` from `scripts/verify-boot-usb.sh`; invoke packaging directly
- [x] 2.2 Update README and Makefile: `make usb` / `make etcher` / `make verify-usb` run unprivileged; Etcher still uses `akoya-boot.img`
- [x] 2.3 Scrub packaging script error messages that imply re-running under `sudo` (keep optional `apt install` hints user-run)

## 3. Rootless QEMU LAN path

- [x] 3.1 Update `scripts/install-bridge-libexec.sh` to apply `cap_net_admin` (or equivalent) so bridge helpers run unprivileged after one-time install
- [x] 3.2 Remove all `sudo` / `sudo -n` invocation from `scripts/run-qemu.sh`; call libexec helpers directly
- [x] 3.3 Update README bridge setup: one-time admin install, then `make test` / `make run` as normal user; revise or demote sudoers-only path

## 4. Verification evidence

- [x] 4.1 Run `make usb` and `make verify-usb` as unprivileged user; capture success output in apply notes
- [x] 4.2 Run `make test` (or headless run entry point) as unprivileged user after one-time bridge install with capabilities; capture evidence or document blocker if environment lacks cap support

## Explicitly deferred

- Rewriting macvtap to avoid `CAP_NET_ADMIN` on Linux
- Requiring admin privilege during one-time `install-bridge-libexec.sh` itself (operator-initiated, not a make target)
