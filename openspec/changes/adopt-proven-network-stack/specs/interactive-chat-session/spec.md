## MODIFIED Requirements

### Requirement: Follow-up session turns without spurious connect failure

When the inference endpoint remains reachable and each prior turn reached fully inactive outbound transport before its turn boundary, the interactive chat session SHALL allow each follow-up non-exit user message to complete a full inference exchange and print an outcome without spurious connection-failure text caused by incomplete prior-turn transport release, connect retry masking, or loss of transport readiness during an idle wait at the input prompt.

#### Scenario: Follow-up after substantial idle at the input prompt

- **GIVEN** the operator received plain assistant reply text for a prior user message
- **AND** outbound transport for that turn was fully inactive before the minimal input prompt appeared
- **AND** the chat-completions endpoint remains reachable
- **AND** the operator waits at the minimal input prompt without submitting a new message for at least twenty seconds
- **WHEN** the operator submits a subsequent non-exit user message
- **THEN** the session runs a full inference exchange for that message
- **AND** console output includes plain assistant reply text or a categorized failure reflecting actual endpoint or transport-lifecycle behavior during that turn
- **AND** console output does not report a spurious connection-failure outcome attributable solely to the idle wait at the prompt
