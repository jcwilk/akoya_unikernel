## ADDED Requirements

### Requirement: Follow-up turns succeed when inference endpoint is reachable

When the configured chat-completions endpoint is reachable from the guest and prior turns in the same session completed successfully, each subsequent non-exit user message SHALL complete an inference exchange without a spurious connection-failure outcome attributable to incomplete release of prior-turn outbound transport.

#### Scenario: Second turn succeeds after first success

- **GIVEN** a usable IPv4 configuration and a reachable chat-completions endpoint
- **AND** the operator completed a first inference turn with a successful assistant reply
- **WHEN** the operator submits a second non-exit user message
- **THEN** the second turn completes with plain assistant reply text on the console
- **AND** console output does not report a connection-failure outcome solely because outbound transport from the prior turn was not fully released

#### Scenario: Later turns remain connectable

- **GIVEN** a session with at least two completed successful inference exchanges
- **AND** the chat-completions endpoint remains reachable
- **WHEN** the operator submits another non-exit user message
- **THEN** outbound transport for the new turn begins from a clean inactive state and the exchange completes
- **AND** no connection-failure outcome occurs unless the endpoint is actually unreachable or refuses the connection during that turn's bounded wait

## MODIFIED Requirements

### Requirement: Isolated outbound transport per inference turn

Each operator-submitted user message that is not an exit command SHALL trigger exactly one outbound chat-completion exchange that begins from a clean inactive transport state, runs through response extraction to completion, and fully releases outbound transport before the turn ends. After release, the network stack SHALL remain capable of opening a fresh outbound connection for the next turn.

#### Scenario: Fresh transport for the first message

- **GIVEN** a usable IPv4 configuration and the operator submits the first user message in a session
- **WHEN** the inference turn begins
- **THEN** outbound transport starts from a clean inactive state with no leftover active connection from a prior turn
- **AND** exactly one chat-completion exchange runs for that message
- **AND** outbound transport is fully released before the turn ends

#### Scenario: Fresh transport for each follow-up message

- **GIVEN** a session with at least one completed inference exchange
- **WHEN** the operator submits another non-exit user message
- **THEN** outbound transport for the new turn begins from a clean inactive state
- **AND** the new turn does not reuse an active outbound connection left open from a prior turn
- **AND** the new turn runs exactly one chat-completion exchange through response extraction
- **AND** outbound transport is fully released before the turn ends

#### Scenario: No lingering transport at turn boundary

- **GIVEN** an inference turn completed or failed for a submitted user message
- **WHEN** control returns to the interactive session for that turn
- **THEN** no active outbound connection remains for that turn
- **AND** no turn-specific receive path or transport state persists that could affect the next turn

#### Scenario: Network stack ready after turn completes

- **GIVEN** an inference turn completed or failed for a submitted user message
- **WHEN** outbound transport for that turn is fully released
- **THEN** the network stack remains able to initiate a new outbound connection on a subsequent turn
- **AND** no turn-specific transport state prevents a fresh connection attempt for the next operator message
