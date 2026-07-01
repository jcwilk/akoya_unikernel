## MODIFIED Requirements

### Requirement: CPU idle when quiescent

When no interface category has pending work and no scheduled timer is due within a short bounded horizon, the event runtime SHALL enter a low-power idle state until hardware activity or a timer indicates new work.

#### Scenario: Idle between chat turns

- **GIVEN** outbound transport for the completed chat turn is fully inactive
- **AND** the session is waiting at the input prompt with no key events pending
- **AND** no protocol or chat state machine requires immediate service
- **WHEN** the runtime finds no pending work
- **THEN** the guest enters an idle state rather than continuously polling at full CPU utilization
- **AND** the guest resumes promptly when operator input or wired activity occurs
