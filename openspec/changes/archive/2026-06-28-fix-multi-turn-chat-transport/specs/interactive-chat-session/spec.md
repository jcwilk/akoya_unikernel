## ADDED Requirements

### Requirement: Follow-up session turns without spurious connect failure

When the inference endpoint remains reachable, the interactive chat session SHALL allow each follow-up non-exit user message to complete a full inference exchange and print an outcome without spurious connection-failure text caused by incomplete prior-turn transport release.

#### Scenario: Second message after successful first reply

- **GIVEN** the operator received plain assistant reply text for the first user message
- **AND** the session returned to waiting for operator input with the minimal input prompt
- **AND** the chat-completions endpoint remains reachable
- **WHEN** the operator submits a second non-exit user message
- **THEN** the session runs a full inference exchange for that message
- **AND** console output includes plain assistant reply text or a categorized failure reflecting actual endpoint behavior during that turn
- **AND** console output does not report a spurious connection-failure outcome attributable to prior-turn transport not being fully released

#### Scenario: Input wait does not mask incomplete prior-turn teardown

- **GIVEN** an inference turn completed for a submitted user message
- **WHEN** the session shows the minimal input prompt for the next operator message
- **THEN** outbound transport for the completed turn is fully released
- **AND** the session does not leave the operator blocked at the input prompt while prior-turn outbound transport remains active or incomplete

## MODIFIED Requirements

### Requirement: Per-turn lifecycle before conversation UI resumes

For each operator-submitted user message that is not an exit command, the interactive chat session SHALL run the outbound inference exchange to completion, fully release outbound transport for that turn, and only then print the assistant reply or resume waiting for operator input. The session SHALL NOT accept a new operator message as the start of the next turn until the prior turn's transport is fully released and its outcome is printed.

#### Scenario: Assistant reply follows transport release

- **GIVEN** the operator submitted a non-exit user message and the inference exchange completed successfully
- **WHEN** the assistant reply becomes available for that turn
- **THEN** outbound transport for that turn is no longer active
- **AND** the reply is printed on the console as plain conversation text
- **AND** the session does not print the reply while outbound transport for that turn remains active

#### Scenario: Next input wait follows transport release

- **GIVEN** an inference turn completed for a submitted user message (success or failure)
- **WHEN** the session returns to waiting for operator input
- **THEN** outbound transport for that turn is fully released
- **AND** no active outbound connection or turn-specific transport state remains from that turn
- **AND** the minimal input prompt appears only after transport release for the completed turn

#### Scenario: Strict ordering within one turn

- **GIVEN** the operator submits a non-exit user message
- **WHEN** the turn proceeds
- **THEN** the sequence is: message submission, outbound transport activation, inference exchange through response extraction, full outbound transport release, assistant outcome printed, session waits for next operator input
- **AND** the session does not accept a new operator message as the next turn until the prior turn's transport is fully released and its outcome is printed
