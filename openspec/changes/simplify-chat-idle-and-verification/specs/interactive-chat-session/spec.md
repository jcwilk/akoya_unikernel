## MODIFIED Requirements

### Requirement: Follow-up session turns without spurious connect failure

When the inference endpoint remains reachable and each prior turn reached fully inactive outbound transport before its turn boundary, the interactive chat session SHALL allow each follow-up non-exit user message to complete a full inference exchange and print an outcome without spurious connection-failure text caused by incomplete prior-turn transport release, connect retry masking, or loss of wired-network readiness during an idle wait at the input prompt.

#### Scenario: Second message after successful first reply

- **GIVEN** the operator received plain assistant reply text for the first user message
- **AND** outbound transport for the first turn was fully inactive before the minimal input prompt appeared
- **AND** the chat-completions endpoint remains reachable
- **WHEN** the operator submits a second non-exit user message
- **THEN** the session runs a full inference exchange for that message beginning from fully inactive transport
- **AND** console output includes plain assistant reply text or a categorized failure reflecting actual endpoint or transport-lifecycle behavior during that turn
- **AND** console output does not report a spurious connection-failure outcome attributable to incomplete prior-turn transport release or masked connect retry

#### Scenario: Follow-up after substantial idle at the input prompt

- **GIVEN** the operator received plain assistant reply text for a prior user message
- **AND** outbound transport for that turn was fully inactive before the minimal input prompt appeared
- **AND** the chat-completions endpoint remains reachable
- **AND** the operator waits at the minimal input prompt without submitting a new message for a substantial period
- **WHEN** the operator submits a subsequent non-exit user message
- **THEN** the session runs a full inference exchange for that message
- **AND** console output includes plain assistant reply text or a categorized failure reflecting actual endpoint or transport-lifecycle behavior during that turn
- **AND** console output does not report a spurious connection-failure outcome attributable solely to the idle wait at the prompt

#### Scenario: Input wait does not mask incomplete prior-turn release

- **GIVEN** an inference turn completed for a submitted user message
- **WHEN** the session shows the minimal input prompt for the next operator message
- **THEN** outbound transport for the completed turn is fully inactive
- **AND** the session does not leave the operator at the input prompt while prior-turn outbound transport remains active, incompletely released, or forcibly cleared without reaching inactive state

### Requirement: Event-driven input wait at prompt

While the interactive chat session waits for operator input at the minimal input prompt, the session SHALL accept integrated keyboard events through the guest event runtime without blocking the entire guest on key availability. The runtime SHALL continue servicing wired network receive and transmit progress needed to keep the guest ready for a new inference turn. Outbound transport for the prior completed turn SHALL remain fully inactive during this wait unless a new turn has begun.

#### Scenario: Prompt visible while runtime services other work

- **GIVEN** the session shows the minimal input prompt after a completed turn
- **AND** outbound transport for that turn is fully inactive
- **WHEN** no key event is yet available
- **THEN** the guest event runtime services wired network work on the same iteration cycle as the input wait
- **AND** the minimal input prompt remains the active input wait state

#### Scenario: Key events still compose the current line

- **GIVEN** the session is in event-driven input wait at the minimal input prompt
- **WHEN** the operator produces US-layout printable characters, Backspace, or Enter on the integrated keyboard
- **THEN** the pending line is edited and submitted with the same semantics as before this change
- **AND** console echo reflects the current line state
