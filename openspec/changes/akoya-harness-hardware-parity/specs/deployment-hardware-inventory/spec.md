## ADDED Requirements

### Requirement: Deployment observation in authoritative record

The authoritative hardware record SHALL include a normalized section for facts confirmed by direct observation of the deployment Medion Akoya EX unit, distinct from retailer or review sources. Observation-derived facts SHALL cover at minimum: processor identity, chipset, installed memory, wired Ethernet adapter identity and placement, wireless adapter identity, firmware vendor and version, primary display characteristics, integrated input devices, and boot storage class.

#### Scenario: Observation batch present after reconciliation

- **GIVEN** direct observation logs from the deployment unit are retained in the source store
- **WHEN** the inventory reconciliation for this change is complete
- **THEN** the authoritative hardware record includes observation-derived facts for each required subsystem listed above
- **AND** each fact names at least one retained observation log as its source

#### Scenario: Observation supersedes imprecise marketing facts

- **GIVEN** an observation-derived fact conflicts with a less precise retailer or review fact on the same field (for example exact PCI location or adapter revision)
- **WHEN** the inventory is updated
- **THEN** the authoritative record reflects the observation-derived value
- **AND** marketing sources remain cited for fields observation did not measure

### Requirement: Observation batch indexed in source store

The source material index SHALL list the deployment observation batch with retrieval date and the categories of facts it supports.

#### Scenario: Index row for observation batch

- **GIVEN** observation logs are retained under the Akoya EX source store
- **WHEN** a reviewer audits provenance
- **THEN** the index includes an entry for the observation batch
- **AND** the entry states that facts come from direct measurement of the deployment unit
