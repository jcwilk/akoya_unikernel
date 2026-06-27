## MODIFIED Requirements

### Requirement: Capturable console output

In headless mode, the run entry point SHALL attach emulated serial console output so bootstrap diagnostic messages are visible in the runner's standard output or log. In headful mode, console output SHALL remain observable through the display and/or serial attachment without requiring automated capture for pass/fail.

In headless mode, captured output SHALL include the assigned IPv4 address line, the connectivity probe result line, and at least one chat-completion success line from the default or configured automated keyboard input sequence.

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
- **WHEN** the run entry point completes successfully in headless mode with default or configured scripted keyboard input
- **THEN** the captured output contains a chat-completion success indicator
- **AND** the captured output contains non-empty assistant reply text from at least one completion response

#### Scenario: Headful observation

- **GIVEN** a correctly built bootstrap image
- **WHEN** the run entry point is invoked in headful mode
- **THEN** an observer can read bootstrap, network, and interactive chat output from the interactive session without relying on automated pass/fail gating
- **AND** the operator can type follow-up messages on the emulated deployment-target integrated keyboard path

### Requirement: Bounded test runtime

In headless mode, the run entry point SHALL terminate within a bounded time when the image fails to produce expected diagnostic output, rather than hanging indefinitely. Headful mode SHALL NOT apply the same automated timeout gate.

The bounded time SHALL account for DHCP negotiation, the connectivity probe, scripted keyboard-driven chat input, and at least one chat-completion HTTP exchange in addition to console bring-up.

#### Scenario: Headless boot hang detection

- **GIVEN** a corrupt or non-booting image artifact
- **WHEN** the run entry point runs in headless mode
- **THEN** the runner exits with non-zero status within the configured timeout
- **AND** the failure mode indicates that expected console output was not observed

#### Scenario: Headful session control

- **GIVEN** the run entry point is invoked in headful mode
- **WHEN** the emulated machine is running
- **THEN** the session continues until the user or operator ends it
- **AND** no automated smoke-test timeout forces termination solely for pass/fail scoring while the operator is still in an interactive chat session

### Requirement: Guest shutdown behavior

The run entry point SHALL support configurable behavior for whether the emulator exits when the guest finishes its bootstrap work.

By default, headless invocations SHALL cause the emulator to exit after expected bootstrap, network, and scripted chat diagnostic output is observed. By default, headful invocations SHALL keep the emulator running until the operator ends the interactive chat session and closes the session manually.

The caller SHALL be able to override the default for either display mode with an explicit shutdown-behavior choice.

#### Scenario: Headless default auto-exit

- **GIVEN** a successful bootstrap build
- **WHEN** the run entry point is invoked in headless mode without a shutdown-behavior override
- **THEN** the emulator exits after scripted keyboard input produces expected diagnostic output and the guest halts
- **AND** the invocation can be used as an automated smoke test

#### Scenario: Headful default hold-open

- **GIVEN** a successful bootstrap build
- **WHEN** the run entry point is invoked in headful mode without a shutdown-behavior override
- **THEN** the emulator remains open while the operator uses the interactive chat session
- **AND** the operator can end the session and close the display when finished

#### Scenario: Override to hold on headless

- **GIVEN** a caller requests hold-open shutdown behavior on a headless invocation
- **WHEN** the run entry point starts emulation
- **THEN** the emulator does not automatically exit solely because the guest halted before scripted keyboard input completed

#### Scenario: Override to auto-exit on headful

- **GIVEN** a caller requests auto-exit shutdown behavior on a headful invocation
- **WHEN** the guest completes bootstrap diagnostics and ends the chat session
- **THEN** the emulator may exit without waiting for manual session termination

## ADDED Requirements

### Requirement: Scripted keyboard input for headless smoke tests

In headless mode, the run entry point SHALL inject predetermined keystrokes into the guest emulated deployment-target keyboard path so automated runs complete at least one chat exchange without physical keyboard input. Injection SHALL use the same guest keyboard input stack exercised on bare metal, not serial console receive as a substitute keyboard.

#### Scenario: Default scripted chat input

- **GIVEN** headless mode is selected and no custom input script is configured
- **WHEN** the guest enters the interactive chat session and signals readiness for input
- **THEN** the runner injects a default key sequence that submits at least one user message and then ends the session
- **AND** captured output includes a successful chat-completion line from that exchange

#### Scenario: Custom scripted input

- **GIVEN** headless mode with a configured multi-line keyboard input script
- **WHEN** the guest accepts keyboard input
- **THEN** the runner injects the configured key sequences in order
- **AND** each sequence is delivered through the emulated integrated keyboard path as if typed by an operator
