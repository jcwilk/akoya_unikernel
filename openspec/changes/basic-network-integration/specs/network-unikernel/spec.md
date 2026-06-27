## ADDED Requirements

### Requirement: Wired Ethernet bring-up

The bootstrap image SHALL initialize the wired Ethernet interface and report link readiness on the console before attempting IP configuration.

#### Scenario: Link ready before DHCP

- **GIVEN** a supported Ethernet environment with a connected cable or emulated link
- **WHEN** the network bootstrap sequence starts
- **THEN** the image completes Ethernet initialization
- **AND** console output indicates whether the link is ready for transmit and receive

#### Scenario: Link not ready

- **GIVEN** no usable Ethernet link is available
- **WHEN** the network bootstrap sequence starts
- **THEN** console output reports that link bring-up failed or timed out
- **AND** the image does not claim a successful DHCP lease

### Requirement: IPv4 address via DHCP

The bootstrap image SHALL obtain an IPv4 address, subnet mask, and default gateway using DHCP on the wired interface.

#### Scenario: Successful lease

- **GIVEN** a DHCP server is reachable on the attached LAN
- **WHEN** the image completes DHCP negotiation within the configured wait period
- **THEN** the image holds a usable IPv4 configuration for outbound traffic

#### Scenario: DHCP unavailable

- **GIVEN** no DHCP server responds within the configured wait period
- **WHEN** DHCP negotiation completes or times out
- **THEN** console output reports DHCP failure
- **AND** the image does not proceed with a connectivity probe as if configured

### Requirement: Assigned address reporting

After successful DHCP, the bootstrap image SHALL print the assigned IPv4 address on the console in a single, human-readable line before any connectivity probe.

#### Scenario: Address visible after lease

- **GIVEN** DHCP completed successfully
- **WHEN** the address reporting step runs
- **THEN** console output includes the assigned IPv4 address
- **AND** an observer can read the address without parsing binary protocols

### Requirement: Single outbound connectivity probe

After reporting its IPv4 address, the bootstrap image SHALL send exactly one ICMP echo request to a configured connectivity target and await a reply or failure within a bounded time.

#### Scenario: Probe succeeds

- **GIVEN** a valid IPv4 configuration and a reachable connectivity target
- **WHEN** the probe completes
- **THEN** console output indicates success
- **AND** console output includes round-trip latency for the echo exchange

#### Scenario: Probe fails

- **GIVEN** a valid IPv4 configuration
- **AND** the connectivity target does not respond or the network blocks the probe
- **WHEN** the probe completes or times out
- **THEN** console output indicates failure
- **AND** console output includes a short reason category (for example timeout, unreachable, or no reply)

### Requirement: Orderly completion after network diagnostics

The bootstrap image SHALL finish its boot-time work after emitting network diagnostics and then enter an orderly halt or idle loop without starting additional services.

#### Scenario: No follow-on traffic

- **GIVEN** network diagnostics have been printed
- **WHEN** boot completes
- **THEN** the image does not send further application traffic beyond what the diagnostics required
- **AND** behavior remains suitable for repeated smoke testing

### Requirement: Modular network boundaries

Network behavior SHALL be structured so that link-layer, IPv4, DHCP, and connectivity-probe concerns can be extended or replaced independently without rewriting unrelated layers.

#### Scenario: Layer replacement without cross-layer churn

- **GIVEN** a future change replaces the wired driver or adds a new upper-layer protocol
- **WHEN** reviewers inspect the network implementation
- **THEN** each layer exposes a narrow interface to adjacent layers
- **AND** DHCP and connectivity-probe logic do not embed hardware-specific details of unrelated devices
