## ADDED Requirements

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
