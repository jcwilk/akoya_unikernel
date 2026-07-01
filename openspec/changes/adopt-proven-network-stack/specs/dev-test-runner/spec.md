## MODIFIED Requirements

### Requirement: Default automated verification exercises multi-turn chat

The project's default automated verification entry point SHALL run multi-turn interactive chat on the main chat unikernel when the inference endpoint is reachable. The gate SHALL submit at least two non-exit user messages with a host-timed idle period of at least twenty seconds at the input prompt between the first successful turn and the second submission. Pass SHALL require plain assistant reply text after each successful turn and SHALL treat unexpected connection-failure or transport-lifecycle failure outcome lines between successful turns as non-zero exit.

#### Scenario: Default gate fails on idle-at-prompt connect regression

- **GIVEN** a reachable chat-completions endpoint
- **AND** the guest would print a connection-failure or transport-lifecycle failure outcome on a follow-up turn after an idle wait at the input prompt despite a successful first turn
- **WHEN** the default automated verification entry point runs to completion
- **THEN** it exits with non-zero status
- **AND** failure output indicates an unexpected connection-failure or transport-lifecycle outcome during multi-turn chat

#### Scenario: Default gate passes healthy multi-turn chat

- **GIVEN** a reachable chat-completions endpoint and a correctly functioning multi-turn transport lifecycle on the main chat unikernel
- **WHEN** the default automated verification entry point runs to completion
- **THEN** captured output contains plain assistant reply text for at least two consecutive turns
- **AND** captured output does not contain connection-failure or transport-lifecycle failure outcome lines between those successful turns

#### Scenario: Default gate exercises twenty-second idle at the input prompt

- **GIVEN** the default automated verification entry point runs with a reachable chat-completions endpoint
- **WHEN** verification completes successfully
- **THEN** the verification path included an idle period of at least twenty seconds at the input prompt between the first successful turn and the second user submission
- **AND** success is determined on the main interactive chat unikernel

## REMOVED Requirements

### Requirement: Timed-gap chat regression automated verification

**Reason:** Idle-at-prompt multi-turn health is verified on the main chat unikernel with host-timed gaps; separate regression boot image duplicated behavior.

**Migration:** Use the default automated verification entry point on the main chat unikernel.
