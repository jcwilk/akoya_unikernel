## ADDED Requirements

### Requirement: Console cleared before reachability probe

Immediately before the connectivity probe runs, the bootstrap image SHALL clear the console display so prior boot and network diagnostic output is no longer visible on screen.

#### Scenario: Prior diagnostics not visible at probe time

- **GIVEN** DHCP completed successfully and the assigned address has been reported
- **WHEN** the connectivity probe is about to start
- **THEN** the console display no longer shows prior boot or network diagnostic lines
- **AND** subsequent reachability and chat output appear on the cleared display

## MODIFIED Requirements

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
