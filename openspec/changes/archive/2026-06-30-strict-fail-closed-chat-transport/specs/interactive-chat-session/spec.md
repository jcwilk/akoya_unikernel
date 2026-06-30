## ADDED Requirements

### Requirement: Fail-closed incomplete transport release

When outbound transport for a completed inference turn cannot reach a fully inactive state before the turn boundary, the interactive chat session SHALL report a categorized transport-lifecycle failure for that turn. The session SHALL NOT print an assistant reply as if the turn succeeded, SHALL NOT show the minimal input prompt as if transport were fully released, and SHALL NOT proceed to the next turn by forcibly clearing transport state to simulate success.

#### Scenario: Incomplete teardown fails the turn categorically

- **GIVEN** an inference exchange for a submitted user message has finished or timed out
- **WHEN** outbound transport for that turn cannot reach fully inactive state before the turn boundary
- **THEN** console output reports a categorized transport-lifecycle failure for that turn in brief, readable form
- **AND** console output does not include plain assistant reply text as if the turn succeeded when transport remains incomplete
- **AND** the session does not show the minimal input prompt as if the prior turn fully released transport when it did not

#### Scenario: Idle input wait implies inactive transport

- **GIVEN** the session shows the minimal input prompt waiting for operator input between turns
- **WHEN** an observer assesses outbound transport state for the completed prior turn
- **THEN** outbound transport for the prior turn is fully inactive
- **AND** no active outbound connection or turn-specific transport state from the prior turn persists during the idle gap

### Requirement: No connect retry masking at turn start

When outbound transport is not fully inactive at the start of a new inference turn, the interactive chat session SHALL fail that turn with a categorized outcome reflecting incomplete prior-turn release or unreachable inactive state. The session SHALL NOT silently recover by repeating connection activation after forcibly clearing transport state from the prior turn.

#### Scenario: Stale transport fails before exchange

- **GIVEN** outbound transport from the prior turn is not fully inactive
- **WHEN** the operator submits a new non-exit user message
- **THEN** the new turn fails with a categorized transport-lifecycle or connection outcome reflecting the inactive-state violation
- **AND** console output does not report a spurious connection-failure outcome attributable solely to a masked retry after incomplete prior-turn release

## MODIFIED Requirements

### Requirement: Per-turn lifecycle before conversation UI resumes

For each operator-submitted user message that is not an exit command, the interactive chat session SHALL run the outbound inference exchange to completion, fully release outbound transport for that turn to a provably inactive state, and only then print the assistant reply or resume waiting for operator input. The session SHALL NOT accept a new operator message as the start of the next turn until the prior turn's transport is fully inactive and its outcome is printed. If full inactive state cannot be reached, the session SHALL fail the turn with a categorized transport-lifecycle outcome rather than proceeding by best-effort release or forced state clearing.

#### Scenario: Assistant reply follows provably inactive transport

- **GIVEN** the operator submitted a non-exit user message and the inference exchange completed successfully
- **WHEN** the assistant reply becomes available for that turn
- **THEN** outbound transport for that turn is fully inactive
- **AND** the reply is printed on the console as plain conversation text
- **AND** the session does not print the reply while outbound transport for that turn remains active or incompletely released

#### Scenario: Next input wait follows provably inactive transport

- **GIVEN** an inference turn completed for a submitted user message (success or categorized failure)
- **WHEN** the session returns to waiting for operator input
- **THEN** outbound transport for that turn is fully inactive
- **AND** no active outbound connection or turn-specific transport state remains from that turn
- **AND** the minimal input prompt appears only after transport is fully inactive for the completed turn

#### Scenario: Strict ordering within one turn

- **GIVEN** the operator submits a non-exit user message
- **WHEN** the turn proceeds
- **THEN** the sequence is: message submission, verify inactive transport, outbound transport activation, inference exchange through response extraction, full outbound transport release to inactive state, assistant outcome printed, session waits for next operator input
- **AND** the session does not accept a new operator message as the next turn until the prior turn's transport is fully inactive and its outcome is printed

#### Scenario: Transport-lifecycle failure blocks false success path

- **GIVEN** an inference exchange for a submitted user message completed but outbound transport cannot reach fully inactive state before the turn boundary
- **WHEN** the turn ends
- **THEN** console output reports a categorized transport-lifecycle failure
- **AND** the session does not print an assistant reply or show the next input prompt as if the turn succeeded

### Requirement: Follow-up session turns without spurious connect failure

When the inference endpoint remains reachable and each prior turn reached fully inactive outbound transport before its turn boundary, the interactive chat session SHALL allow each follow-up non-exit user message to complete a full inference exchange and print an outcome without spurious connection-failure text caused by incomplete prior-turn transport release or connect retry masking.

#### Scenario: Second message after successful first reply

- **GIVEN** the operator received plain assistant reply text for the first user message
- **AND** outbound transport for the first turn was fully inactive before the minimal input prompt appeared
- **AND** the chat-completions endpoint remains reachable
- **WHEN** the operator submits a second non-exit user message
- **THEN** the session runs a full inference exchange for that message beginning from fully inactive transport
- **AND** console output includes plain assistant reply text or a categorized failure reflecting actual endpoint or transport-lifecycle behavior during that turn
- **AND** console output does not report a spurious connection-failure outcome attributable to incomplete prior-turn transport release or masked connect retry

#### Scenario: Input wait does not mask incomplete prior-turn release

- **GIVEN** an inference turn completed for a submitted user message
- **WHEN** the session shows the minimal input prompt for the next operator message
- **THEN** outbound transport for the completed turn is fully inactive
- **AND** the session does not leave the operator at the input prompt while prior-turn outbound transport remains active, incompletely released, or forcibly cleared without reaching inactive state
