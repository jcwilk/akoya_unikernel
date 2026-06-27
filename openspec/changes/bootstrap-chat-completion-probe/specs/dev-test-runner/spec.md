## MODIFIED Requirements

### Requirement: Capturable console output

In headless mode, the run entry point SHALL attach emulated serial console output so bootstrap diagnostic messages are visible in the runner's standard output or log. In headful mode, console output SHALL remain observable through the display and/or serial attachment without requiring automated capture for pass/fail.

In headless mode, captured output SHALL include the assigned IPv4 address line, the connectivity probe result line, and the chat-completion diagnostic line.

#### Scenario: Headless diagnostic message assertion

- **GIVEN** a correctly built bootstrap image
- **WHEN** the run entry point completes successfully in headless mode
- **THEN** the captured output contains the bootstrap diagnostic message

#### Scenario: Headless network diagnostic assertion

- **GIVEN** a correctly built bootstrap image
- **AND** the workstation LAN provides DHCP and allows the connectivity probe to succeed
- **WHEN** the run entry point completes successfully in headless mode
- **THEN** the captured output contains the assigned IPv4 address reporting line
- **AND** the captured output contains a successful connectivity probe result with latency

#### Scenario: Headless chat-completion assertion

- **GIVEN** a correctly built bootstrap image
- **AND** the workstation LAN provides DHCP
- **AND** the configured chat-completions endpoint is reachable from the guest
- **WHEN** the run entry point completes successfully in headless mode
- **THEN** the captured output contains a chat-completion success indicator
- **AND** the captured output contains non-empty assistant reply text from the completion response

#### Scenario: Headful observation

- **GIVEN** a correctly built bootstrap image
- **WHEN** the run entry point is invoked in headful mode
- **THEN** an observer can read bootstrap, network, and chat-completion diagnostic output from the interactive session without relying on automated pass/fail gating

### Requirement: Bounded test runtime

In headless mode, the run entry point SHALL terminate within a bounded time when the image fails to produce expected diagnostic output, rather than hanging indefinitely. Headful mode SHALL NOT apply the same automated timeout gate.

The bounded time SHALL account for DHCP negotiation, the connectivity probe, and the chat-completion HTTP exchange in addition to console bring-up.

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
