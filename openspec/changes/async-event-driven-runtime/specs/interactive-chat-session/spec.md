## ADDED Requirements

### Requirement: Event-driven input wait at prompt

While the interactive chat session waits for operator input at the minimal input prompt, the session SHALL accept integrated keyboard events through the guest event runtime without blocking the entire guest on key availability. Outbound transport for the prior completed turn SHALL remain fully inactive during this wait unless a new turn has begun.

#### Scenario: Prompt visible while runtime services other work

- **GIVEN** the session shows the minimal input prompt after a completed turn
- **AND** outbound transport for that turn is fully inactive
- **WHEN** no key event is yet available
- **THEN** the guest event runtime may service wired network and timer work without requiring operator input
- **AND** the minimal input prompt remains the active input wait state

#### Scenario: Key events still compose the current line

- **GIVEN** the session is in event-driven input wait at the minimal input prompt
- **WHEN** the operator produces US-layout printable characters, Backspace, or Enter on the integrated keyboard
- **THEN** the pending line is edited and submitted with the same semantics as before this change
- **AND** console echo reflects the current line state

## MODIFIED Requirements

### Requirement: Per-turn lifecycle before conversation UI resumes

For each operator-submitted user message that is not an exit command, the interactive chat session SHALL advance the outbound inference exchange through the guest event runtime until the exchange completes, fully release outbound transport for that turn to a provably inactive state, and only then print the assistant reply or resume waiting for operator input. The session SHALL NOT accept a new operator message as the start of the next turn until the prior turn's transport is fully inactive and its outcome is printed. If full inactive state cannot be reached, the session SHALL fail the turn with a categorized transport-lifecycle outcome rather than proceeding by best-effort release or forced state clearing.

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

#### Scenario: Input wait does not defer transport release

- **GIVEN** an inference turn is completing outbound transport release
- **WHEN** the session is already showing or about to show the minimal input prompt for the next operator message
- **THEN** transport release for the completing turn continues to advance through the event runtime during input wait
- **AND** the session does not require operator input before finishing release for the prior turn
