## Required for this change

## 1. Inventory source cleanup

- [ ] 1.1 Remove the Offtek memory-vendor source file from the Akoya EX source store and its index entry
- [ ] 1.2 Re-audit `target/akoya_ex.yaml`: remove any facts supported solely by the removed source; ensure remaining facts trace to qualifying primary sources per the updated spec
- [ ] 1.3 Update the source index README: align exclusion guidance with the new spec intent; remove Offtek-based conflict narrative

## 2. Verification

- [ ] 2.1 Confirm every retained source file has an index row and every YAML fact maps to at least one qualifying source
- [ ] 2.2 Run `npx @fission-ai/openspec@latest validate inventory-source-quality-criteria --type change` and `npx @fission-ai/openspec@latest validate --specs`

## Explicitly deferred

- Searching for replacement Akoya EX–primary sources for facts lost when Offtek is removed (for example model SKU MD95335)
- Automated linting of source quality
