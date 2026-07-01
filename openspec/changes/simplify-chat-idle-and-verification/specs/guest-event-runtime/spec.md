## MODIFIED Requirements

### Requirement: Active interface servicing

On each event-runtime iteration where work is pending, the runtime SHALL visit every active interface category—scheduled timers, operator input, wired receive and transmit, and in-flight protocol or chat state—so that no active category is starved long enough to overflow its bounded queue or miss a deadline. While the interactive chat session waits at the input prompt, wired receive servicing SHALL not be artificially capped at a fixed small per-iteration frame count that can leave inbound readiness stale across a long idle wait.

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

#### Scenario: Network stays serviced during idle at prompt

- **GIVEN** the interactive chat session shows the minimal input prompt after a completed turn
- **AND** outbound transport for that turn is fully inactive
- **AND** no key event is yet available
- **WHEN** the runtime iterates during a long idle wait
- **THEN** wired receive is serviced on each iteration where the session remains at the input prompt
- **AND** follow-up inference turns are not denied solely because inbound readiness was not polled during the idle wait

### Requirement: CPU idle when quiescent

When no interface category has pending work and no scheduled timer is due within a short bounded horizon, the event runtime SHALL enter a low-power idle state until hardware activity or a timer indicates new work.

#### Scenario: Idle between chat turns

- **GIVEN** outbound transport for the completed chat turn is fully inactive
- **AND** the session is waiting at the input prompt with no key events pending
- **AND** no protocol or chat state machine requires immediate service
- **WHEN** the runtime finds no pending work
- **THEN** the guest enters an idle state rather than continuously polling at full CPU utilization
- **AND** the guest resumes promptly when operator input or wired activity occurs
