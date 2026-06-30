# network-unikernel Specification

## Purpose
TBD - created by archiving change basic-network-integration. Update Purpose after archive.
## Requirements
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

After reporting its IPv4 address, the bootstrap image SHALL send exactly one ICMP echo request to the configured chat/inference host on the LAN—the same host used for HTTP chat-completion requests—and await a reply or failure within a bounded time.

#### Scenario: Probe succeeds

- **GIVEN** a valid IPv4 configuration and a reachable configured chat/inference host
- **WHEN** the probe completes
- **THEN** console output includes a short human-readable line indicating the configured host is reachable
- **AND** an observer can identify which host was probed from that line alone

#### Scenario: Probe fails

- **GIVEN** a valid IPv4 configuration
- **AND** the configured chat/inference host does not respond or the network blocks the probe
- **WHEN** the probe completes or times out
- **THEN** console output indicates failure
- **AND** console output includes a short reason category (for example timeout, unreachable, or no reply)

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

### Requirement: Modular network boundaries

Network behavior SHALL be structured so that link-layer, IPv4, DHCP, connectivity-probe, TCP transport, and HTTP client concerns can be extended or replaced independently without rewriting unrelated layers.

#### Scenario: Layer replacement without cross-layer churn

- **GIVEN** a future change replaces the wired driver, adds a new upper-layer protocol, or swaps the HTTP client
- **WHEN** reviewers inspect the network implementation
- **THEN** each layer exposes a narrow interface to adjacent layers
- **AND** DHCP, connectivity-probe, and chat-completion logic do not embed hardware-specific details of unrelated devices

### Requirement: Console cleared before reachability probe

Immediately before the connectivity probe runs, the bootstrap image SHALL clear the console display so prior boot and network diagnostic output is no longer visible on screen.

#### Scenario: Prior diagnostics not visible at probe time

- **GIVEN** DHCP completed successfully and the assigned address has been reported
- **WHEN** the connectivity probe is about to start
- **THEN** the console display no longer shows prior boot or network diagnostic lines
- **AND** subsequent reachability and chat output appear on the cleared display

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

