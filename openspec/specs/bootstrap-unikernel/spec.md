# bootstrap-unikernel Specification

## Purpose
TBD - created by archiving change akoya-unikernel-build-pipeline. Update Purpose after archive.
## Requirements
### Requirement: Bootstrap diagnostic message

The bootstrap unikernel image SHALL emit a short, fixed, human-readable diagnostic message during boot.

#### Scenario: Message visible on successful boot

- **GIVEN** a correctly built bootstrap image
- **WHEN** the image boots in a supported environment (emulated or bare metal)
- **THEN** the diagnostic message appears in console output
- **AND** the message identifies the image as an intentional bootstrap smoke test (not a silent hang)

### Requirement: Console output without external dependencies

The bootstrap image SHALL produce its initial diagnostic message using only bootstrapped console facilities available immediately after kernel handoff, without relying on filesystem mounts or user interaction. Network services SHALL be used only after that initial message is emitted.

#### Scenario: Standalone console-first boot

- **GIVEN** no disk filesystem is available after boot
- **WHEN** the bootstrap image starts
- **THEN** the initial diagnostic message is still emitted to console output before any network initialization begins

#### Scenario: Network follows initial message

- **GIVEN** a correctly built bootstrap image
- **WHEN** the image starts
- **THEN** the initial diagnostic message appears before DHCP or connectivity probes run

### Requirement: Deterministic bootstrap scope

The bootstrap image SHALL perform no application logic beyond initialization, console setup, emitting diagnostic output (including network diagnostics and interactive chat session activity) under the guest event runtime, and an orderly halt or idle loop after the operator ends the chat session.

#### Scenario: No hidden side effects

- **GIVEN** the bootstrap image is invoked for pipeline verification
- **WHEN** boot completes
- **THEN** no persistent services beyond boot-time diagnostics and the interactive chat session remain active
- **AND** no persistent mutations are made to attached storage
- **AND** behavior remains suitable for repeated smoke testing when the session is ended explicitly

#### Scenario: Network scope bounded to chat session

- **GIVEN** a correctly built bootstrap image
- **WHEN** the interactive chat session is active
- **THEN** network activity is limited to address acquisition, address reporting, a single connectivity probe against the configured chat/inference host, and HTTP chat-completion exchanges for submitted user lines
- **AND** no listening servers or unrelated retry loops continue outside active inference for a submitted line

#### Scenario: Event runtime is primary control flow

- **GIVEN** a correctly built bootstrap image after early console initialization
- **WHEN** network diagnostics or the interactive chat session are running
- **THEN** the guest event runtime is the primary application control flow coordinating that work
- **AND** the image does not run a separate parallel application loop for the same work

### Requirement: Build identity in diagnostic output

The bootstrap diagnostic output SHALL include a build identity sufficient to confirm which pipeline run produced the image.

#### Scenario: Distinguish rebuilds

- **GIVEN** two bootstrap images produced by different build runs
- **WHEN** each image boots and prints its diagnostic message
- **THEN** an observer can tell the two builds apart from the emitted output alone

### Requirement: Conversation-focused console handoff

After IPv4 configuration succeeds, the bootstrap image SHALL transition the console to a conversation-focused presentation by clearing prior boot output before reachability verification and presenting subsequent chat interaction with minimal, dialogue-oriented formatting.

#### Scenario: Chat session begins on a cleared display

- **GIVEN** IPv4 configuration succeeded
- **WHEN** the interactive chat session is about to accept operator input
- **THEN** prior boot and network trace lines are not visible on the console display
- **AND** the operator sees reachability confirmation followed by the minimal chat input prompt

