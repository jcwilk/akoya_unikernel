## Why

The project needs a durable, reviewable record of Medion Akoya EX hardware facts before the physical machine arrives, so build and emulation work can proceed with verified source material rather than assumptions. Today only a thin shell profile exists; there is no structured inventory or provenance rules for what counts as evidence.

## What Changes

- Introduce a machine-readable hardware record and a companion source-material store under `target/`, with an index document that tracks what each source supports.
- Add a new OpenSpec capability governing how hardware information is collected, stored, and kept current, including a strict rule that stored sources must be fully confirmed for the Medion Akoya EX specifically.
- During apply, seed the source-material store with a best-effort internet research pass and populate the hardware record from only Akoya-EX-confirmed facts.

## Capabilities

### New Capabilities

- `deployment-hardware-inventory`: Authoritative hardware documentation for the Medion Akoya EX, including provenance rules for source material and maintenance of the inventory index.

### Modified Capabilities

- `deployment-target`: Clarify that the deployment target is documented through the hardware inventory capability in addition to existing bare-metal behavioral constraints.

## Impact

- New files under `target/` (`akoya_ex.yaml`, `akoya_ex/` directory and README index).
- Existing `target/akoya.profile` remains the thin build-time constants file; relationship to the YAML inventory is documented in `design.md`.
- Apply work includes internet research to gather initial Akoya-EX-specific source material; no kernel, QEMU, or build pipeline behavior changes in this change.
