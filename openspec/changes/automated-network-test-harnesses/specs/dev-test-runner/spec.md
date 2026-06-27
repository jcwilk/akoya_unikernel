## ADDED Requirements

### Requirement: Scripted full-app interaction with output assertions

The development test runner SHALL support a host-side scripted interaction mode for the main interactive chat unikernel that combines timed keyboard injection with output assertions between steps. A script SHALL describe delays, text to type, optional session exit actions, and expected console patterns that must appear before the next step proceeds. The mode SHALL run non-interactively and report pass or fail with actionable output when an assertion is not satisfied.

#### Scenario: Single-turn reply assertion

- **GIVEN** headless emulation of the main chat unikernel with a reachable chat-completions endpoint
- **AND** a script that types a user message instructing a distinctive reply and asserts that reply appears in captured output
- **WHEN** the scripted interaction run completes
- **THEN** the runner reports pass only if the expected reply text appeared after the corresponding typing step
- **AND** failure output identifies which expected pattern was missing

#### Scenario: Reachability assertion before chat steps

- **GIVEN** a script that includes an assertion for the configured chat or inference host reachability line
- **WHEN** network bootstrap completes during the run
- **THEN** the runner verifies the reachability line appears in captured output before chat typing steps execute
- **AND** the run fails fast with actionable output if reachability is absent

#### Scenario: Multi-turn without connection failure lines

- **GIVEN** a script that submits more than one non-exit user message with assertions for plain assistant reply after each turn
- **WHEN** the scripted interaction run completes successfully
- **THEN** captured output contains plain assistant reply text for each asserted turn
- **AND** captured output does not contain a connection-failure outcome line between successful turns

#### Scenario: Non-interactive pass or fail exit

- **GIVEN** a complete scripted interaction definition
- **WHEN** an agent invokes the runner once in headless mode with that script
- **THEN** the process exits zero on pass and non-zero on failure without manual keyboard input
- **AND** failure output is sufficient to identify the failing step or missing pattern

### Requirement: Transport test image automated verification

The development test runner SHALL provide an agent-runnable entry point that builds or selects the transport-test boot image, runs it headlessly on the workstation LAN emulation path, captures console output, and exits non-zero when the transport-test aggregate result reports failure.

#### Scenario: One-command transport test run

- **GIVEN** project sources and workstation emulation prerequisites are satisfied
- **WHEN** an agent invokes the documented transport-test verification entry point without manual intermediate steps
- **THEN** the transport-test boot image runs headlessly on the workstation LAN
- **AND** the entry point exits zero when captured output shows aggregate pass

#### Scenario: Transport test failure is actionable

- **GIVEN** a transport-test run where at least one scenario fails
- **WHEN** the verification entry point completes
- **THEN** it exits with non-zero status
- **AND** captured or summarized output identifies that the transport suite failed without requiring interactive inspection

#### Scenario: Distinct image selection

- **GIVEN** both the main chat unikernel and transport-test boot image are present in the build output area
- **WHEN** the transport-test verification entry point runs
- **THEN** the transport-test logical identity is selected automatically for that entry point
- **AND** the main chat unikernel is not started unless a separate caller explicitly selects it

## MODIFIED Requirements

### Requirement: Capturable console output

In headless mode, the run entry point SHALL attach emulated serial console output so bootstrap diagnostic messages are visible in the runner's standard output or log. In headful mode, console output SHALL remain observable through the display and/or serial attachment without requiring automated capture for pass/fail.

In headless mode, captured output SHALL include a successful reachability result for the configured chat/inference host, plain assistant reply text from at least one inference exchange, and evidence that the minimal chat input prompt appeared during the scripted session.

When scripted interaction with output assertions is used for the main chat unikernel, captured output SHALL remain the source of truth for pass/fail, and assertion failures SHALL be reported without requiring an operator to read an interactive display.

#### Scenario: Headless diagnostic message assertion

- **GIVEN** a correctly built bootstrap image
- **WHEN** the run entry point completes successfully in headless mode
- **THEN** the captured output contains the bootstrap diagnostic message

#### Scenario: Headless network diagnostic assertion

- **GIVEN** a correctly built bootstrap image
- **AND** the workstation LAN provides DHCP
- **AND** the configured chat/inference host responds to the connectivity probe
- **WHEN** the run entry point completes successfully in headless mode
- **THEN** the captured output contains a short human-readable line indicating the configured chat/inference host is reachable
- **AND** the captured output does not require parsing key=value probe status framing to determine success

#### Scenario: Headless chat-completion assertion

- **GIVEN** a correctly built bootstrap image
- **AND** the workstation LAN provides DHCP
- **AND** the configured chat-completions endpoint is reachable from the guest
- **WHEN** the run entry point completes successfully in headless mode with default or configured scripted keyboard input
- **THEN** the captured output contains non-empty assistant reply text from at least one completion response on its own line(s)
- **AND** the captured output does not rely on key=value chat-completion status framing to identify a successful turn

#### Scenario: Headful observation

- **GIVEN** a correctly built bootstrap image
- **WHEN** the run entry point is invoked in headful mode
- **THEN** an observer can read bootstrap, network, and interactive chat output from the interactive session without relying on automated pass/fail gating
- **AND** the operator can type follow-up messages on the emulated deployment-target integrated keyboard path

#### Scenario: Assertion harness uses captured output

- **GIVEN** headless scripted interaction with output assertions for the main chat unikernel
- **WHEN** an expected pattern is absent from captured console output at an assertion step
- **THEN** the run fails without opening an interactive display
- **AND** the failure report references the missing expected content in human-readable form

### Requirement: Headless multi-turn smoke coverage with per-turn transport

Headless smoke tests SHALL remain able to validate multi-turn chat without manual operator input. Pass and fail assertions SHALL continue to use the existing plain-reply, reachability, and minimal-input-prompt contracts and SHALL NOT require persistent outbound transport across scripted turns.

When a multi-step scripted interaction definition includes per-turn output assertions, pass SHALL require each asserted plain reply and SHALL treat connection-failure outcome lines between successful turns as failure unless the script explicitly expects them.

#### Scenario: Default scripted session still completes an exchange

- **GIVEN** headless mode with default scripted keyboard input and a reachable chat-completions endpoint
- **WHEN** the run entry point completes successfully
- **THEN** captured output contains non-empty plain assistant reply text from at least one inference exchange on its own line(s)
- **AND** captured output includes evidence that the minimal chat input prompt appeared during the scripted session
- **AND** success is determined without requiring transport to remain active between scripted submissions

#### Scenario: Multi-turn script uses the same assertion contract

- **GIVEN** headless mode with a configured multi-line keyboard input script that submits more than one non-exit user message
- **WHEN** the run entry point completes successfully
- **THEN** captured output may contain assistant reply text from multiple turns
- **AND** pass/fail still relies on plain reply text and reachability output rather than transport persistence or key=value chat status framing

#### Scenario: Multi-turn assertion detects connection failure between turns

- **GIVEN** headless scripted interaction with per-turn reply assertions for more than one user message
- **AND** the guest prints a connection-failure outcome after the first successful turn
- **WHEN** the run completes
- **THEN** the runner reports failure
- **AND** failure output indicates that an unexpected connection-failure outcome appeared during the multi-turn script
