## MODIFIED Requirements

### Requirement: Orderly completion after network diagnostics

The bootstrap image SHALL finish its boot-time work after emitting network diagnostics (including connectivity probe and chat-completion results) and then enter an orderly halt or idle loop without starting additional services.

#### Scenario: No follow-on traffic

- **GIVEN** network diagnostics and chat-completion diagnostics have been printed
- **WHEN** boot completes
- **THEN** the image does not send further application traffic beyond what the diagnostics required
- **AND** behavior remains suitable for repeated smoke testing

### Requirement: Modular network boundaries

Network behavior SHALL be structured so that link-layer, IPv4, DHCP, connectivity-probe, TCP transport, and HTTP client concerns can be extended or replaced independently without rewriting unrelated layers.

#### Scenario: Layer replacement without cross-layer churn

- **GIVEN** a future change replaces the wired driver, adds a new upper-layer protocol, or swaps the HTTP client
- **WHEN** reviewers inspect the network implementation
- **THEN** each layer exposes a narrow interface to adjacent layers
- **AND** DHCP, connectivity-probe, and chat-completion logic do not embed hardware-specific details of unrelated devices
