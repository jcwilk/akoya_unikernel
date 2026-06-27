## MODIFIED Requirements

### Requirement: Capturable console output

In headless mode, the run entry point SHALL attach emulated serial console output so bootstrap diagnostic messages are visible in the runner's standard output or log. In headful mode, console output SHALL remain observable through the display and/or serial attachment without requiring automated capture for pass/fail.

In headless mode, captured output SHALL include a successful reachability result for the configured chat/inference host, plain assistant reply text from at least one inference exchange, and evidence that the minimal chat input prompt appeared during the scripted session.

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

### Requirement: Scripted keyboard input for headless smoke tests

In headless mode, the run entry point SHALL inject predetermined keystrokes into the guest emulated deployment-target keyboard path so automated runs complete at least one chat exchange without physical keyboard input. Injection SHALL use the same guest keyboard input stack exercised on bare metal, not serial console receive as a substitute keyboard.

#### Scenario: Default scripted chat input

- **GIVEN** headless mode is selected and no custom input script is configured
- **WHEN** the guest enters the interactive chat session and signals readiness for input
- **THEN** the runner injects a default key sequence that submits at least one user message and then ends the session
- **AND** captured output includes plain assistant reply text from that exchange

#### Scenario: Custom scripted input

- **GIVEN** headless mode with a configured multi-line keyboard input script
- **WHEN** the guest accepts keyboard input
- **THEN** the runner injects the configured key sequences in order
- **AND** each sequence is delivered through the emulated integrated keyboard path as if typed by an operator
