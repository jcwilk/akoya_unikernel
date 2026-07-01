## ADDED Requirements

### Requirement: Maintained production IPv4 transport stack

The main chat unikernel and transport-test boot image SHALL use a maintained, widely deployed embedded IPv4 implementation for ARP resolution, DHCP, ICMP connectivity probing, and outbound TCP connections. The implementation SHALL NOT rely on a project-local single-connection TCP state machine for production chat or transport-test traffic.

#### Scenario: Follow-up TCP connection after idle at prompt

- **GIVEN** a successful first interactive chat turn with fully inactive outbound transport before the input prompt appeared
- **AND** the inference endpoint remains reachable
- **AND** the operator waits at the input prompt without submitting a new message for at least twenty seconds
- **WHEN** the operator submits a subsequent non-exit message
- **THEN** the image completes a new outbound TCP connection and HTTP exchange for that turn
- **AND** console output does not report a spurious connection-failure outcome attributable to stale in-house transport state from the prior turn

#### Scenario: Transport-test shares production stack

- **GIVEN** the transport-test boot image completed IPv4 configuration
- **WHEN** a scheduled outbound TCP scenario runs
- **THEN** it uses the same maintained IPv4/TCP implementation as the main chat unikernel
- **AND** success is not reported through a parallel mock or abbreviated TCP path

### Requirement: Driver boundary preserved

Wired Ethernet transmit and receive for the deployment-target NIC SHALL remain behind the existing device poll/send boundary. Upper-layer address resolution and TCP SHALL consume received frames and enqueue transmits through that boundary rather than duplicating hardware access in application code.

#### Scenario: Chat does not bypass the NIC device

- **GIVEN** the interactive chat session performs an inference exchange
- **WHEN** outbound and inbound frames move through the production path
- **THEN** hardware access is confined to the wired device layer and its adapter to the maintained stack
- **AND** HTTP chat logic does not program NIC registers directly

## MODIFIED Requirements

### Requirement: Modular network boundaries

Network behavior SHALL be structured so that the wired driver, the maintained IPv4 transport stack, connectivity probing, and HTTP chat client can be extended or replaced independently without rewriting unrelated layers. The maintained stack SHALL sit between the driver adapter and application chat logic.

#### Scenario: Layer replacement without cross-layer churn

- **GIVEN** a future change replaces the wired driver or HTTP client
- **WHEN** reviewers inspect the network implementation
- **THEN** each layer exposes a narrow interface to adjacent layers
- **AND** DHCP, connectivity-probe, and chat-completion logic do not embed hardware-specific details of unrelated devices
