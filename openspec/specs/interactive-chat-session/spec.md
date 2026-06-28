# interactive-chat-session Specification

## Purpose
TBD - created by archiving change interactive-chat-session. Update Purpose after archive.
## Requirements
### Requirement: Integrated keyboard input on deployment target

On bare metal matching the Medion Akoya EX deployment profile, the interactive chat session SHALL accept operator input from the notebook integrated keyboard without requiring an external USB keyboard or serial console as the typing device.

#### Scenario: Built-in keyboard on target hardware

- **GIVEN** the image boots on Akoya-class bare metal with integrated keyboard
- **WHEN** the operator enters the interactive chat session
- **THEN** key presses on the built-in keyboard produce input for the pending chat line
- **AND** typing does not depend on attaching an external serial terminal for input

### Requirement: US keyboard layout from key events

The interactive chat session SHALL interpret key events from the deployment-target keyboard controller path using US keyboard layout semantics.

#### Scenario: Printable characters appear on the current line

- **GIVEN** the interactive chat session is waiting for user input
- **WHEN** the operator presses keys that produce US-layout printable ASCII characters on the integrated keyboard
- **THEN** those characters are echoed to the console as part of the current input line
- **AND** the characters are retained as the pending user message until submission or editing

#### Scenario: Backspace edits the current line

- **GIVEN** the operator has typed one or more characters on the current input line
- **WHEN** the operator presses Backspace on the integrated keyboard
- **THEN** the last character is removed from the pending line
- **AND** console echo reflects the shortened line state

#### Scenario: Enter submits the current line

- **GIVEN** the operator has composed a non-empty input line
- **WHEN** the operator presses Enter on the integrated keyboard
- **THEN** the pending line is submitted to the chat session as a user message
- **AND** a new empty input line begins for the next turn

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

### Requirement: Explicit session exit

The interactive chat session SHALL end only when the operator issues an explicit exit action; the image SHALL NOT halt immediately after the first successful inference exchange.

#### Scenario: Exit command ends session

- **GIVEN** the session is waiting for user input
- **WHEN** the operator submits a recognized exit command via the integrated keyboard
- **THEN** no further inference requests are sent for that session
- **AND** the image proceeds to orderly shutdown after network diagnostics and chat work complete

#### Scenario: Session continues after first reply

- **GIVEN** the operator received a successful assistant reply
- **WHEN** the operator has not issued an exit command
- **THEN** the session remains active and accepts another user message
- **AND** the image does not halt solely because one inference exchange completed

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

