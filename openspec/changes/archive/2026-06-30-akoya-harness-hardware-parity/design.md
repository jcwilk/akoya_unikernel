## Context

Prior exploration compared `scripts/run-qemu.sh` and kernel drivers against `target/akoya_ex/raw_inspection_logs/`. Findings:

| Area | Harness today | Deployment unit |
|------|---------------|-----------------|
| CPU | QEMU `-cpu pentium3` | Pentium M 735 (family 6 model 13, 1.7 GHz, SSE2) |
| RAM | 512 MiB (intentionally kept) | ~2 GiB |
| Boot | Default `make test` uses direct multiboot `-kernel` | Insyde BIOS → GRUB on USB image |
| Time | `tsc_per_ms = 100000` fixed | TSC ~1.7 GHz; gaps wildly wrong on metal |
| NIC | RTL8139 (correct chip); QEMU topology | `01:02.0`, rev 10, IRQ 11, `8139too` class |
| Console | VGA text + COM1 serial | Same early path; panel 1280×768 via i915 later |
| Input headless | QEMU `sendkey` → i8042 | N/A on metal; serial remains essential for automation |
| Input headful | SDL → i8042 | Built-in PS/2 keyboard + Synaptics touchpad |
| Inventory YAML | Retailer/review sources only | Raw logs not yet reconciled |

Human decisions for this change:

1. Fix divergences except emulated RAM size.
2. Maximize practical simulation parity; boot-chain parity matters for pre-flash confidence.
3. Keep headless serial capture; keep post-bootstrap screen clear.
4. Headful should be as faithful as practical; headless must retain trivial send/receive text for tests.
5. Reconcile raw logs into authoritative inventory.

## Goals / Non-Goals

**Goals:**

- Authoritative inventory reflects direct observation of the deployment unit (CPU, chipset, PCI map, wired NIC, BIOS, display, input, storage class).
- QEMU CPU matches deployment profile (`pentium-m` / closest QEMU model).
- Guest `time_delay_ms` within ~10% of requested duration on deployment CPU and on CPU-matched emulation.
- `make verify-usb` documented and required for pre-flash sign-off alongside `make test`.
- Headful runs use deployment-faithful VGA text console sizing; integrated keyboard path unchanged.
- Headless automation unchanged in contract: serial stdio capture + monitor keyboard injection.
- RTL8139 init sequence validated against deployment rev-10 behavior (CONFIG1/HLTCLK wake already present).

**Non-Goals:**

- Emulating full 2 GiB RAM or 855GM/i915 framebuffer drivers.
- WiFi, modem, CardBus, FireWire, touchpad, or ACPI EC simulation.
- Replacing polling NIC driver with IRQ-driven RX unless bring-up fails on metal.
- Removing serial console or changing post-network `console_clear` behavior.
- Headful automated assertion harness (headless-only scripts remain).
- Guest-side inference pre-flight (host pre-flight stays; optional future work).

## Decisions

### D1 — Profile drives emulation CPU, not RAM

**Choice:** `scripts/run-qemu.sh` sources `target/akoya.profile` for `AKOYA_CPU_CLASS` (map to QEMU `-cpu` model, prefer `qemu32` or `pentium` with Pentium M flags where available; fallback chain documented). RAM stays `-m 512` unless overridden by env for experiments.

**Alternatives:** Hardcode `-cpu pentium` — rejected; profile is already the deployment constant source.

### D2 — TSC calibration via PIT cross-check at boot

**Choice:** `time_init()` measures TSC delta across a PIT-based ~50–100 ms interval (port 0x40/0x43), derives `tsc_per_ms`, clamps to sane bounds. Works on bare metal and QEMU when CPU model is aligned.

**Alternatives:** Fixed constant per platform table — rejected; calibration survives CPU frequency scaling better. HPET/ACPI PM timer — deferred (ICH4 has them but PIT is simpler and sufficient for ms delays).

### D3 — Two-tier verification

**Choice:**

- **Fast tier:** `make test` — direct multiboot, timed-gap regression (unchanged entry point).
- **Deploy tier:** `make verify-usb` — required documented pre-flash gate; README and agent table updated.

**Alternatives:** Make `make test` run USB boot — rejected (slow, duplicates coverage); both tiers required for release confidence.

### D4 — Inventory reconciliation structure

**Choice:** Add `observed_on_deployment_unit:` section to `akoya_ex.yaml` with nested facts (cpu, chipset, pci, network.wired, network.wireless, memory, display, bios, input, storage) each citing `raw_inspection_logs/<file>` as source. Index row in `target/akoya_ex/README.md` for the observation batch (date 2026-06-29, source = deployment unit direct observation store).

Retain existing retailer/review facts; observation supersedes where more precise (e.g., exact PCI BDF, MAC, rev).

### D5 — Headful fidelity

**Choice:** QEMU headful uses `-vga std` (explicit) with standard VGA text; no SDL scaling hacks. Document that operator sees 80×25 text like early bare-metal boot. Serial still attached for optional capture.

**Alternatives:** `-display gtk` — SDL already documented; keep SDL.

### D6 — Headless text I/O unchanged

**Choice:** Keep `-serial stdio` + monitor `sendkey`. Optional future: document that operators may run with serial only; no requirement to detect VGA without serial.

### D7 — NIC alignment

**Choice:** Audit `rtl8139_init` against inspection (I/O BAR, rev 10, link-up poll). Add targeted regression only if metal testing finds gap. Document that QEMU does not model IRQ 11 — polling remains acceptable.

## Risks / Trade-offs

| Risk | Mitigation |
|------|------------|
| QEMU lacks perfect Pentium M model | Use closest `-cpu` + profile flag; document fallback; calibrate TSC at runtime |
| PIT calibration inaccurate under heavy IRQ load | Acceptable for bootstrap delays; re-measure if gaps drift |
| USB verify tier skipped in CI | Document in README; tasks require human/agent attestation for pre-flash |
| Inventory YAML grows large | Normalize core fields only; raw logs remain canonical detail store |
| sendkey timing still unlike human typing | Timed-gap regression uses guest-side delays, not sendkey pacing |

## Migration Plan

1. Land inventory + profile updates (no runtime change).
2. Land TSC calibration; re-run `make test` on workstation.
3. Land QEMU profile CPU + headful tweak; re-run headful smoke manually.
4. Update README verification table; run `make verify-usb` once for evidence.
5. Archive change; living specs updated.

Rollback: revert TSC and QEMU args independently; inventory is additive.

## Open Questions

- Whether to add `AKOYA_QEMU_CPU` env override for experiments (default from profile) — **default yes, document in README**.
- Exact QEMU CPU string that best matches Pentium M 735 on host's QEMU version — **resolve during apply with `qemu-system-i386 -cpu help`**.
