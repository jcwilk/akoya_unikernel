## ADDED Requirements

### Requirement: US keyboard console input

After network diagnostics complete, the image SHALL accept operator input from the console keyboard using US keyboard layout semantics for the interactive chat session.

#### Scenario: Printable characters appear on the current line

- **GIVEN** the interactive chat session is waiting for user input
- **WHEN** the operator presses keys that produce US-layout printable ASCII characters
- **THEN** those characters are echoed to the console as part of the current input line
- **AND** the characters are retained as the pending user message until submission or editing

#### Scenario: Backspace edits the current line

- **GIVEN** the operator has typed one or more characters on the current input line
- **WHEN** the operator presses Backspace
- **THEN** the last character is removed from the pending line
- **AND** console echo reflects the shortened line state

#### Scenario: Enter submits the current line

- **GIVEN** the operator has composed a non-empty input line
- **WHEN** the operator presses Enter
- **THEN** the pending line is submitted to the chat session as a user message
- **AND** a new empty input line begins for the next turn

### Requirement: Interactive chat session loop

After network diagnostics succeed, the image SHALL run an interactive chat session that repeatedly accepts user input, requests inference, and prints assistant replies until the operator ends the session.

#### Scenario: Prompt before each user turn

- **GIVEN** network diagnostics and the initial chat prompt have completed
- **WHEN** the session is ready for operator input
- **THEN** console output indicates that the image is waiting for user input
- **AND** the operator can distinguish input waits from assistant replies

#### Scenario: Assistant reply before next input

- **GIVEN** the operator submitted a user message and inference succeeded
- **WHEN** the assistant reply is available
- **THEN** the reply is printed on the console
- **AND** the session returns to waiting for the next user input unless the operator ends the session

### Requirement: Explicit session exit

The interactive chat session SHALL end only when the operator issues an explicit exit action; the image SHALL NOT halt immediately after the first successful inference exchange.

#### Scenario: Exit command ends session

- **GIVEN** the session is waiting for user input
- **WHEN** the operator submits a recognized exit command
- **THEN** no further inference requests are sent for that session
- **AND** the image proceeds to orderly shutdown after network diagnostics and chat work complete

#### Scenario: Session continues after first reply

- **GIVEN** the operator received a successful assistant reply
- **WHEN** the operator has not issued an exit command
- **THEN** the session remains active and accepts another user message
- **AND** the image does not halt solely because one inference exchange completed

### Requirement: Conversation history for follow-up turns

The interactive chat session SHALL retain prior user and assistant messages in memory and include every retained turn in each subsequent inference request payload.

#### Scenario: Second request includes first exchange

- **GIVEN** the operator submitted a first user message and received an assistant reply
- **WHEN** the operator submits a second user message
- **THEN** the inference request includes both prior user and assistant messages plus the new user message
- **AND** the request ordering preserves conversational order

#### Scenario: History grows with each turn

- **GIVEN** multiple successful inference exchanges in one session
- **WHEN** a new inference request is sent
- **THEN** the request includes all retained user and assistant messages from the session so far
- **AND** the endpoint receives sufficient context to answer follow-up questions coherently
