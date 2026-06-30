## MODIFIED Requirements

### Requirement: Bounded timed idle between scheduled turns

The timed-gap chat regression boot image SHALL run without operator keyboard input. Between scheduled chat turns it SHALL wait at the input prompt for a configured duration using event-runtime scheduled timing rather than blocking the entire guest on keyboard availability or busy-waiting for the gap to elapse, mimicking operator pacing and headful idle-at-prompt conditions where no keyboard input occurs during the gap. The wait SHALL use guest millisecond timing that reflects the configured duration on deployment hardware and on CPU-faithful emulation within the tolerance defined by the guest-timekeeping capability.

#### Scenario: Timed gap at input prompt

- **GIVEN** a scheduled chat turn completed and outbound transport for that turn is fully inactive
- **WHEN** the next scheduled turn is pending
- **THEN** the image waits for a bounded configured duration at the input prompt without keyboard input
- **AND** outbound transport remains fully inactive throughout the timed idle gap
- **AND** the next scheduled turn begins only after the timed wait completes
- **AND** the elapsed gap is within ten percent of the configured duration on deployment CPU or CPU-faithful emulation

#### Scenario: No keyboard dependency between turns

- **GIVEN** the timed-gap chat regression schedule includes more than one chat turn
- **WHEN** the suite executes between consecutive turns
- **THEN** progress does not depend on integrated keyboard input or external serial typing
- **AND** the minimal interactive chat input prompt may appear during the timed idle gap as in the main session

#### Scenario: Gap does not busy-spin

- **GIVEN** a configured inter-turn idle gap is in progress
- **WHEN** no operator input and no unrelated protocol work is pending
- **THEN** the guest idles rather than continuously polling at full CPU utilization for the entire gap
- **AND** the gap duration still meets the configured tolerance when the next turn begins
