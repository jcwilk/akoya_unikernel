# timed-gap-chat-regression Specification

## Purpose
TBD - created by archiving change strict-fail-closed-chat-transport. Update Purpose after archive.
## Requirements
### Requirement: Distinct timed-gap chat regression boot identity

The project SHALL provide a standalone boot image whose logical identity is distinct from the main interactive chat unikernel and from the network transport test runner. Observers and automated runners SHALL be able to tell which identity ran from console output and from runner selection semantics without inferring from incidental build artifacts alone.

#### Scenario: Identity visible at startup

- **GIVEN** the timed-gap chat regression boot image starts on a supported emulation or deployment target
- **WHEN** initial console output appears
- **THEN** output identifies the image as the timed-gap chat regression runner
- **AND** output does not present the main interactive chat session as the only boot mode without distinguishing this identity

#### Scenario: Runner selects timed-gap regression identity

- **GIVEN** the development workstation has built the main chat unikernel, the network transport test runner, and the timed-gap chat regression boot image
- **WHEN** an automated caller requests the timed-gap chat regression logical identity
- **THEN** the timed-gap chat regression image is the one started under emulation
- **AND** the main chat unikernel is not started unless explicitly selected

### Requirement: Production chat turn path reuse

The timed-gap chat regression boot image SHALL exercise the same production interactive chat turn lifecycle used by the main chat unikernel for outbound inference exchanges, including per-turn transport activation, chat-completion exchange, response extraction, and full transport release to inactive state before turn end. It SHALL NOT substitute a parallel mock HTTP client or abbreviated transport path for chat turns.

#### Scenario: Chat turns use production exchange path

- **GIVEN** IPv4 configuration succeeded on the timed-gap chat regression image
- **WHEN** a scheduled chat turn runs
- **THEN** the turn follows the same production chat-completion exchange lifecycle as the main interactive chat unikernel
- **AND** console output does not claim success through a substitute or stub chat path

#### Scenario: Per-turn inactive transport at turn end

- **GIVEN** a scheduled chat turn completed or failed
- **WHEN** the turn ends before the next scheduled turn or aggregate reporting
- **THEN** outbound transport for that turn is fully inactive
- **AND** no active outbound connection from that turn persists into the timed idle gap

### Requirement: Bounded timed idle between scheduled turns

The timed-gap chat regression boot image SHALL run without operator keyboard input. Between scheduled chat turns it SHALL wait at the input prompt using bounded timed blocking for a configured duration rather than waiting for human typing, mimicking operator pacing and headful idle-at-prompt conditions where no keyboard input occurs during the gap.

#### Scenario: Timed gap at input prompt

- **GIVEN** a scheduled chat turn completed and outbound transport for that turn is fully inactive
- **WHEN** the next scheduled turn is pending
- **THEN** the image waits for a bounded configured duration at the input prompt without keyboard input
- **AND** outbound transport remains fully inactive throughout the timed idle gap
- **AND** the next scheduled turn begins only after the timed wait completes

#### Scenario: No keyboard dependency between turns

- **GIVEN** the timed-gap chat regression schedule includes more than one chat turn
- **WHEN** the suite executes between consecutive turns
- **THEN** progress does not depend on integrated keyboard input or external serial typing
- **AND** the minimal interactive chat input prompt may appear during the timed idle gap as in the main session

### Requirement: Multi-turn schedule with per-turn and aggregate reporting

The timed-gap chat regression boot image SHALL execute a fixed, bounded schedule of at least three consecutive chat turns with a timed idle gap before each turn after the first. Each turn SHALL report pass or fail on the console in human-readable form reflecting whether the production chat turn completed with expected outcome categories. After all scheduled turns finish, the image SHALL print a single aggregate pass or fail summary and reach halt or idle suitable for automated capture.

#### Scenario: Three consecutive turns with idle gaps

- **GIVEN** IPv4 configuration succeeded and the configured chat-completions endpoint accepts connections
- **WHEN** the timed-gap chat regression schedule runs to completion
- **THEN** the image completes at least three consecutive chat turns
- **AND** each turn after the first is preceded by a bounded timed idle gap at the input prompt
- **AND** console output reports pass or fail for each turn in plain language

#### Scenario: Aggregate summary on success

- **GIVEN** every scheduled chat turn reported pass
- **WHEN** the schedule completes
- **THEN** console output includes a clear aggregate indication that all timed-gap chat regression turns passed
- **AND** the image reaches halt or idle without starting unrelated services

#### Scenario: Aggregate summary on failure

- **GIVEN** at least one scheduled chat turn reported fail, including transport-lifecycle or connection-failure outcomes between otherwise successful turns
- **WHEN** the schedule completes
- **THEN** console output includes a clear aggregate indication that one or more turns failed
- **AND** an automated runner can treat the run as failed from captured console output alone

#### Scenario: Endpoint unreachable fails required turns

- **GIVEN** IPv4 configuration succeeded on the timed-gap chat regression image
- **AND** the configured chat-completions endpoint does not accept outbound connections within the bounded wait period
- **WHEN** a scheduled chat turn that requires that endpoint runs
- **THEN** console output reports fail for that turn
- **AND** the turn is not omitted from the schedule or reported as pass due to skip

