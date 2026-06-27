## Why

Early inventory work retained sources that name the Akoya EX but do not actually document machine hardware—memory-vendor compatibility pages infer limits from product catalogs, and forum threads offer unverified peer answers. Those categories create false conflicts and ambiguous provenance. The living spec needs generalized exclusion criteria so future source decisions are consistent.

## What Changes

- Add behavioral requirements to `deployment-hardware-inventory` defining which classes of sources are insufficient for hardware claims, grounded in authority and purpose rather than a blocklist of site types.
- During apply, remove the Offtek memory-vendor source from the Akoya EX inventory and revise the hardware record, index, and any facts that depended solely on it.

## Capabilities

### New Capabilities

- (none)

### Modified Capabilities

- `deployment-hardware-inventory`: Add source-quality exclusion requirements; clarify that retained material must be primary specification or verified observation, not commercially inferred or user-discourse claims.

## Impact

- Living spec `openspec/specs/deployment-hardware-inventory/spec.md` gains new requirements after archive.
- Apply touches `target/akoya_ex/` source store, index, and `target/akoya_ex.yaml` (Offtek removal and provenance audit).
- No build, QEMU, or kernel behavior changes.
