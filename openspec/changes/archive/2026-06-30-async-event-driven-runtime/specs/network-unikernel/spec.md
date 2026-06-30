## ADDED Requirements

### Requirement: Event-driven network bootstrap

Network bootstrap—including link bring-up, address acquisition, address reporting, console transition, connectivity probing, and handoff to the interactive chat session—SHALL advance through the guest event runtime without blocking the entire guest on wired activity or millisecond deadlines. Console-visible ordering and outcomes SHALL match the existing bootstrap contract.

#### Scenario: DHCP progresses without guest-wide blocking

- **GIVEN** a DHCP server is reachable on the attached LAN
- **WHEN** address acquisition is in progress
- **THEN** the guest event runtime continues to service timers and wired receive work incrementally
- **AND** a successful lease still results in a usable IPv4 configuration within the configured wait period

#### Scenario: Connectivity probe uses the same runtime

- **GIVEN** DHCP completed successfully and the assigned address has been reported
- **WHEN** the single outbound connectivity probe runs
- **THEN** the probe advances through the event runtime
- **AND** console output still reports reachability or failure with a short reason category before chat begins

## MODIFIED Requirements

### Requirement: Orderly completion after network diagnostics

The bootstrap image SHALL finish its boot-time work after the interactive chat session ends under the guest event runtime and then enter an orderly halt or idle loop without starting additional application services beyond the event runtime itself.

#### Scenario: No follow-on traffic after session ends

- **GIVEN** network diagnostics have been printed and the operator ended the interactive chat session
- **WHEN** boot completes
- **THEN** the image does not send further application traffic beyond what diagnostics and the chat session required
- **AND** behavior remains suitable for repeated smoke testing when sessions end explicitly

#### Scenario: Event runtime is not an extra background service

- **GIVEN** the interactive chat session has ended
- **WHEN** the image completes boot-time work
- **THEN** the guest event runtime stops scheduling application work
- **AND** the image does not continue unrelated retry or polling loops outside the documented halt or idle state
