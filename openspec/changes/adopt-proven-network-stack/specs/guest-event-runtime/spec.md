## MODIFIED Requirements

### Requirement: Active interface servicing

On each event-runtime iteration where work is pending, the runtime SHALL visit every active interface category—scheduled timers, operator input, wired receive and transmit, maintained-stack timer work, and in-flight chat state—so that no active category is starved long enough to overflow its bounded queue or miss a deadline.

#### Scenario: Keyboard and network both active

- **GIVEN** the operator is composing a line at the input prompt
- **AND** inbound wired frames are arriving or maintained-stack timers are due
- **WHEN** the runtime iterates
- **THEN** both operator input and wired network categories receive service before the iteration ends
- **AND** neither category is deferred until the other has fully blocked the guest

#### Scenario: Stack timers serviced during input wait

- **GIVEN** the interactive chat session shows the minimal input prompt after a completed turn
- **AND** outbound transport for that turn is fully inactive
- **WHEN** no key event is yet available
- **THEN** the runtime services maintained-stack timer and receive work on the same iteration cycle as the input wait
- **AND** follow-up inference turns are not denied solely because timer or receive servicing stopped during the idle wait
