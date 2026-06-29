## Required for this change

## 1. Boot ISO packaging

- [ ] 1.1 Add `scripts/build-boot-iso.sh`: invoke `scripts/build.sh` when `build/kernel.bin` is missing, stage GRUB Multiboot1 layout, run `grub-mkrescue`, write `build/akoya-boot.iso`, emit `AKOYA_ISO_RESULT=...` on success/failure
- [ ] 1.2 Fail fast when `grub-mkrescue`, `xorriso`, or other required host tools are absent; error output names the missing dependency and suggested package install on Debian/Ubuntu
- [ ] 1.3 Add `make iso` Makefile target wrapping the packaging script
- [ ] 1.4 Embed build-id in ISO volume label or GRUB menu title for operator identification

## 2. QEMU boot-from-ISO path

- [ ] 2.1 Extend `scripts/run-qemu.sh` with `--boot-iso PATH` (mutually exclusive with `--image` / `--logical`) to boot the guest from optical media using BIOS/Legacy boot order
- [ ] 2.2 Reuse existing macvtap LAN attachment, serial capture, headless timeout, and bootstrap assertion logic when booting from ISO
- [ ] 2.3 Document `--boot-iso` in `run-qemu.sh` usage text and README

## 3. ISO verification entry point

- [ ] 3.1 Add `scripts/verify-boot-iso.sh`: ensure ISO exists (call packaging script if needed), run headless `--boot-iso` smoke with inference pre-flight, exit 0 on bootstrap + configured network/chat assertions
- [ ] 3.2 Add `make verify-iso` Makefile target (or document `verify-boot-iso.sh` as the agent entry point in README)
- [ ] 3.3 Capture verification evidence: successful run log showing ISO boot path and bootstrap diagnostic message in `build/` or console output referenced in apply notes

## 4. Documentation

- [ ] 4.1 Update README bare-metal section: automated ISO path as primary; USB `dd` imaging steps with device-selection warnings; internal-drive imaging note; firmware boot-menu guidance
- [ ] 4.2 Move manual GRUB/USB setup to a fallback appendix; note workstation packages (`grub-pc-bin`, `xorriso`) for packaging

## Explicitly deferred

- Bare-metal acceptance on physical Medion Akoya EX hardware (blocked until machine arrives; VM verification satisfies pre-shipment gate)
- UEFI boot support
- Transport-test ISO variant or multi-image GRUB menu beyond production kernel default
