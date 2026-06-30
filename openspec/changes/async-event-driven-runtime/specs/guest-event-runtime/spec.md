## ADDED Requirements

### Requirement: Single cooperative event runtime

After early console initialization, the bootstrap image SHALL run all bootstrap diagnostics, interactive chat session work, and orderly shutdown preparation under one cooperative event runtime. The runtime SHALL advance work incrementally on each iteration and SHALL remain the sole application control flow until the image enters its post-session halt or idle state.

#### Scenario: Chat and network share one runtime

- **GIVEN** IPv4 configuration succeeded and the interactive chat session is active
- **WHEN** the operator is waiting at the input prompt and inbound wired frames or outbound transport progress are pending
- **THEN** the same event runtime services keyboard input, wired network progress, and chat turn state within the same iteration cycle
- **AND** the image does not delegate chat or network work to a separate background execution context

#### Scenario: Orderly halt follows runtime completion

- **GIVEN** the operator ended the interactive chat session
- **WHEN** the image finishes session teardown work
- **THEN** the event runtime stops scheduling new application work
- **AND** the image proceeds to its documented halt or idle state

### Requirement: Non-blocking execution policy

The event runtime SHALL NOT block waiting for operator input, inbound wired data, outbound transport progress, or the expiry of a scheduled millisecond deadline. Work that cannot finish immediately SHALL be deferred and resumed on a later iteration when the relevant interface or timer indicates readiness.

#### Scenario: No spin-wait for keyboard input

- **GIVEN** the interactive chat session is waiting for operator input at the minimal input prompt
- **WHEN** no key event is available
- **THEN** the runtime continues other scheduled work on the same iteration or subsequent iterations
- **AND** the guest does not busy-spin solely to wait for a key press

#### Scenario: No spin-wait for network progress

- **GIVEN** an outbound chat-completion exchange is in progress
- **WHEN** the wired interface has no new inbound data and outbound transmission is not yet complete
- **THEN** the runtime yields to other active interfaces and scheduled timers rather than spinning until data arrives
- **AND** the exchange resumes when the wired interface or timer indicates progress

#### Scenario: Immediate hardware operations remain bounded

- **GIVEN** the runtime must perform a wired or keyboard controller operation that completes in bounded immediate time
- **WHEN** that operation is required to acknowledge hardware or enqueue an event
- **THEN** the runtime may stall only for the duration of that immediate operation
- **AND** the stall does not wait for operator input, frame arrival, or a millisecond deadline to elapse

### Requirement: Active interface servicing

On each event-runtime iteration where work is pending, the runtime SHALL visit every active interface category—scheduled timers, operator input, wired receive and transmit, and in-flight protocol or chat state—so that no active category is starved long enough to overflow its bounded queue or miss a deadline.

#### Scenario: Keyboard and network both active

- **GIVEN** the operator is composing a line at the input prompt
- **AND** inbound wired frames are arriving for an in-flight or completing transport exchange
- **WHEN** the runtime iterates
- **THEN** both operator input and wired network categories receive service before the iteration ends
- **AND** neither category is deferred until the other has fully blocked the guest

#### Scenario: Timer due during input wait

- **GIVEN** a scheduled millisecond deadline is due during an input wait
- **WHEN** the runtime iterates
- **THEN** the timer category is serviced in the same iteration cycle as operator input and wired network categories that are active
- **AND** the deadline is not missed solely because the guest was waiting for operator input

### Requirement: CPU idle when quiescent

When no interface category has pending work and no scheduled timer is due within a short bounded horizon, the event runtime SHALL enter a low-power idle state until hardware activity or a timer indicates new work.

#### Scenario: Idle between chat turns

- **GIVEN** outbound transport for the completed chat turn is fully inactive
- **AND** the session is waiting at the input prompt with no key events pending
- **AND** no protocol or chat state machine requires immediate service
- **WHEN** the runtime finds no pending work
- **THEN** the guest enters an idle state rather than continuously polling at full CPU utilization
- **AND** the guest resumes promptly when operator input or wired activity occurs

#### Scenario: Idle during timed regression gap

- **GIVEN** the timed-gap chat regression image is in a configured inter-turn idle period
- **AND** outbound transport for the prior turn is fully inactive
- **WHEN** no other work is pending
- **THEN** the guest idles rather than busy-waiting for the gap duration
- **AND** the next scheduled turn begins when the configured gap elapses

### Requirement: Hardware wakeup for input and wired network

The event runtime SHALL integrate hardware signaling for operator keyboard input and wired network activity so that idle periods end when new input or wired work arrives, without requiring continuous polling for those devices.

#### Scenario: Keyboard wakeup from idle

- **GIVEN** the guest is idle waiting for operator input
- **WHEN** the operator presses a key on the integrated keyboard path
- **THEN** the runtime resumes and the key event is available for the pending line editor
- **AND** the resume does not depend on an external serial typing device

#### Scenario: Wired network wakeup from idle

- **GIVEN** the guest is idle while an outbound exchange awaits a response
- **WHEN** an inbound wired frame for the active exchange arrives
- **THEN** the runtime resumes and the exchange state machine advances
- **AND** the resume does not require the operator to provide input
