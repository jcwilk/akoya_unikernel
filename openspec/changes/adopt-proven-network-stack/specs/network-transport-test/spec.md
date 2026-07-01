## MODIFIED Requirements

### Requirement: Production network stack reuse

The transport-test boot image SHALL exercise the same production network stack used by the main chat unikernel for link bring-up, IPv4 configuration, DHCP, and outbound TCP transport. Transport scenarios SHALL run through the maintained embedded IPv4/TCP implementation shared with chat rather than through a substitute, mock, or project-local TCP state machine.

#### Scenario: Link and DHCP before scenarios

- **GIVEN** a supported wired or emulated Ethernet environment with a reachable DHCP server
- **WHEN** the transport-test image completes its network bootstrap
- **THEN** console output reports link readiness and assigned IPv4 configuration using the same diagnostic style as the main chat unikernel
- **AND** no scenario begins outbound TCP work before IPv4 configuration succeeds or is reported as failed

#### Scenario: TCP scenarios use production transport path

- **GIVEN** IPv4 configuration succeeded on the transport-test image
- **WHEN** a transport scenario performs outbound connection work
- **THEN** the scenario uses the maintained production TCP transport shared with the main chat unikernel
- **AND** console output does not claim success through a parallel mock or stub transport path
