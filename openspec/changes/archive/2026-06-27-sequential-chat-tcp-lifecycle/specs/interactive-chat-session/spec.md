## ADDED Requirements

### Requirement: Per-turn lifecycle before conversation UI resumes

For each operator-submitted user message that is not an exit command, the interactive chat session SHALL run the outbound inference exchange to completion, fully release outbound transport for that turn, and only then print the assistant reply or resume waiting for operator input.

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

## MODIFIED Requirements

### Requirement: Interactive chat session loop

After network diagnostics succeed, the image SHALL run an interactive chat session that repeatedly accepts user input, runs one isolated outbound inference exchange per non-exit message, and prints assistant outcomes only after each exchange's outbound transport is fully released, until the operator ends the session.

#### Scenario: Prompt before each user turn

- **GIVEN** network diagnostics and the initial chat prompt have completed
- **WHEN** the session is ready for operator input
- **THEN** console output shows a minimal input prompt consisting of a greater-than character followed by a trailing space
- **AND** the operator can distinguish input waits from assistant replies

#### Scenario: Assistant reply before next input

- **GIVEN** the operator submitted a user message and inference succeeded
- **WHEN** outbound transport for that turn is fully released and the assistant reply is available
- **THEN** the reply is printed on the console as plain conversation text
- **AND** the session returns to waiting for the next user input unless the operator ends the session

### Requirement: Conversation history for follow-up turns

The interactive chat session SHALL retain prior user and assistant messages in memory across turns, include every retained turn in each subsequent inference request payload, and SHALL NOT retain outbound transport state across turns as a substitute for conversation history.

#### Scenario: Second request includes first exchange

- **GIVEN** the operator submitted a first user message and received an assistant reply
- **WHEN** the operator submits a second user message
- **THEN** the inference request includes both prior user and assistant messages plus the new user message
- **AND** the request ordering preserves conversational order
- **AND** the second turn begins outbound transport from a clean inactive state rather than reusing the first turn's transport

#### Scenario: History grows with each turn

- **GIVEN** multiple successful inference exchanges in one session
- **WHEN** a new inference request is sent
- **THEN** the request includes all retained user and assistant messages from the session so far
- **AND** the endpoint receives sufficient context to answer follow-up questions coherently
- **AND** each turn's outbound transport is isolated even though message history accumulates in memory
