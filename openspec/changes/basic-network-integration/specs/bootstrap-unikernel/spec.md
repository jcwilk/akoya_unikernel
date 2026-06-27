## MODIFIED Requirements

### Requirement: Console output without external dependencies

The bootstrap image SHALL produce its initial diagnostic message using only bootstrapped console facilities available immediately after kernel handoff, without relying on filesystem mounts or user interaction. Network services MAY be used only after that initial message is emitted and only when the built image is the network-enabled bootstrap variant.

#### Scenario: Standalone console-first boot

- **GIVEN** no disk filesystem is available after boot
- **WHEN** the bootstrap image starts
- **THEN** the initial diagnostic message is still emitted to console output before any network initialization begins

#### Scenario: Network variant defers remote services

- **GIVEN** the network-enabled bootstrap variant
- **WHEN** the image starts
- **THEN** the initial diagnostic message appears before DHCP or connectivity probes run

### Requirement: Deterministic bootstrap scope

The bootstrap image SHALL perform no application logic beyond initialization, console setup, emitting diagnostic output (including network diagnostics when the network-enabled variant is built), and an orderly halt or idle loop.

#### Scenario: No hidden side effects

- **GIVEN** the bootstrap image is invoked for pipeline verification
- **WHEN** boot completes
- **THEN** no persistent services beyond boot-time diagnostics remain active
- **AND** no persistent mutations are made to attached storage
- **AND** behavior is suitable for repeated smoke testing

#### Scenario: Network scope bounded to diagnostics

- **GIVEN** the network-enabled bootstrap variant
- **WHEN** boot completes
- **THEN** network activity is limited to address acquisition, address reporting, and a single connectivity probe
- **AND** no listening servers or background retry loops continue after diagnostics finish
