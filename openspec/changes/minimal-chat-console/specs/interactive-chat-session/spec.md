## MODIFIED Requirements

### Requirement: Interactive chat session loop

After network diagnostics succeed, the image SHALL run an interactive chat session that repeatedly accepts user input, requests inference, and prints assistant replies until the operator ends the session.

#### Scenario: Prompt before each user turn

- **GIVEN** network diagnostics and the initial chat prompt have completed
- **WHEN** the session is ready for operator input
- **THEN** console output shows a minimal input prompt consisting of a greater-than character followed by a trailing space
- **AND** the operator can distinguish input waits from assistant replies

#### Scenario: Assistant reply before next input

- **GIVEN** the operator submitted a user message and inference succeeded
- **WHEN** the assistant reply is available
- **THEN** the reply is printed on the console as plain conversation text
- **AND** the session returns to waiting for the next user input unless the operator ends the session
