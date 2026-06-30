## 1. Inventory reconciliation

- [x] 1.1 Add `observed_on_deployment_unit` section to `target/akoya_ex.yaml` with normalized facts from `target/akoya_ex/raw_inspection_logs/` (CPU, chipset, memory, PCI wired NIC, wireless NIC, BIOS, display, input, storage class); cite source log per fact
- [x] 1.2 Add observation-batch index row to `target/akoya_ex/README.md` with retrieval date and supported fact categories
- [x] 1.3 Sync `target/akoya.profile` CPU declaration with observation (`pentium-m` / Pentium M 735) if not already aligned; keep `AKOYA_RAM_MB=2048` for bare-metal documentation

## 2. Guest timekeeping

- [x] 2.1 Replace fixed `tsc_per_ms` in `kernel/time/time.c` with PIT-cross-check calibration at `time_init()`; clamp to sane bounds
- [x] 2.2 Verify calibrated `time_delay_ms` is within ~10% of configured duration under CPU-faithful QEMU (document command and observed values in apply notes)
- [x] 2.3 Re-run `make test` after calibration; confirm timed-gap regression still passes

## 3. Emulation CPU and headful fidelity

- [x] 3.1 Source `target/akoya.profile` in `scripts/run-qemu.sh`; map `AKOYA_CPU_CLASS` to QEMU `-cpu` (document chosen model from `qemu-system-i386 -cpu help`); keep `-m 512` default
- [x] 3.2 Add optional `AKOYA_QEMU_CPU` env override; document in README
- [x] 3.3 Set explicit headful VGA (`-vga std` or equivalent) for deployment-faithful text console; confirm SDL keyboard still uses PS/2 path
- [x] 3.4 Manual headful smoke: bootstrap message, network line, interactive prompt visible in SDL window

## 4. Deploy-faithful verification tier

- [x] 4.1 Update README agent verification table and pre-flash guidance: `make verify-usb` required alongside `make test` for flash sign-off
- [x] 4.2 Run `make verify-usb` on workstation; capture pass evidence in apply notes (bootstrap + connectivity probe)

## 5. NIC alignment audit

- [x] 5.1 Audit `kernel/net/eth/rtl8139.c` init against observation facts (I/O BAR, rev 10, CONFIG1/HLTCLK wake, link-up poll); fix only if metal or faithful emulation shows a gap
- [x] 5.2 Document in apply notes that IRQ-driven RX remains polling-based unless bring-up fails on deployment unit

## 6. Documentation and validation

- [x] 6.1 Update README divergence notes: RAM emulation intentionally reduced; serial headless preserved; screen clear unchanged
- [x] 6.2 Run `npx @fission-ai/openspec@latest validate akoya-harness-hardware-parity --type change` and fix any issues

## Explicitly deferred

- Guest-side inference endpoint pre-flight (host TCP check remains sufficient for this change)
- WiFi, touchpad, i915 framebuffer, or full chipset emulation in QEMU
- IRQ-driven RTL8139 receive path unless deployment bring-up requires it
- Headful automated assertion harness (headless scripts remain the automation surface)
- Raising emulated RAM to deployment capacity
