# http-chat-probe Specification

## Purpose
TBD - created by archiving change bootstrap-chat-completion-probe. Update Purpose after archive.
## Requirements
### Requirement: Multi-turn chat-completion exchanges

After the image holds a usable IPv4 configuration on the wired interface, it SHALL perform one or more outbound HTTP chat-completion requests during an interactive session before halting. Each user-submitted line that is not an exit command SHALL trigger exactly one inference request for that turn, and each such request SHALL use outbound transport that is activated for that turn and fully released before the next turn begins.

#### Scenario: First request after DHCP

- **GIVEN** DHCP completed successfully and the assigned address has been reported on the console
- **WHEN** the operator submits the first user message in the interactive session
- **THEN** the image initiates one HTTP request to the configured chat-completions endpoint for that turn from a clean inactive transport state
- **AND** outbound transport for that turn is fully released after the exchange completes
- **AND** further requests occur only when the operator submits additional non-exit messages, each beginning from a clean inactive transport state

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

For each inference exchange, the image SHALL extract the assistant response or failure outcome before outbound transport for that turn is fully inactive, and SHALL print the assistant reply or failure outcome as plain conversation text on its own line(s) only after outbound transport for that turn is fully inactive, before prompting for the next user input or ending the session. On failure, the image SHALL print a brief, human-readable reason without key=value diagnostic framing. Transport-lifecycle failures SHALL use a distinct readable category separate from endpoint connection refusal or HTTP errors when inactive state cannot be reached.

#### Scenario: Successful completion for a turn

- **GIVEN** a usable IPv4 configuration and a chat-completions endpoint that returns a valid success response
- **WHEN** an inference exchange completes for a submitted user message
- **THEN** the assistant reply text is extracted before outbound transport for that turn is fully inactive
- **AND** console output includes the assistant reply text on its own line(s) only after outbound transport for that turn is fully inactive
- **AND** the reply is not wrapped in key=value status framing
- **AND** an observer can read the reply without parsing raw HTTP on the wire

#### Scenario: Endpoint unreachable for a turn

- **GIVEN** a usable IPv4 configuration
- **AND** the configured endpoint does not accept the connection within the bounded wait period
- **WHEN** an inference exchange completes or times out
- **THEN** outbound transport for that turn is fully inactive before the failure outcome is printed
- **AND** console output indicates chat-completion failure for that turn in brief, readable form
- **AND** console output includes a short reason category (for example connection refused or timeout)
- **AND** the failure line does not use key=value diagnostic framing

#### Scenario: Transport-lifecycle failure for a turn

- **GIVEN** a usable IPv4 configuration
- **AND** outbound transport for the turn cannot reach fully inactive state before the turn boundary
- **WHEN** the inference turn ends
- **THEN** console output indicates a transport-lifecycle failure for that turn in brief, readable form
- **AND** console output does not print plain assistant reply text as if the turn succeeded
- **AND** the failure line does not use key=value diagnostic framing

#### Scenario: HTTP or parse error for a turn

- **GIVEN** a usable IPv4 configuration
- **AND** the endpoint returns a non-success HTTP status or a body that cannot be interpreted as a chat completion
- **WHEN** an inference exchange completes
- **THEN** outbound transport for that turn is fully inactive before the failure outcome is printed
- **AND** console output indicates chat-completion failure for that turn in brief, readable form
- **AND** console output includes a short reason category (for example http-error or bad-response)
- **AND** the failure line does not use key=value diagnostic framing

### Requirement: Isolated outbound transport per inference turn

Each operator-submitted user message that is not an exit command SHALL trigger exactly one outbound chat-completion exchange that begins from a fully inactive transport state, runs through response extraction to completion, and reaches fully inactive outbound transport before the turn ends. After release, the network stack SHALL remain capable of opening a fresh outbound connection for the next turn. The image SHALL NOT mask incomplete release by forcibly clearing transport state and retrying connection activation on the same or next turn.

#### Scenario: Fresh transport for the first message

- **GIVEN** a usable IPv4 configuration and the operator submits the first user message in a session
- **WHEN** the inference turn begins
- **THEN** outbound transport is fully inactive with no leftover state from a prior turn
- **AND** exactly one chat-completion exchange runs for that message
- **AND** outbound transport is fully inactive before the turn ends

#### Scenario: Fresh transport for each follow-up message

- **GIVEN** a session with at least one completed inference exchange whose transport reached fully inactive state before the turn ended
- **WHEN** the operator submits another non-exit user message
- **THEN** outbound transport for the new turn is fully inactive before connection activation
- **AND** the new turn does not reuse an active outbound connection or incompletely released state from a prior turn
- **AND** the new turn runs exactly one chat-completion exchange through response extraction
- **AND** outbound transport is fully inactive before the turn ends

#### Scenario: No lingering transport at turn boundary

- **GIVEN** an inference turn completed or failed for a submitted user message
- **WHEN** control returns to the interactive session for that turn
- **THEN** outbound transport for that turn is fully inactive
- **AND** no turn-specific receive path or transport state persists that could affect the next turn

#### Scenario: Network stack ready after turn completes

