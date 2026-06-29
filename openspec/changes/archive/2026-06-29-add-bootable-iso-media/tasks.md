## Required for this change

## 1. Boot ISO packaging

- [x] 1.1 Add `scripts/build-boot-iso.sh`: invoke `scripts/build.sh` when `build/kernel.bin` is missing, stage GRUB Multiboot1 layout, run `grub-mkrescue`, write `build/akoya-boot.iso`, emit `AKOYA_ISO_RESULT=...` on success/failure
- [x] 1.2 Fail fast when `grub-mkrescue`, `xorriso`, or other required host tools are absent; error output names the missing dependency and suggested package install on Debian/Ubuntu
- [x] 1.3 Add `make iso` Makefile target wrapping the packaging script
- [x] 1.4 Embed build-id in ISO volume label or GRUB menu title for operator identification

## 2. QEMU boot-from-ISO path

- [x] 2.1 Extend `scripts/run-qemu.sh` with `--boot-iso PATH` (mutually exclusive with `--image` / `--logical`) to boot the guest from optical media using BIOS/Legacy boot order
- [x] 2.2 Reuse existing macvtap LAN attachment, serial capture, and headless timeout when booting from ISO; support a lighter assertion profile (bootstrap + connectivity-probe success only) distinct from default multi-turn chat smoke
- [x] 2.3 Document `--boot-iso` in `run-qemu.sh` usage text and README

## 3. ISO verification entry point

- [x] 3.1 Add `scripts/verify-boot-iso.sh`: ensure ISO exists (call packaging script if needed), run headless `--boot-iso` smoke without inference pre-flight, exit 0 on bootstrap diagnostic message plus successful connectivity probe to the build-configured probe target (default `google.com`)
- [x] 3.2 Add `make verify-iso` Makefile target (or document `verify-boot-iso.sh` as the agent entry point in README)
- [x] 3.3 Capture verification evidence: successful run log showing ISO boot path, bootstrap diagnostic message, and successful connectivity-probe output in `build/` or console output referenced in apply notes

## 4. Documentation

- [x] 4.1 Update README bare-metal section: automated ISO path as primary; USB `dd` imaging steps with device-selection warnings; internal-drive imaging note; firmware boot-menu guidance
- [x] 4.2 Move manual GRUB/USB setup to a fallback appendix; note workstation packages (`grub-pc-bin`, `xorriso`) for packaging

## Explicitly deferred

- Bare-metal acceptance on physical Medion Akoya EX hardware (blocked until machine arrives; VM verification satisfies pre-shipment gate)
- UEFI boot support
- Transport-test ISO variant or multi-image GRUB menu beyond production kernel default
