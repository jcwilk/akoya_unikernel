## Context

The repository already produces `kernel.bin` / `kernel.elf` via `scripts/build.sh` and validates them under QEMU by passing the flat Multiboot payload directly to the emulator (`scripts/run-qemu.sh`). The deployment profile (`target/akoya.profile`) specifies Multiboot1 boot via GRUB on USB in BIOS/Legacy mode. README bare-metal instructions are manual: prepare USB, install GRUB, add a multiboot entry.

The Akoya EX has no VT-x; operators need a flash drive ready before hardware delivery. An ISO provides a single artifact that can be written to USB (whole-device image) or to an internal boot partition/drive with standard imaging tools, then selected in the firmware boot menu.

## Goals / Non-Goals

**Goals:**

- One agent-invokable packaging script that builds (if needed) and emits a BIOS/Legacy-bootable ISO containing the main chat unikernel.
- ISO boots under `qemu-system-i386` from virtual optical media so packaging regressions are caught before hardware is available.
- Document operator steps to write the ISO to removable flash and to internal storage.
- Emit a machine-parseable packaging outcome line consistent with the build pipeline style.

**Non-Goals:**

- UEFI boot support (target firmware is BIOS/Legacy).
- Transport-test ISO variants (production kernel only for this change).
- Bare-metal acceptance on real Akoya hardware (deferred until machine arrives; VM gate only).
- Replacing or removing the existing direct-kernel QEMU path.
- Auto-detecting or installing GRUB on operator workstations beyond documenting prerequisites.

## Decisions

### ISO layout: GRUB + Multiboot1 payload

Use `grub-mkrescue` (Debian/Ubuntu: `grub-pc-bin`, `xorriso`) to produce a hybrid-capable BIOS boot ISO. GRUB config contains one default entry loading `/boot/kernel.bin` via Multiboot1, matching `AKOYA_BOOT_PROTOCOL="multiboot1"` in the deployment profile.

**Alternatives considered:**

- **Manual loop-mount + ext2 + syslinux** — more moving parts; GRUB matches README's existing manual path.
- **El Torito-only without GRUB** — kernel is Multiboot, not a Linux bzImage; GRUB is the standard Multiboot1 loader.

### Script placement and naming

Add `scripts/build-boot-iso.sh` as the packaging entry point. It invokes `scripts/build.sh` when `build/kernel.bin` is missing or stale (mtime comparison), stages a temporary tree, runs `grub-mkrescue`, writes `build/akoya-boot.iso`, and prints `AKOYA_ISO_RESULT=...`.

Add `make iso` as a thin wrapper for discoverability.

### VM verification path

Add `scripts/verify-boot-iso.sh` that:

1. Ensures ISO exists (calls packaging script if absent).
2. Invokes the existing QEMU runner with a new `--boot-iso PATH` mode (or dedicated internal flag) so the guest boots from optical media instead of `-kernel`.
3. Runs headless with a **lighter gate than default multi-turn chat smoke**: bootstrap diagnostic message plus successful outbound connectivity probe (default build probe target is `google.com` via `AKOYA_PROBE_HOST`). Reuse macvtap LAN attachment and serial capture from `run-qemu.sh`.
4. **Does not** require inference-endpoint pre-flight or chat-completion assertions—connectivity-probe success is sufficient evidence that the ISO booted and wired networking works.

**Rationale:** Boot-from-ISO is a distinct code path from `-kernel`; a dedicated verify script keeps agent entry points obvious while sharing LAN infrastructure. Ping success proves bootloader wiring, kernel execution, DHCP, and outbound IP reachability without coupling ISO packaging validation to llama.cpp availability.

**Alternative:** Reuse full default smoke gate — rejected; operator goal is pre-shipment boot-media confidence, not chat regression on every ISO build.

### Writable media operator flow

Document in README (not spec):

- **USB flash:** `sudo dd if=build/akoya-boot.iso of=/dev/sdX bs=4M status=progress conv=fsync` (with warnings about device selection).
- **Internal drive:** same whole-disk write or partition-targeted write depending on operator intent; firmware boot order set to USB or HDD as appropriate.

ISO is the interchange format; imaging tool choice stays operator-side.

## Risks / Trade-offs

| Risk | Mitigation |
|------|------------|
| Missing host packages (`grub-mkrescue`, `xorriso`) | Fail fast with actionable install hints in script output |
| ISO boots in QEMU but not on real USB | VM gate covers bootloader wiring; bare-metal checklist remains in README |
| Stale kernel inside ISO after rebuild | Packaging script rebuilds or documents `--force-rebuild`; verify script always packages fresh ISO by default |
| GRUB version differences across distros | Pin minimum tested distro note in README; CI/agent runs on project dev workstation |

## Migration Plan

1. Land packaging script and Makefile target.
2. Extend `run-qemu.sh` for ISO boot mode.
3. Add verify script; wire optional `make verify-iso` or document as agent entry point.
4. Update README bare-metal section to prefer automated ISO path; keep manual GRUB steps as fallback appendix.
5. No migration of existing build artifacts; ISO is additive output.

## Open Questions

- Whether to embed build-id in ISO volume label for operator identification (recommended: yes, in design implementation).
- Exact minimum GRUB package set on non-Debian workstations (document as tested-on; fail-fast elsewhere).
