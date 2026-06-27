## Context

The hardware inventory spec already requires Akoya EX–exclusive confirmation and component corroboration. In practice, contributors still treated **memory-vendor compatibility pages** and **forum Q&A** as evidence because they name the model. Those sources fail a deeper test: they do not document the machine's engineering specifications—they document what a vendor sells or what a stranger asserted.

The Offtek page listed a 1 GB maximum and only 512 MB modules; that was treated as a "conflict" with the Intel 855GM chipset limit. The vendor listing does not measure machine capability—it reflects catalog availability. It should never have been retained.

## Goals / Non-Goals

**Goals:**

- Encode generalized rules: retained sources must be **primary** (OEM, professional review of the actual product, engineering docs for machine-confirmed subsystems)—not **derivative** (reseller catalogs, forum answers).
- Exclude material whose claims are **commercially inferred** or **user-discourse unverified**.
- Remove Offtek from the inventory and re-audit YAML during apply.

**Non-Goals:**

- Enumerating banned domains or file formats in the spec.
- Changing the three-artifact layout (`akoya_ex.yaml`, `akoya_ex/`, README index).
- Rewriting the Akoya EX exclusive confirmation requirement (it stays; new rules complement it).

## Decisions

### 1. Generalize by authority and purpose, not examples

**Choice:** New requirements describe *why* a source class is insufficient (lack of authoritative verification; claims inferred from commerce or informal discourse) rather than naming "forums" or "memory vendors."

**Rationale:** Future edge cases (marketplace listings, wiki edits, AI summaries) fail the same test without updating a blocklist.

**Design examples (not spec obligations):**

| Weak pattern | Why it fails |
|--------------|--------------|
| Memory upgrade shop "max 1 GB" | Limit reflects stocked SKUs, not OEM engineering |
| Forum "Akoya EX supports X" | Unverified peer assertion, no accountability |
| Marketplace title mentioning specs | Marketing copy, not specification document |

### 2. Component datasheets remain allowed with machine tie-in

**Choice:** Keep existing component-corroboration rule; Intel 855GM datasheet + review confirming 855GM on Akoya EX remains valid.

**Rationale:** Engineering documentation for a confirmed subsystem is primary evidence for that subsystem's limits—not a reseller inference.

### 3. Remove Offtek in apply

**Choice:** Delete `offtek-md95335-memory-specs.md`; drop YAML facts supported only by Offtek (notably **MD95335** model number and PC2700 slot details unless corroborated elsewhere).

**Rationale:** Aligns inventory with the new rules immediately.

## Risks / Trade-offs

| Risk | Mitigation |
|------|------------|
| Inventory becomes sparser after Offtek removal | Accept sparse record until stronger Akoya EX sources arrive |
| "Professional review" boundary fuzzy | Review must name Akoya EX and describe tested unit configuration |
| Over-exclusion of retailer pages that quote OEM specs | Retain only if page reproduces verifiable OEM specification text, not vendor-inferred limits |

## Migration Plan

1. Archive spec delta.
2. Remove Offtek source and dependent YAML fields.
3. Update README index and conflict section (remove Offtek-based conflict narrative).

## Open Questions

- None blocking propose; MD95335 may disappear from YAML until an OEM or review source confirms the SKU.
