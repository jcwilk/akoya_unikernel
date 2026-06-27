## MODIFIED Requirements

### Requirement: QEMU smoke-test entry point

The project SHALL provide a single documented development-workstation entry point that runs a built boot image under emulation. The caller SHALL specify either headful or headless execution mode on every invocation; the entry point SHALL NOT assume a default mode when none is given.

When the selected boot image is the network-enabled bootstrap variant, the entry point SHALL attach an emulated wired Ethernet interface bridged to the development workstation LAN so the guest performs real DHCP against the same network as the workstation.

#### Scenario: Headless run after build

- **GIVEN** a successful bootstrap build on the development workstation
- **WHEN** the documented run entry point is invoked in headless mode
- **THEN** the boot image starts under emulation without an interactive display
- **AND** the process exits with success when the expected bootstrap diagnostic output is observed

#### Scenario: Headful run after build

- **GIVEN** a successful bootstrap build on the development workstation
- **WHEN** the documented run entry point is invoked in headful mode
- **THEN** the boot image starts under emulation with an interactive display on the development workstation
- **AND** console output remains available for observation

#### Scenario: Missing display mode

- **GIVEN** a caller invokes the run entry point without specifying headful or headless mode
- **WHEN** the entry point parses arguments
- **THEN** it exits immediately with a non-zero status
- **AND** the error explains that a display mode is required

#### Scenario: Conflicting display modes

- **GIVEN** a caller specifies both headful and headless mode
- **WHEN** the entry point parses arguments
- **THEN** it exits immediately with a non-zero status
- **AND** the error explains that exactly one display mode must be chosen

#### Scenario: Network image uses bridged LAN

- **GIVEN** the network-enabled bootstrap variant is selected
- **WHEN** the run entry point starts emulation
- **THEN** the guest wired interface is bridged to the workstation LAN
- **AND** DHCP requests from the guest can reach a server on that LAN

### Requirement: Capturable console output

In headless mode, the run entry point SHALL attach emulated serial console output so bootstrap diagnostic messages are visible in the runner's standard output or log. In headful mode, console output SHALL remain observable through the display and/or serial attachment without requiring automated capture for pass/fail.

When the network-enabled bootstrap variant is under test in headless mode, captured output SHALL include the assigned IPv4 address line and the connectivity probe result line.

#### Scenario: Headless diagnostic message assertion

- **GIVEN** a correctly built bootstrap image
- **WHEN** the run entry point completes successfully in headless mode
- **THEN** the captured output contains the bootstrap diagnostic message

#### Scenario: Headless network diagnostic assertion

- **GIVEN** a correctly built network-enabled bootstrap image
- **AND** the workstation LAN provides DHCP and allows the connectivity probe to succeed
- **WHEN** the run entry point completes successfully in headless mode
- **THEN** the captured output contains the assigned IPv4 address reporting line
- **AND** the captured output contains a successful connectivity probe result with latency

#### Scenario: Headful observation

- **GIVEN** a correctly built bootstrap image
- **WHEN** the run entry point is invoked in headful mode
- **THEN** an observer can read bootstrap diagnostic output from the interactive session without relying on automated pass/fail gating

### Requirement: Bounded test runtime

In headless mode, the run entry point SHALL terminate within a bounded time when the image fails to produce expected diagnostic output, rather than hanging indefinitely. Headful mode SHALL NOT apply the same automated timeout gate.

When the network-enabled bootstrap variant is under test, the bounded time SHALL account for DHCP negotiation and the connectivity probe in addition to console bring-up.

#### Scenario: Headless boot hang detection

- **GIVEN** a corrupt or non-booting image artifact
- **WHEN** the run entry point runs in headless mode
- **THEN** the runner exits with non-zero status within the configured timeout
- **AND** the failure mode indicates that expected console output was not observed

#### Scenario: Headful session control

- **GIVEN** the run entry point is invoked in headful mode
- **WHEN** the emulated machine is running
- **THEN** the session continues until the user or operator ends it
- **AND** no automated smoke-test timeout forces termination solely for pass/fail scoring

## ADDED Requirements

### Requirement: Stable emulated MAC address

When running the network-enabled bootstrap variant under emulation, the run entry point SHALL configure the guest wired interface with a fixed, project-defined MAC address on every invocation.

#### Scenario: Repeated runs reuse the same MAC

- **GIVEN** two consecutive headless runs of the network-enabled bootstrap variant on the same workstation
- **WHEN** each run starts emulation
- **THEN** the guest uses the same MAC address in both runs
- **AND** a DHCP server on the LAN can treat the guest as the same client across runs

### Requirement: Bridged networking prerequisites

When the network-enabled bootstrap variant requires bridged LAN access, the run entry point SHALL fail fast with an actionable error if bridged networking prerequisites on the workstation are not satisfied.

#### Scenario: Bridge unavailable

- **GIVEN** bridged networking is required for the selected image
- **AND** the configured bridge or permission prerequisites are missing
- **WHEN** the run entry point prepares emulation
- **THEN** it exits immediately with a non-zero status
- **AND** the error explains what the operator must configure on the workstation
