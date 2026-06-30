# dev-test-runner Specification

## Purpose
Pre-hardware validation by running built boot images under 32-bit x86 emulation on the development workstation, with explicit headful or headless modes, logical image selection, and smoke-test behavior for automation.
## Requirements
### Requirement: QEMU smoke-test entry point

The project SHALL provide a single documented development-workstation entry point that runs a built boot image under emulation. The caller SHALL specify either headful or headless execution mode on every invocation; the entry point SHALL NOT assume a default mode when none is given.

On every invocation, the entry point SHALL attach an emulated wired Ethernet interface directly to the development workstation LAN so the guest performs real DHCP and IP traffic against the same router and network as the workstation, without disrupting the host's own network connectivity.

#### Scenario: Headless run after build

- **GIVEN** a successful bootstrap build on the development workstation
- **WHEN** the documented run entry point is invoked in headless mode
- **THEN** the boot image starts under emulation without an interactive display
- **AND** the guest wired interface is on the workstation LAN
- **AND** the process exits with success when the expected bootstrap and network diagnostic output is observed

#### Scenario: Headful run after build

- **GIVEN** a successful bootstrap build on the development workstation
- **WHEN** the documented run entry point is invoked in headful mode
- **THEN** the boot image starts under emulation with an interactive display on the development workstation
- **AND** the guest wired interface is on the workstation LAN
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

#### Scenario: Guest reaches workstation router

- **GIVEN** the workstation LAN has a reachable DHCP server and router
- **WHEN** the run entry point starts emulation in either display mode
- **THEN** DHCP requests from the guest are visible on the workstation LAN
- **AND** the guest can obtain an address from the same network the workstation uses

### Requirement: Emulated target matches deployment word size

The QEMU runner SHALL emulate 32-bit x86 execution consistent with the deployment target, without requiring host hardware virtualization.

#### Scenario: Emulation without KVM dependency

- **GIVEN** the development host lacks usable hardware acceleration for the emulated target
- **WHEN** the QEMU test entry point runs
- **THEN** the smoke test still executes using software emulation
- **AND** the test remains suitable for cross-architecture workstations

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

### Requirement: Explicit boot image selection

The run entry point SHALL accept an optional path identifying which built boot image to run. When the path is omitted, the entry point SHALL resolve the image from the project's build output area by **logical image identity**, not by raw file count.

Co-emitted format variants of the same logical identity (e.g. linked and flat encodings of one build named `v1`) SHALL count as a single runnable candidate when determining whether exactly one image exists for auto-selection.

#### Scenario: Caller supplies image path

- **GIVEN** a valid boot image path is provided
- **WHEN** the run entry point is invoked in either display mode
- **THEN** that image is the one started under emulation

#### Scenario: Exactly one logical image in output area

- **GIVEN** no image path is provided
- **AND** the build output area contains exactly one logical runnable image
- **WHEN** the run entry point is invoked
- **THEN** that sole logical image is selected automatically

#### Scenario: Format variants do not create ambiguity

- **GIVEN** no image path is provided
- **AND** the build output area contains multiple on-disk artifacts that are format variants of the same logical image `v1`
- **WHEN** the run entry point resolves candidates
- **THEN** `v1` is counted once toward the exactly-one rule
- **AND** auto-selection proceeds as if only one logical image were present

#### Scenario: No runnable images

- **GIVEN** no image path is provided
- **AND** the build output area contains zero logical runnable images
- **WHEN** the run entry point is invoked
- **THEN** it exits immediately with a non-zero status
- **AND** the error explains that no runnable image was found and a build is required

#### Scenario: Multiple logical images

- **GIVEN** no image path is provided
- **AND** the build output area contains more than one logical runnable image
- **WHEN** the run entry point is invoked
- **THEN** it exits immediately with a non-zero status
- **AND** the error explains that the image path must be specified
- **AND** the error enumerates the ambiguous **logical identities** (not every format variant) so the caller can choose

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

### Requirement: Local workstation emulator

The run entry point SHALL invoke a 32-bit x86 system emulator installed on the development workstation. It SHALL fail fast with an actionable error when that emulator is not available.

The run entry point SHALL NOT rely on containerized emulator fallback.

#### Scenario: Emulator present

