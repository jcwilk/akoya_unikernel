## ADDED Requirements

### Requirement: Distinct transport-test boot identity

The project SHALL provide a standalone boot image whose logical identity is distinct from the main interactive chat unikernel. Observers and automated runners SHALL be able to tell which identity ran from console output and from runner selection semantics without inferring from incidental build artifacts alone.

#### Scenario: Identity visible at startup

- **GIVEN** the transport-test boot image starts on a supported emulation or deployment target
- **WHEN** initial console output appears
- **THEN** output identifies the image as the network transport test runner
- **AND** output does not present the interactive chat session input prompt used by the main chat unikernel

#### Scenario: Runner selects transport-test identity

- **GIVEN** the development workstation has built both the main chat unikernel and the transport-test boot image
- **WHEN** an automated caller requests the transport-test logical identity
- **THEN** the transport-test image is the one started under emulation
- **AND** the main chat unikernel is not started unless explicitly selected

### Requirement: Production network stack reuse

The transport-test boot image SHALL exercise the same production network stack used by the main chat unikernel for link bring-up, IPv4 configuration, DHCP, and outbound TCP transport. Transport scenarios SHALL run through those shared layers rather than through a substitute or mock stack.

#### Scenario: Link and DHCP before scenarios

- **GIVEN** a supported wired or emulated Ethernet environment with a reachable DHCP server
- **WHEN** the transport-test image completes its network bootstrap
- **THEN** console output reports link readiness and assigned IPv4 configuration using the same diagnostic style as the main chat unikernel
- **AND** no scenario begins outbound TCP work before IPv4 configuration succeeds or is reported as failed

#### Scenario: TCP scenarios use production transport path

- **GIVEN** IPv4 configuration succeeded on the transport-test image
- **WHEN** a transport scenario performs outbound connection work
- **THEN** the scenario uses the production TCP transport implementation shared with the main chat unikernel
- **AND** console output does not claim success through a parallel mock or stub transport path

### Requirement: Non-interactive timed execution

The transport-test boot image SHALL run without operator keyboard input and without entering the interactive chat session loop. Between scenario steps it SHALL wait using bounded timed delays or equivalent duration-blocking primitives rather than waiting for human typing.

#### Scenario: No chat input prompt

- **GIVEN** the transport-test image has finished network bootstrap
- **WHEN** the scenario suite executes
- **THEN** the minimal interactive chat input prompt never appears
- **AND** progress between steps does not depend on integrated keyboard input

#### Scenario: Delayed step progression

- **GIVEN** a scenario schedule includes a pause between two connection attempts
- **WHEN** the first attempt completes
- **THEN** the image waits for the configured duration before starting the next step
- **AND** the wait does not require external keystrokes or serial input

### Requirement: Bounded transport scenario suite

The transport-test image SHALL execute a fixed, bounded suite of transport scenarios designed to stress sequential connection lifecycle behavior. Each scenario SHALL report pass or fail on the console in human-readable form. The suite SHALL include at minimum: multiple sequential outbound connections to the same host, a delayed gap between connection attempts, and handling of unreachable endpoints where applicable.

#### Scenario: Sequential connections to same host

- **GIVEN** IPv4 configuration succeeded and the configured remote host accepts outbound connections
- **WHEN** the sequential-connection scenario runs
- **THEN** the image opens more than one outbound connection to the same host in order
- **AND** console output reports pass or fail for that scenario in plain language

#### Scenario: Delay between connections

- **GIVEN** a scenario that opens one connection, waits a configured interval, then opens another
- **WHEN** the scenario completes
- **THEN** console output indicates whether the delayed second connection succeeded
- **AND** the reported outcome is readable without parsing key=value diagnostic framing

#### Scenario: Unreachable endpoint handling

- **GIVEN** a scenario targets a host that does not accept connections within the bounded wait period
- **WHEN** the scenario completes
- **THEN** console output reports the scenario result as pass or fail according to the scenario's expected outcome category
- **AND** the image continues with remaining scenarios or proceeds to aggregate reporting rather than hanging indefinitely

### Requirement: Aggregate result and orderly completion

After all scenarios finish, the transport-test image SHALL print a single aggregate pass or fail summary on the console, then enter an orderly halt or idle state suitable for automated runners to capture output and exit.

#### Scenario: Aggregate summary on success

- **GIVEN** every scenario in the suite reported pass
- **WHEN** the suite completes
- **THEN** console output includes a clear aggregate indication that all transport scenarios passed
- **AND** the image reaches halt or idle without starting unrelated services

#### Scenario: Aggregate summary on failure

- **GIVEN** at least one scenario reported fail
- **WHEN** the suite completes
- **THEN** console output includes a clear aggregate indication that one or more scenarios failed
- **AND** an automated runner can treat the run as failed from captured console output alone

#### Scenario: Runner captures outcome headlessly

- **GIVEN** the transport-test image is run under headless emulation on the development workstation LAN path
- **WHEN** the guest reaches halt or idle after the suite
- **THEN** captured runner output includes the aggregate pass or fail summary
- **AND** the runner invocation can complete with non-zero status when aggregate failure is reported
