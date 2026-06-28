## ADDED Requirements

### Requirement: Default automated verification exercises multi-turn chat

The project's default automated verification entry point SHALL run a multi-turn scripted chat regression that submits at least two non-exit user messages and asserts plain assistant reply text after each turn when the inference endpoint is reachable. Unexpected connection-failure outcome lines between successful turns SHALL cause non-zero exit.

#### Scenario: Default gate fails on turn-two connect regression

- **GIVEN** a reachable chat-completions endpoint
- **AND** the guest would print a connection-failure outcome on a follow-up turn despite a successful first turn
- **WHEN** the default automated verification entry point runs to completion
- **THEN** it exits with non-zero status
- **AND** failure output indicates an unexpected connection-failure outcome during multi-turn scripted interaction

#### Scenario: Default gate passes healthy multi-turn chat

- **GIVEN** a reachable chat-completions endpoint and a correctly functioning multi-turn transport lifecycle
- **WHEN** the default automated verification entry point runs to completion
- **THEN** captured output contains plain assistant reply text for at least two turns
- **AND** captured output does not contain connection-failure outcome lines between those successful turns

### Requirement: Transport verification does not substitute for multi-turn chat gate

Passing transport-only automated verification SHALL NOT satisfy the default multi-turn chat health gate. Assessment of multi-turn chat transport health SHALL require successful multi-turn scripted chat regression in addition to any transport-only verification when both are available.

#### Scenario: Transport pass does not excuse chat multi-turn fail

- **GIVEN** transport-only verification reports aggregate pass
- **AND** multi-turn scripted chat regression fails with a connection-failure outcome between turns
- **WHEN** assessing whether multi-turn chat transport is healthy
- **THEN** multi-turn chat health is not satisfied by transport verification alone
- **AND** the multi-turn chat regression failure remains blocking for chat health sign-off

## MODIFIED Requirements

### Requirement: Scripted keyboard input for headless smoke tests

In headless mode, the run entry point SHALL inject predetermined keystrokes into the guest emulated deployment-target keyboard path so automated runs complete multi-turn chat regression without physical keyboard input. The default automated verification entry point SHALL use a multi-turn script that submits at least two non-exit user messages before ending the session. Injection SHALL use the same guest keyboard input stack exercised on bare metal, not serial console receive as a substitute keyboard.

#### Scenario: Default scripted chat input

- **GIVEN** headless mode is selected and no custom input script is configured
- **WHEN** the guest enters the interactive chat session and signals readiness for input
- **THEN** the runner injects a default key sequence that submits at least two non-exit user messages and then ends the session
- **AND** captured output includes plain assistant reply text from each submitted turn when the inference endpoint is reachable
- **AND** captured output does not contain connection-failure outcome lines between successful turns

#### Scenario: Custom scripted input

- **GIVEN** headless mode with a configured multi-line keyboard input script
- **WHEN** the guest accepts keyboard input
- **THEN** the runner injects the configured key sequences in order
- **AND** each sequence is delivered through the emulated integrated keyboard path as if typed by an operator