- **GIVEN** a host-installed 32-bit x86 system emulator is available
- **WHEN** the run entry point is invoked with prerequisites otherwise satisfied
- **THEN** emulation starts using the host installation

#### Scenario: Emulator absent

- **GIVEN** no suitable host-installed emulator is available
- **WHEN** the run entry point is invoked
- **THEN** it exits immediately with a non-zero status
- **AND** the error indicates that a host emulator must be installed

### Requirement: Stable emulated MAC address

When running the bootstrap image under emulation, the run entry point SHALL configure the guest wired interface with a fixed, project-defined MAC address on every invocation.

#### Scenario: Repeated runs reuse the same MAC

- **GIVEN** two consecutive runs of the bootstrap image on the same workstation
- **WHEN** each run starts emulation in either display mode
- **THEN** the guest uses the same MAC address in both runs
- **AND** a DHCP server on the LAN can treat the guest as the same client across runs

### Requirement: Workstation LAN networking prerequisites

The run entry point SHALL fail fast with an actionable error if LAN attachment prerequisites on the workstation are not satisfied.

#### Scenario: LAN attachment unavailable

- **GIVEN** the configured wired interface or permission prerequisites for LAN attachment are missing
- **WHEN** the run entry point prepares emulation in either display mode
- **THEN** it exits immediately with a non-zero status
- **AND** the error explains what the operator must configure on the workstation

### Requirement: Host connectivity preservation

The run entry point SHALL preserve the development workstation's usable network access during ephemeral LAN attachment setup, the emulation session, and teardown.

#### Scenario: Host remains online during test

- **GIVEN** the workstation had usable network access before the run entry point started
- **WHEN** the entry point completes setup, runs emulation, and tears down LAN attachment
- **THEN** the workstation retains usable network access throughout
- **AND** teardown restores the pre-test networking state for the designated wired interface

### Requirement: No isolated guest networking mode

The run entry point SHALL NOT offer a run path that connects the guest to an isolated or NAT-only virtual network instead of the workstation LAN.

#### Scenario: LAN attachment is mandatory

- **GIVEN** a caller invokes the run entry point with valid arguments
- **WHEN** emulation starts
- **THEN** the guest wired interface is on the workstation LAN
- **AND** no alternative user-mode or NAT-only networking mode is selected

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

### Requirement: Inference endpoint pre-flight before automated verification

Before starting headless automated verification that depends on the configured chat or inference host—including default smoke, timed-gap chat regression, scripted full-app interaction, and transport-test entry points—the development test runner SHALL verify that the configured inference endpoint is reachable from the development workstation. If the endpoint is unreachable, the runner SHALL exit immediately with non-zero status and actionable error output **without** starting emulation, **without** running partial scenarios, and **without** treating unreachability as an optional skip. A reachable inference endpoint is a required precondition for verification; unreachability indicates environment or configuration that must be corrected before verification proceeds.

#### Scenario: Abort before emulation when inference endpoint is down

- **GIVEN** the configured chat or inference endpoint is not reachable from the development workstation within the bounded pre-flight check
- **WHEN** an agent invokes any automated verification entry point that depends on that endpoint
- **THEN** the runner exits immediately with non-zero status
- **AND** emulation does not start
- **AND** no transport or chat scenario is reported as passed due to skip or omission

#### Scenario: Actionable error when inference infrastructure is unavailable

- **GIVEN** inference endpoint pre-flight fails
- **WHEN** the runner exits
- **THEN** error output states that the configured inference endpoint is unreachable
- **AND** error output indicates that inference availability must be restored before verification can proceed
- **AND** an operator or agent can identify the failure as an environment problem rather than a product regression

#### Scenario: Pre-flight passes when inference is healthy

- **GIVEN** the configured inference endpoint accepts connections from the development workstation
- **WHEN** an agent invokes a dependent automated verification entry point
- **THEN** pre-flight succeeds and emulation proceeds
- **AND** verification continues under the harness or smoke contract for that entry point

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

### Requirement: Default automated verification exercises multi-turn chat

