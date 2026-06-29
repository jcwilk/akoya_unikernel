## Why

Boot-media packaging for legacy-BIOS USB sticks currently requires `sudo make usb` because the implementation uses loop mounts and `grub-install` on a block device. Verification scripts invoke `sudo` when the image is missing. The QEMU runner invokes `sudo -n` for macvtap LAN setup. None of this should be part of the normal agent or developer workflow — build, package, verify, and emulate should run as an unprivileged user with host tools on `PATH`.

## What Changes

- Refactor USB/HDD disk-image packaging so `make usb` / `make etcher` complete without root or any privilege-elevation wrapper.
- Remove `sudo` invocation from verification scripts that build or boot packaged media.
- Refactor the QEMU run path so documented test/run entry points never call `sudo`; macvtap setup uses a capability-bound helper the user can invoke directly after one-time workstation install.
- Document that one-time workstation install steps (copying libexec helpers, setting file capabilities) are operator actions outside automated make targets, not part of day-to-day script execution.
- Update README and error messages to stop recommending `sudo make usb` or `sudo apt` inside script failure text as if the script itself will run elevated.

## Capabilities

### New Capabilities

<!-- none -->

### Modified Capabilities

- `boot-media-packaging`: All packaging entry points (ISO and legacy-BIOS USB/HDD disk image) SHALL run unprivileged; add explicit disk-image packaging contract aligned with Etcher/USB use.
- `unikernel-build-pipeline`: Build and packaging entry points SHALL not require superuser privileges.
- `dev-test-runner`: Documented run and verification entry points SHALL not invoke privilege elevation during normal operation.

## Impact

- `scripts/build-boot-usb.sh`, `scripts/verify-boot-usb.sh`, `scripts/run-qemu.sh`
- `scripts/qemu-bridge-up.sh` / `scripts/qemu-bridge-down.sh` and install path (capability binding instead of sudoers-from-runner)
- `Makefile`, `README.md`
- No change to kernel source or bare-metal runtime behavior
