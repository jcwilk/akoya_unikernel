# http-chat-probe Specification

## Purpose
TBD - created by archiving change bootstrap-chat-completion-probe. Update Purpose after archive.
## Requirements
### Requirement: Single chat-completion request after network configuration

After the image holds a usable IPv4 configuration on the wired interface, it SHALL perform exactly one outbound HTTP chat-completion request to a build-configured endpoint on the LAN before halting.

#### Scenario: Request sent after DHCP

- **GIVEN** DHCP completed successfully and the assigned address has been reported on the console
- **WHEN** the chat-completion step runs
- **THEN** the image initiates one HTTP request to the configured chat-completions endpoint
- **AND** no second chat-completion request is sent during the same boot

#### Scenario: Skipped when IP configuration failed

- **GIVEN** the image did not obtain a usable IPv4 configuration
- **WHEN** boot proceeds past the address-acquisition step
- **THEN** the image does not attempt a chat-completion request
- **AND** console output does not claim a successful completion response

### Requirement: OpenAI-compatible request payload

The chat-completion request SHALL use a JSON body in the standard OpenAI chat-completions shape: a `messages` array containing one user-role message with the configured diagnostic text.

#### Scenario: Diagnostic user message

- **GIVEN** a usable IPv4 configuration and a reachable chat-completions endpoint
- **WHEN** the HTTP request is sent
- **THEN** the request body includes the configured diagnostic user message as a user-role chat message
- **AND** the endpoint can accept the payload without a custom non-OpenAI adapter

### Requirement: Response reporting on console

The image SHALL print chat-completion outcome on the console in human-readable lines before halting. On success, the printed content SHALL include the model's reply text extracted from the HTTP response body. On failure, the printed content SHALL include a short reason category.

#### Scenario: Successful completion

- **GIVEN** a usable IPv4 configuration and a chat-completions endpoint that returns a valid success response
- **WHEN** the HTTP exchange completes
- **THEN** console output indicates chat-completion success
- **AND** console output includes the assistant reply text from the response body
- **AND** an observer can read the reply without parsing raw HTTP on the wire

#### Scenario: Endpoint unreachable

- **GIVEN** a usable IPv4 configuration
- **AND** the configured endpoint does not accept the connection within the bounded wait period
- **WHEN** the HTTP exchange completes or times out
- **THEN** console output indicates chat-completion failure
- **AND** console output includes a short reason category (for example connection refused or timeout)

#### Scenario: HTTP or parse error

- **GIVEN** a usable IPv4 configuration
- **AND** the endpoint returns a non-success HTTP status or a body that cannot be interpreted as a chat completion
- **WHEN** the HTTP exchange completes
- **THEN** console output indicates chat-completion failure
- **AND** console output includes a short reason category (for example http-error or bad-response)

### Requirement: Orderly shutdown after chat probe

After emitting chat-completion diagnostic output (success or failure), the image SHALL halt or enter an idle loop without further application traffic.

#### Scenario: No follow-on requests

- **GIVEN** chat-completion diagnostic output has been printed
- **WHEN** boot completes
- **THEN** the image does not send further HTTP requests or application-layer traffic
- **AND** behavior remains suitable for repeated smoke testing