The project's default automated verification entry point SHALL run timed-gap multi-turn chat regression that completes at least three consecutive chat turns with bounded timed idle gaps between turns when the inference endpoint is reachable. Pass SHALL require plain assistant reply text after each successful turn and SHALL treat unexpected connection-failure or transport-lifecycle failure outcome lines between successful turns as non-zero exit. The default gate SHALL use the timed-gap chat regression boot identity or an equivalent path that exercises the same production chat turn lifecycle with guest-side timed idle gaps, not host-side keyboard injection alone.

#### Scenario: Default gate fails on turn-two connect regression

- **GIVEN** a reachable chat-completions endpoint
- **AND** the guest would print a connection-failure or transport-lifecycle failure outcome on a follow-up turn despite a successful first turn
- **WHEN** the default automated verification entry point runs to completion
- **THEN** it exits with non-zero status
- **AND** failure output indicates an unexpected connection-failure or transport-lifecycle outcome during multi-turn timed-gap regression

#### Scenario: Default gate passes healthy multi-turn chat

- **GIVEN** a reachable chat-completions endpoint and a correctly functioning fail-closed multi-turn transport lifecycle
- **WHEN** the default automated verification entry point runs to completion
- **THEN** captured output contains plain assistant reply text for at least three consecutive turns
- **AND** captured output does not contain connection-failure or transport-lifecycle failure outcome lines between those successful turns

#### Scenario: Default gate exercises timed idle gaps

- **GIVEN** the default automated verification entry point runs with a reachable chat-completions endpoint
- **WHEN** regression completes successfully
- **THEN** the verification path included bounded timed idle gaps at the input prompt between consecutive chat turns
- **AND** success is not determined solely by immediate host-side keyboard injection without guest-side idle gaps

### Requirement: Transport verification does not substitute for multi-turn chat gate

Passing transport-only automated verification SHALL NOT satisfy the default multi-turn chat health gate. Assessment of multi-turn chat transport health SHALL require successful timed-gap multi-turn chat regression in addition to any transport-only verification when both are available. Passing timed-gap chat regression SHALL NOT excuse transport-only verification failures when transport stack health is separately required, and passing transport-only verification SHALL NOT excuse timed-gap chat regression failures.

#### Scenario: Transport pass does not excuse chat multi-turn fail

- **GIVEN** transport-only verification reports aggregate pass
- **AND** timed-gap multi-turn chat regression fails with a connection-failure or transport-lifecycle outcome between turns
- **WHEN** assessing whether multi-turn chat transport is healthy
- **THEN** multi-turn chat health is not satisfied by transport verification alone
- **AND** the timed-gap chat regression failure remains blocking for chat health sign-off

#### Scenario: Chat regression pass does not excuse transport-only fail when transport health is assessed

- **GIVEN** timed-gap multi-turn chat regression reports aggregate pass
- **AND** transport-only verification reports aggregate fail
- **WHEN** assessing whether raw transport stack scenarios are healthy
- **THEN** transport stack health is not satisfied by chat regression alone
- **AND** the transport-only failure remains blocking for transport stack sign-off when that assessment is required

### Requirement: Packaged ISO verification under emulation

The project SHALL provide a documented development-workstation entry point that boots the packaged ISO under 32-bit x86 emulation and verifies that the guest booted successfully and completed wired-network bring-up. Pass for this entry point SHALL require the bootstrap diagnostic message and a successful outbound connectivity probe to the build-configured probe target. It SHALL NOT require chat-completion exchange or inference-endpoint availability beyond what the connectivity probe itself needs.

#### Scenario: Headless ISO boot smoke test

- **GIVEN** a successfully produced boot ISO on the development workstation
- **AND** workstation emulation and LAN attachment prerequisites are satisfied
- **AND** the workstation LAN provides DHCP and allows the guest connectivity probe to succeed
- **WHEN** an agent or human invokes the documented ISO verification entry point in headless mode
- **THEN** the emulator boots from the ISO as optical media rather than loading a flat kernel image directly
- **AND** captured console output contains the bootstrap diagnostic message
- **AND** captured console output shows a successful outbound connectivity probe result for the build-configured probe target
- **AND** the process exits with success without running multi-turn chat regression

#### Scenario: ISO verification does not require inference endpoint

