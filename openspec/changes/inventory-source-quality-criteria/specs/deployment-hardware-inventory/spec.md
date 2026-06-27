# deployment-hardware-inventory Specification (delta)

## ADDED Requirements

### Requirement: Primary specification or observation

Retained source material SHALL document the Medion Akoya EX through manufacturer publication, direct observation of the deployment machine, professional review of an identified Akoya EX unit, or engineering documentation for a subsystem already confirmed on the Akoya EX through a separate retained source. Material whose main value is resale, compatibility merchandising, or informal peer assistance SHALL NOT satisfy this requirement on its own.

#### Scenario: Manufacturer or direct observation

- **GIVEN** a source is published by the machine manufacturer for this product, or records verifiable measurements from the actual Akoya EX hardware
- **WHEN** a contributor evaluates it for the inventory
- **THEN** it MAY be retained if it also meets Akoya EX exclusive confirmation

#### Scenario: Professional review of identified unit

- **GIVEN** a source is a professional or editorial review that names the Medion Akoya EX and describes configuration of a unit under test
- **WHEN** a contributor evaluates it for the inventory
- **THEN** it MAY be retained for facts observed or stated in that review

### Requirement: No commercially inferred hardware claims

Hardware capability or configuration claims SHALL NOT be supported solely by material whose stated limits are derived from a seller's product catalog, upgrade compatibility listing, or stocked offerings rather than from specification or observation of the machine.

#### Scenario: Upgrade catalog states a limit

- **GIVEN** a source's primary purpose is selling compatible parts for the Akoya EX
- **AND** the source states a maximum configuration based on which modules or options the seller lists
- **WHEN** a contributor evaluates it for a hardware capability claim
- **THEN** the source SHALL NOT be retained as evidence for that claim
- **AND** the claim SHALL NOT appear in the hardware record unless supported by primary specification or observation

#### Scenario: Catalog agrees with other evidence

- **GIVEN** a commercial compatibility listing repeats a limit also documented by primary specification or observation
- **WHEN** a contributor evaluates provenance
- **THEN** the commercial listing alone still SHALL NOT be retained as evidence
- **AND** the claim SHALL rely on the primary source in the index

### Requirement: No unverified user discourse

Material consisting of user questions, peer answers, discussion threads, or other informal community discourse SHALL NOT be retained as inventory evidence, even when participants name the Medion Akoya EX or assert hardware facts.

#### Scenario: Peer answer names the machine

- **GIVEN** a source is a support forum, Q&A thread, or similar user-generated exchange
- **AND** a participant states hardware facts about the Medion Akoya EX
- **WHEN** a contributor evaluates it for the inventory
- **THEN** the source SHALL NOT be retained
- **AND** those assertions SHALL NOT support entries in the hardware record

#### Scenario: Discourse cites primary documentation

- **GIVEN** user discourse merely links to or quotes manufacturer documentation
- **WHEN** a contributor evaluates provenance
- **THEN** the discourse itself still SHALL NOT be retained
- **AND** the contributor SHALL locate and retain the cited primary source instead if it qualifies
