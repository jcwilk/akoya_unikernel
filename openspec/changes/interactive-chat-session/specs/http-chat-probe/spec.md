## MODIFIED Requirements

### Requirement: Multi-turn chat-completion exchanges

After the image holds a usable IPv4 configuration on the wired interface, it SHALL perform one or more outbound HTTP chat-completion requests during an interactive session before halting. Each user-submitted line that is not an exit command SHALL trigger exactly one inference request for that turn.

#### Scenario: First request after DHCP

- **GIVEN** DHCP completed successfully and the assigned address has been reported on the console
- **WHEN** the operator submits the first user message in the interactive session
- **THEN** the image initiates one HTTP request to the configured chat-completions endpoint for that turn
- **AND** further requests occur only when the operator submits additional non-exit messages

#### Scenario: Skipped when IP configuration failed

- **GIVEN** the image did not obtain a usable IPv4 configuration
- **WHEN** boot proceeds past the address-acquisition step
- **THEN** the image does not attempt chat-completion requests
- **AND** console output does not claim a successful completion response

### Requirement: OpenAI-compatible request payload with conversation history

Each chat-completion request SHALL use a JSON body in the standard OpenAI chat-completions shape: a `messages` array listing the full retained conversation—every prior user and assistant turn in order, plus the new user message for the current turn.

#### Scenario: First user message only

- **GIVEN** a usable IPv4 configuration and a reachable chat-completions endpoint
- **WHEN** the operator submits the first user message in a session
- **THEN** the request body includes that user message as a user-role chat message
- **AND** the endpoint can accept the payload without a custom non-OpenAI adapter

#### Scenario: Follow-up includes prior turns

- **GIVEN** a session with at least one completed user and assistant exchange
- **WHEN** the operator submits a follow-up user message
- **THEN** the request body includes all retained prior user and assistant messages in order
- **AND** the new user message is appended as the latest user-role entry

### Requirement: Per-turn response reporting on console

For each inference exchange, the image SHALL print chat-completion outcome on the console in human-readable lines before prompting for the next user input or ending the session. On success, the printed content SHALL include the model's reply text extracted from the HTTP response body. On failure, the printed content SHALL include a short reason category.

#### Scenario: Successful completion for a turn

- **GIVEN** a usable IPv4 configuration and a chat-completions endpoint that returns a valid success response
- **WHEN** an inference exchange completes for a submitted user message
- **THEN** console output indicates chat-completion success for that turn
- **AND** console output includes the assistant reply text from the response body
- **AND** an observer can read the reply without parsing raw HTTP on the wire

#### Scenario: Endpoint unreachable for a turn

- **GIVEN** a usable IPv4 configuration
- **AND** the configured endpoint does not accept the connection within the bounded wait period
- **WHEN** an inference exchange completes or times out
- **THEN** console output indicates chat-completion failure for that turn
- **AND** console output includes a short reason category (for example connection refused or timeout)

#### Scenario: HTTP or parse error for a turn

- **GIVEN** a usable IPv4 configuration
- **AND** the endpoint returns a non-success HTTP status or a body that cannot be interpreted as a chat completion
- **WHEN** an inference exchange completes
- **THEN** console output indicates chat-completion failure for that turn
- **AND** console output includes a short reason category (for example http-error or bad-response)

## REMOVED Requirements

### Requirement: Single chat-completion request after network configuration

**Reason:** Replaced by multi-turn interactive session with one request per submitted user line.

**Migration:** Operators use the interactive session; automated smoke tests drive scripted input for at least one exchange.

### Requirement: OpenAI-compatible request payload

**Reason:** Superseded by requirement that includes full conversation history in each payload.

**Migration:** Same OpenAI shape; `messages` now carries retained turns instead of a single diagnostic message only.

### Requirement: Response reporting on console

**Reason:** Superseded by per-turn response reporting before the next prompt.

**Migration:** Console tokens remain per exchange; timing shifts to within the session loop.

### Requirement: Orderly shutdown after chat probe

**Reason:** Shutdown occurs after the interactive session ends, not after a single probe.

**Migration:** See `interactive-chat-session` exit requirement and `network-unikernel` orderly completion update.
