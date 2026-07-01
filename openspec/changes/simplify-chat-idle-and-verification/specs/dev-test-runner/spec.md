## MODIFIED Requirements

### Requirement: Default automated verification exercises multi-turn chat

The project's default automated verification entry point SHALL run multi-turn interactive chat on the main chat unikernel when the inference endpoint is reachable. The gate SHALL submit at least two non-exit user messages with a host-timed idle period of at least twenty seconds at the input prompt between the first successful turn and the second submission, exercising the same production interactive session path used in headful operation. Pass SHALL require plain assistant reply text after each successful turn and SHALL treat unexpected connection-failure or transport-lifecycle failure outcome lines between successful turns as non-zero exit.

#### Scenario: Default gate fails on idle-at-prompt connect regression

- **GIVEN** a reachable chat-completions endpoint
- **AND** the guest would print a connection-failure or transport-lifecycle failure outcome on a follow-up turn after an idle wait at the input prompt despite a successful first turn
- **WHEN** the default automated verification entry point runs to completion
- **THEN** it exits with non-zero status
- **AND** failure output indicates an unexpected connection-failure or transport-lifecycle outcome during multi-turn chat

#### Scenario: Default gate passes healthy multi-turn chat

- **GIVEN** a reachable chat-completions endpoint and a correctly functioning fail-closed multi-turn transport lifecycle on the main chat unikernel
- **WHEN** the default automated verification entry point runs to completion
- **THEN** captured output contains plain assistant reply text for at least two consecutive turns
- **AND** captured output does not contain connection-failure or transport-lifecycle failure outcome lines between those successful turns

#### Scenario: Default gate exercises idle at the input prompt

- **GIVEN** the default automated verification entry point runs with a reachable chat-completions endpoint
- **WHEN** verification completes successfully
- **THEN** the verification path included an idle period of at least twenty seconds at the input prompt between the first successful turn and the second user submission
- **AND** success is determined on the main interactive chat unikernel rather than a separate regression-only boot identity

### Requirement: Transport verification does not substitute for multi-turn chat gate

Passing transport-only automated verification SHALL NOT satisfy the default multi-turn chat health gate. Assessment of multi-turn chat transport health SHALL require successful multi-turn interactive chat verification on the main chat unikernel in addition to any transport-only verification when both are available. Passing multi-turn chat verification SHALL NOT excuse transport-only verification failures when transport stack health is separately required, and passing transport-only verification SHALL NOT excuse multi-turn chat verification failures.

#### Scenario: Transport pass does not excuse chat multi-turn fail

- **GIVEN** transport-only verification reports aggregate pass
- **AND** multi-turn interactive chat verification on the main chat unikernel fails with a connection-failure or transport-lifecycle outcome between turns
- **WHEN** assessing whether multi-turn chat transport is healthy
- **THEN** multi-turn chat health is not satisfied by transport verification alone
- **AND** the multi-turn chat verification failure remains blocking for chat health sign-off

#### Scenario: Chat verification pass does not excuse transport-only fail when transport health is assessed

- **GIVEN** multi-turn interactive chat verification on the main chat unikernel reports pass
- **AND** transport-only verification reports aggregate fail
- **WHEN** assessing whether raw transport stack scenarios are healthy
- **THEN** transport stack health is not satisfied by chat verification alone
- **AND** the transport-only failure remains blocking for transport stack sign-off when that assessment is required

## REMOVED Requirements

### Requirement: Timed-gap chat regression automated verification

**Reason:** Idle-at-prompt multi-turn health is verified on the main interactive chat unikernel with host-timed gaps; a separate regression boot image and entry point duplicated behavior and diverged from headful reproduction.

**Migration:** Use the default automated verification entry point and the consolidated idle-at-prompt scripted interaction definition on the main chat unikernel.
