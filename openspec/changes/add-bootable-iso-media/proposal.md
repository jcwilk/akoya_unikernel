## Why

The project can cross-compile a bootable kernel and validate it under QEMU by loading the flat image directly, but bare-metal deployment still requires manual GRUB/USB setup documented in the README. That gap blocks the practical goal of having a bootable flash drive ready before the Akoya hardware arrives—plug in, power on, and run diagnostics without ad-hoc bootloader configuration on delivery day.

## What Changes

- Add a single documented packaging entry point that turns a successful kernel build into a BIOS/Legacy-bootable ISO suitable for writing to removable flash media or an internal boot drive.
- Package the production chat unikernel (not transport-test) as the default boot payload, using the deployment target's Multiboot1 boot protocol.
- Extend pre-hardware verification so agents can boot the packaged ISO under emulation and assert the same bootstrap and network diagnostic behavior as the existing direct-image smoke path.
- Document how operators write the ISO to USB or internal storage and what to expect on first bare-metal boot.
- Replace the README's deferred "manual USB/GRUB" note with a first-class automated path while keeping transport-test and direct-kernel QEMU paths unchanged.

## Capabilities

### New Capabilities

- `boot-media-packaging`: Produce a BIOS/Legacy-bootable ISO from built unikernel artifacts, with agent-parseable success/failure output and a stable artifact reference suitable for flashing or burning before hardware is on hand.

### Modified Capabilities

- `dev-test-runner`: Add a verification path that boots the packaged ISO under emulation (not only a direct kernel image) and gates on observable bootstrap success.
- `deployment-target`: Codify that deployment boot media SHALL support BIOS/Legacy boot from removable USB flash and from written internal boot storage on target-class hardware.

## Impact

- New packaging script and build output under the project build area (ISO artifact).
- Optional Makefile target for discoverability alongside existing `build`, `test`, and `run`.
- README updates for flash-drive preparation and bare-metal first boot.
- Workstation tooling dependencies for ISO creation (host utilities documented in design/tasks, not spec obligations).
- No changes to kernel source behavior, network stack, or chat session semantics beyond how the image is delivered to the bootloader.
