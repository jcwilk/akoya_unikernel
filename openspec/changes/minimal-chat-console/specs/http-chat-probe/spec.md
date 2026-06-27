## MODIFIED Requirements

### Requirement: Per-turn response reporting on console

For each inference exchange, the image SHALL print the assistant's reply as plain conversation text on its own line(s) before prompting for the next user input or ending the session. On failure, the image SHALL print a brief, human-readable reason without key=value diagnostic framing.

#### Scenario: Successful completion for a turn

- **GIVEN** a usable IPv4 configuration and a chat-completions endpoint that returns a valid success response
- **WHEN** an inference exchange completes for a submitted user message
- **THEN** console output includes the assistant reply text on its own line(s)
- **AND** the reply is not wrapped in key=value status framing
- **AND** an observer can read the reply without parsing raw HTTP on the wire

#### Scenario: Endpoint unreachable for a turn

- **GIVEN** a usable IPv4 configuration
- **AND** the configured endpoint does not accept the connection within the bounded wait period
- **WHEN** an inference exchange completes or times out
- **THEN** console output indicates chat-completion failure for that turn in brief, readable form
- **AND** console output includes a short reason category (for example connection refused or timeout)
- **AND** the failure line does not use key=value diagnostic framing

#### Scenario: HTTP or parse error for a turn

- **GIVEN** a usable IPv4 configuration
- **AND** the endpoint returns a non-success HTTP status or a body that cannot be interpreted as a chat completion
- **WHEN** an inference exchange completes
- **THEN** console output indicates chat-completion failure for that turn in brief, readable form
- **AND** console output includes a short reason category (for example http-error or bad-response)
- **AND** the failure line does not use key=value diagnostic framing
