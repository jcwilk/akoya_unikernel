## MODIFIED Requirements

### Requirement: Emulated target matches deployment word size

The development test runner SHALL emulate 32-bit x86 execution consistent with the deployment target CPU class declared in the deployment profile, without requiring host hardware virtualization. Emulated CPU selection SHALL prefer the deployment Pentium M class over generic earlier-generation stand-ins when the host emulator supports a faithful model.

#### Scenario: Emulation without KVM dependency

- **GIVEN** the development host lacks usable hardware acceleration for the emulated target
- **WHEN** the development test runner executes
- **THEN** smoke and regression tests still run using software emulation
- **AND** the tests remain suitable for cross-architecture workstations

#### Scenario: CPU class matches deployment profile

- **GIVEN** the deployment profile declares Pentium M class CPU
- **WHEN** emulation starts for any headless or headful run path
- **THEN** the emulated CPU is selected from the profile declaration rather than a hardcoded unrelated model
- **AND** documented overrides remain available for deliberate experiments without changing the default

### Requirement: Capturable console output

In headless mode, the run entry point SHALL attach emulated serial console output so bootstrap diagnostic messages are visible in the runner's standard output or log. In headful mode, console output SHALL remain observable through the display and/or serial attachment without requiring automated capture for pass/fail.

In headless mode, captured output SHALL include a successful reachability result for the configured chat/inference host, plain assistant reply text from at least one inference exchange, and evidence that the minimal chat input prompt appeared during the scripted session.

When scripted interaction with output assertions is used for the main chat unikernel, captured output SHALL remain the source of truth for pass/fail, and assertion failures SHALL be reported without requiring an operator to read an interactive display.

Headless mode SHALL continue to support serial console capture as the primary automation channel. Post-network screen clearing in the guest SHALL remain permitted.

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

#### Scenario: Headless serial remains available

- **GIVEN** headless emulation of any boot image
- **WHEN** the guest prints bootstrap or chat diagnostics
- **THEN** serial-attached capture receives the same text the automation harness asserts against
- **AND** headless runs do not require an interactive display for pass or fail determination

## ADDED Requirements

### Requirement: Headful emulation deployment fidelity

In headful mode, the development test runner SHALL present a standard VGA text console consistent with early bare-metal boot on the deployment target, and SHALL route operator keyboard input through the emulated integrated PS/2 keyboard path rather than substituting serial stdin for typing.

#### Scenario: Headful operator uses integrated keyboard path

- **GIVEN** headful emulation with prerequisites satisfied
- **WHEN** an operator types in the interactive display window
- **THEN** keystrokes reach the guest through the emulated PS/2 keyboard stack
- **AND** behavior matches the bare-metal integrated keyboard contract for interactive chat

#### Scenario: Headful VGA text console

- **GIVEN** headful emulation starts successfully
- **WHEN** the guest initializes its console
- **THEN** the operator sees VGA text output in the interactive display
- **AND** serial capture may run in parallel without replacing display output

### Requirement: Deploy-faithful boot verification tier

The project SHALL document a deploy-faithful verification entry point that boots packaged removable-media images through BIOS firmware and bootloader handoff, distinct from direct multiboot kernel loading. Pre-flash sign-off SHALL require successful deploy-faithful verification in addition to fast direct-multiboot regression when both entry points are available on the workstation.

#### Scenario: USB image boots through firmware path

- **GIVEN** a prepared USB or HDD disk image from project packaging
- **WHEN** deploy-faithful verification runs headlessly with LAN prerequisites satisfied
- **THEN** the emulator boots from the disk image as a legacy hard disk rather than loading a flat kernel directly
- **AND** captured output satisfies the existing bootstrap and connectivity probe contract for that entry point

#### Scenario: Fast tier does not replace deploy tier

- **GIVEN** timed-gap chat regression passes via direct multiboot loading
- **WHEN** assessing readiness to flash deployment media
- **THEN** deploy-faithful verification success is still required
- **AND** documentation states both tiers and their roles

#### Scenario: Deploy tier documented for agents

- **GIVEN** an agent or operator consults project documentation for verification entry points
- **WHEN** they assess pre-flash requirements
- **THEN** deploy-faithful verification is listed alongside fast regression
- **AND** the distinction between direct multiboot smoke and firmware boot is explicit
