## Why

Exhaustive direct observation of the deployment Medion Akoya EX now exists, but the development test harness still emulates a generic low-end PC (wrong CPU model, uncalibrated guest timekeeping, direct-multiboot shortcuts) while the authoritative hardware record has not absorbed the observation batch. QEMU passes can therefore diverge from bare-metal behavior—especially timed-gap chat regression and deploy boot paths—without anyone noticing until flash.

## What Changes

- Reconcile verified deployment-unit observation into the authoritative hardware inventory and source index.
- Drive emulated CPU selection from the deployment profile so QEMU matches Pentium M class rather than a generic Pentium III stand-in.
- Calibrate guest millisecond timing so bounded delays (timed-gap regression, DHCP/timeouts) reflect intended durations on deployment hardware and faithfully emulated CPU.
- Tighten headful emulation fidelity (display and integrated keyboard path) without sacrificing headless serial capture and automated text injection.
- Add a documented deploy-faithful boot verification tier (BIOS/GRUB media boot) complementary to fast direct-multiboot smoke tests.
- Align RTL8139 bring-up with deployment-silicon behaviors already required on bare metal (power/wake, polling model); IRQ-driven RX remains out of scope unless needed.
- **Explicit non-goal:** raising emulated RAM to deployment capacity—guest memory footprint stays modest and 512 MiB emulation is acceptable.

## Capabilities

### New Capabilities

- `guest-timekeeping`: Accurate millisecond delays on deployment CPU class and matching emulation.

### Modified Capabilities

- `deployment-hardware-inventory`: Incorporate direct deployment observation batch; normalize core subsystem facts in the authoritative record.
- `deployment-target`: Deployment profile supplies CPU constants consumed by build and emulation; document RAM emulation non-goal.
- `dev-test-runner`: CPU/profile-driven emulation, headful fidelity, deploy-faithful boot tier, preserve serial headless automation.
- `timed-gap-chat-regression`: Timed idle gaps depend on accurate guest timekeeping.

## Impact

- `target/akoya_ex.yaml`, `target/akoya_ex/README.md`, `target/akoya.profile`
- `scripts/run-qemu.sh`, related verify entry points, `Makefile` / README verification table
- `kernel/time/time.c` and callers of millisecond delays
- `kernel/net/eth/rtl8139.c` (deployment-silicon alignment only where behavior differs from QEMU today)
- OpenSpec living specs for the capabilities listed above after archive
