## Context

The repository targets the Medion Akoya EX notebook. Build tooling currently reads a thin `target/akoya.profile` shell constants file; there is no structured hardware inventory or provenance discipline. The human wants a simple layout that can grow freely before the physical machine arrives, without schema ceremony or open-question tracking in the YAML itself.

## Goals / Non-Goals

**Goals:**

- Establish three concrete artifacts under `target/`:
  - `target/akoya_ex.yaml` — single machine-readable hardware record; free-form, objective facts; grows continuously as details are discovered.
  - `target/akoya_ex/` — directory for source material (PDFs, saved web pages, markdown notes, images, etc.).
  - `target/akoya_ex/README.md` — human-readable index of every source in the directory and what Akoya EX facts it supports; kept up to date when sources change.
- Enforce strict Akoya EX–only provenance: nothing enters the store unless fully confirmed for this exact model.
- Seed initial source material during apply via best-effort internet research, populating YAML only from confirmed facts.

**Non-Goals:**

- Prescribing which hardware fields the YAML must contain (the record is intentionally open-ended).
- JSON Schema validation, confidence tags, simulation/QEMU mapping files, or codegen in this change.
- Replacing or auto-generating `target/akoya.profile` (it remains the small build-time constants file sourced by `scripts/build.sh`).
- Physical-machine dumps (laptop not yet on hand).
- Kernel drivers, QEMU device wiring, or build pipeline consumption of the YAML.

## Decisions

### 1. Flat three-file layout instead of layered inventory tree

**Choice:** One YAML file, one source directory, one README index — no separate `sources.yaml`, `simulation.yaml`, or `gather/` session logs.

**Rationale:** Matches the human's explicit simplification request. Provenance lives in README + stored files, not parallel metadata files.

**Alternatives considered:**

| Option | Why not now |
|--------|-------------|
| Multi-file YAML + JSON Schema | Rejected as too complicated for current needs |
| Provenance embedded in YAML per field | Adds ceremony; README + source files are enough |

### 2. YAML is objective facts only

**Choice:** `akoya_ex.yaml` holds hardware details in whatever YAML structure is useful at the time. No `open_questions` section, no confidence enums required.

**Rationale:** Unknowns are simply omitted until confirmed. The strict source rule prevents guessing from entering the record.

### 3. Provenance enforced at the source-store boundary

**Choice:** A fact may appear in YAML only if backed by Akoya EX–specific source material in `target/akoya_ex/`. Generic component datasheets alone are insufficient; a separate Akoya EX source must tie the component to this machine.

**Rationale:** Prevents "probably the same NIC as other 2006 Medion laptops" drift before the machine arrives.

### 4. `akoya.profile` stays separate

**Choice:** Keep `target/akoya.profile` as the bash-sourcable build constants subset. Document in `target/akoya_ex/README.md` that the profile is derived manually from confirmed inventory facts when build tooling needs them.

**Rationale:** Avoids pulling a YAML parser into shell build scripts in this change.

## Risks / Trade-offs

| Risk | Mitigation |
|------|------------|
| Very little Akoya EX–specific material exists online | Apply task is best-effort; YAML may stay sparse until the machine arrives; do not fill gaps with variant guesses |
| README index drifts from directory contents | Index maintenance is a spec requirement; apply includes README as part of every source addition |
| Free-form YAML becomes inconsistent over time | Acceptable trade-off; structure can evolve organically; strict sourcing matters more than schema |
| Duplicate facts between `akoya.profile` and YAML | Document manual sync rule; defer codegen |

## Migration Plan

Greenfield addition under `target/`. No changes to existing build or test behavior. Rollback is revert the apply branch.

**Apply sequence:**

1. Create `target/akoya_ex/` and `target/akoya_ex/README.md`.
2. Research and store Akoya EX–specific sources; reject variant/generic-only material.
3. Create `target/akoya_ex.yaml` populated only from confirmed sources.
4. Cross-check README index covers all stored sources and supported YAML claims.

## Open Questions

- Exact Medion marketing SKU / model number strings for search (to be captured if found in Akoya EX–specific sources during apply).
- Which Akoya EX–specific official or archival pages remain reachable on the public web at apply time.