- **GIVEN** the configured chat or inference endpoint is unreachable from the development workstation
- **AND** the workstation LAN still allows the guest connectivity probe to succeed
- **WHEN** the ISO verification entry point runs in headless mode
- **THEN** verification may still pass when bootstrap and connectivity-probe assertions succeed
- **AND** failure is not reported solely because chat-completion infrastructure is unavailable

#### Scenario: ISO verification without existing ISO

- **GIVEN** no boot ISO artifact is present in the project build output area
- **WHEN** the ISO verification entry point is invoked
- **THEN** it produces the ISO via the documented packaging entry point before starting emulation
- **OR** it exits with non-zero status if packaging fails

#### Scenario: ISO boot failure is actionable

- **GIVEN** a corrupt or non-booting ISO artifact, or a booted guest that never reports connectivity-probe success
- **WHEN** the ISO verification entry point runs in headless mode
- **THEN** the runner exits with non-zero status within the configured timeout
- **AND** failure output indicates which expected bootstrap or connectivity-probe output was not observed after booting from the ISO

### Requirement: ISO verification preserves LAN-attached emulation

When verifying a packaged ISO, the development test runner SHALL attach the guest wired interface to the workstation LAN using the same mandatory LAN attachment behavior as direct-kernel runs.

#### Scenario: ISO boot on workstation LAN

- **GIVEN** ISO verification runs in headless mode with LAN prerequisites satisfied
- **WHEN** emulation starts from the packaged ISO
- **THEN** the guest wired interface is on the workstation LAN
- **AND** network diagnostic output remains observable in captured console output when the LAN provides DHCP

### Requirement: Unprivileged run and verification entry points

Documented development-workstation entry points for emulation and boot-media verification SHALL NOT invoke superuser privilege elevation during normal operation.

When workstation LAN attachment requires host capabilities the current user lacks, the entry point SHALL fail fast with actionable guidance for one-time workstation setup performed outside the script, rather than elevating privileges inside the script.

#### Scenario: Run entry point never elevates

- **GIVEN** an unprivileged user invokes a documented run or boot-media verification entry point
- **WHEN** the script executes
- **THEN** it does not invoke superuser privilege elevation or privilege-elevation wrappers
- **AND** emulation or verification proceeds when prerequisites are satisfied for the current user

#### Scenario: LAN setup unavailable without elevation

- **GIVEN** macvtap LAN attachment prerequisites are not satisfied for the unprivileged user
- **WHEN** a documented run entry point that requires LAN attachment is invoked
- **THEN** it exits immediately with non-zero status
- **AND** error output describes one-time workstation setup the operator must perform
- **AND** the script does not attempt superuser privilege elevation as a fallback

#### Scenario: Boot-media verification builds without elevation

- **GIVEN** a boot-media verification entry point is invoked and the packaged artifact is missing
- **WHEN** the verification flow triggers packaging
- **THEN** packaging runs without superuser privilege elevation

### Requirement: Timed-gap chat regression automated verification

The development test runner SHALL provide an agent-runnable entry point that builds or selects the timed-gap chat regression boot image, runs it headlessly on the workstation LAN emulation path, captures console output, and exits non-zero when the timed-gap aggregate result reports failure or when expected per-turn outcomes are absent.

#### Scenario: One-command timed-gap regression run

- **GIVEN** project sources and workstation emulation prerequisites are satisfied
- **WHEN** an agent invokes the documented timed-gap chat regression verification entry point without manual intermediate steps
- **THEN** the timed-gap chat regression boot image runs headlessly on the workstation LAN
- **AND** the entry point exits zero when captured output shows aggregate pass for at least three consecutive scheduled turns

#### Scenario: Timed-gap failure is actionable

- **GIVEN** a timed-gap chat regression run where at least one scheduled turn fails or aggregate result is fail
- **WHEN** the verification entry point completes
- **THEN** it exits with non-zero status
- **AND** captured or summarized output identifies the failure without requiring interactive inspection

#### Scenario: Distinct image selection for timed-gap regression

- **GIVEN** the main chat unikernel, network transport test runner, and timed-gap chat regression boot image are present in the build output area
- **WHEN** the timed-gap chat regression verification entry point runs
- **THEN** the timed-gap chat regression logical identity is selected automatically for that entry point
- **AND** the main chat unikernel and transport-test image are not started unless a separate caller explicitly selects them

