## Required for this change

## 1. Layout

- [ ] 1.1 Create `target/akoya_ex/` directory for source material
- [ ] 1.2 Create `target/akoya_ex/README.md` as the source index (initial structure: how to use the directory, relationship to `akoya.profile`, Akoya EX–only source rule)
- [ ] 1.3 Create `target/akoya_ex.yaml` as the machine-readable hardware record (start sparse; only Akoya EX–confirmed facts)

## 2. Initial source gathering (best effort)

- [ ] 2.1 Search the public web for materials that **explicitly identify the Medion Akoya EX** (official product pages, manuals, reviews that name this model, archived retailer listings, etc.); reject other Akoya variants and unnamed similar notebooks
- [ ] 2.2 Save retained sources into `target/akoya_ex/` (markdown summaries, downloaded HTML/PDF where permitted, or link archives with local notes — each file must be storable in-repo)
- [ ] 2.3 Update `target/akoya_ex/README.md` to index every retained source: filename, what Akoya EX facts it supports, and retrieval date/URL
- [ ] 2.4 Populate `target/akoya_ex.yaml` only with facts traceable to indexed Akoya EX–specific sources; leave unknown subsystems absent rather than inferred
- [ ] 2.5 Audit: every YAML fact has a supporting source in the index; no retained source lacks an index entry; no variant-only or generic-component-only evidence entered the store

## Explicitly deferred

- QEMU simulation mapping or harness consumption of the YAML inventory
- Codegen from `akoya_ex.yaml` into kernel headers or `akoya.profile`
- Physical-machine dumps (`lspci`, `dmidecode`, etc.) until the laptop is on hand
- JSON Schema or automated validation of the YAML structure