- **GIVEN** an inference turn completed or failed for a submitted user message
- **WHEN** outbound transport for that turn is fully inactive at turn end
- **THEN** the network stack remains able to initiate a new outbound connection on a subsequent turn
- **AND** no turn-specific transport state prevents a fresh connection attempt for the next operator message

#### Scenario: Single connection activation attempt per turn

- **GIVEN** outbound transport is fully inactive at turn start
- **WHEN** the inference turn activates outbound transport for a submitted user message
- **THEN** connection activation is attempted without a silent second attempt triggered solely to recover from incomplete prior-turn release
- **AND** connection failure reflects actual endpoint or wire behavior during that turn's bounded wait

### Requirement: Bounded completion output per request

Each chat-completion request SHALL instruct the configured inference endpoint to emit at most five hundred tokens of generated completion output for that turn. The same cap SHALL apply on every turn in a session, including the first user message and all follow-up messages.

#### Scenario: First turn includes output cap

- **GIVEN** a usable IPv4 configuration and a reachable chat-completions endpoint
- **WHEN** the operator submits the first user message in a session
- **THEN** the outbound request conveys a completion output limit of five hundred tokens for that turn
- **AND** the endpoint receives the limit as part of the standard chat-completions request shape

#### Scenario: Follow-up turn includes same output cap

- **GIVEN** a session with at least one completed user and assistant exchange
- **WHEN** the operator submits a follow-up user message
- **THEN** the outbound request for that turn conveys the same five-hundred-token completion output limit
- **AND** the limit is not omitted on later turns

#### Scenario: Cap prevents unbounded generation requests

- **GIVEN** a user message that could elicit a lengthy assistant reply from the inference service
- **WHEN** the image sends the chat-completion request
- **THEN** the request does not leave completion output size unconstrained
- **AND** the requested upper bound remains five hundred tokens for that turn

### Requirement: Follow-up turns succeed when inference endpoint is reachable

When the configured chat-completions endpoint is reachable from the guest and each prior turn in the same session reached fully inactive outbound transport before its turn boundary, each subsequent non-exit user message SHALL complete an inference exchange without a spurious connection-failure outcome attributable to incomplete release of prior-turn outbound transport or connect retry masking.

#### Scenario: Second turn succeeds after first success

- **GIVEN** a usable IPv4 configuration and a reachable chat-completions endpoint
- **AND** the operator completed a first inference turn with a successful assistant reply and fully inactive transport at turn end
- **WHEN** the operator submits a second non-exit user message
- **THEN** the second turn completes with plain assistant reply text on the console
- **AND** console output does not report a connection-failure outcome solely because outbound transport from the prior turn was not fully inactive or was masked by connect retry

#### Scenario: Later turns remain connectable

- **GIVEN** a session with at least two completed successful inference exchanges each ending with fully inactive transport
- **AND** the chat-completions endpoint remains reachable
- **WHEN** the operator submits another non-exit user message
- **THEN** outbound transport for the new turn is fully inactive before connection activation and the exchange completes
- **AND** no connection-failure outcome occurs unless the endpoint is actually unreachable or refuses the connection during that turn's bounded wait

### Requirement: Provably inactive transport at turn start

Each chat-completion inference turn SHALL begin only when outbound transport is fully inactive with no leftover active connection, receive path, or turn-specific transport state from a prior turn. If inactive state cannot be verified at turn start, the turn SHALL fail with a categorized transport-lifecycle outcome rather than proceeding with connection activation.

#### Scenario: First turn starts from inactive stack

- **GIVEN** a usable IPv4 configuration and no prior inference turn in the session
- **WHEN** the first inference turn begins for a submitted user message
- **THEN** outbound transport is fully inactive before connection activation
- **AND** exactly one chat-completion exchange runs for that message

#### Scenario: Follow-up turn rejects stale transport

- **GIVEN** a session with at least one prior inference turn
- **AND** outbound transport from the prior turn is not fully inactive
- **WHEN** a new inference turn would begin for a submitted user message
- **THEN** the turn fails with a categorized transport-lifecycle outcome
- **AND** the turn does not proceed by forcibly clearing stale transport and retrying connection activation

### Requirement: Fail-closed transport release before turn ends

After each chat-completion exchange, outbound transport for that turn SHALL reach fully inactive state before the turn ends. If inactive state cannot be reached within the bounded release wait for that turn, the image SHALL report a categorized transport-lifecycle failure for that turn and SHALL NOT treat the turn as successfully complete for purposes of printing an assistant reply or ending the turn as if transport were fully released.

#### Scenario: Successful turn ends with inactive transport

- **GIVEN** a chat-completion exchange completed successfully for a submitted user message
- **WHEN** the inference turn ends
- **THEN** outbound transport for that turn is fully inactive
- **AND** assistant reply text may be printed only after inactive state is reached

#### Scenario: Release timeout fails the turn

- **GIVEN** a chat-completion exchange completed or failed for a submitted user message
- **WHEN** outbound transport cannot reach fully inactive state before the turn boundary
- **THEN** the turn reports a categorized transport-lifecycle failure on the console
- **AND** the turn does not proceed to print an assistant reply or end as if transport were fully released by forcibly clearing state

