## ADDED Requirements

### Requirement: Timed-gap chat regression automated verification

The development test runner SHALL provide an agent-runnable entry point that builds or selects the timed-gap chat regression boot image, runs it headlessly on the workstation LAN emulation path, captures console output, and exits non-zero when the timed-gap aggregate result reports failure or when expected per-turn outcomes are absent.

#### Scenario: One-command timed-gap regression run

- **GIVEN** project sources and workstation emulation prerequisites are satisfied
- **WHEN** an agent invokes the documented timed-gap chat regression verification entry point without manual intermediate steps
- **THEN** the timed-gap chat regression boot image runs headlessly on the workstation LAN
- **AND** the entry point exits zero when captured output shows aggregate pass for at least three consecutive scheduled turns

#### Scenario: Timed-gap failure is actionable

- **GIVEN** a timed-gap chat regression run where at least one scheduled turn fails or aggregate result is fail
- **WHEN** the verification entry point completes
- **THEN** it exits with non-zero status
- **AND** captured or summarized output identifies the failure without requiring interactive inspection

#### Scenario: Distinct image selection for timed-gap regression

- **GIVEN** the main chat unikernel, network transport test runner, and timed-gap chat regression boot image are present in the build output area
- **WHEN** the timed-gap chat regression verification entry point runs
- **THEN** the timed-gap chat regression logical identity is selected automatically for that entry point
- **AND** the main chat unikernel and transport-test image are not started unless a separate caller explicitly selects them

## MODIFIED Requirements

### Requirement: Default automated verification exercises multi-turn chat

The project's default automated verification entry point SHALL run timed-gap multi-turn chat regression that completes at least three consecutive chat turns with bounded timed idle gaps between turns when the inference endpoint is reachable. Pass SHALL require plain assistant reply text after each successful turn and SHALL treat unexpected connection-failure or transport-lifecycle failure outcome lines between successful turns as non-zero exit. The default gate SHALL use the timed-gap chat regression boot identity or an equivalent path that exercises the same production chat turn lifecycle with guest-side timed idle gaps, not host-side keyboard injection alone.

#### Scenario: Default gate fails on turn-two connect regression

- **GIVEN** a reachable chat-completions endpoint
- **AND** the guest would print a connection-failure or transport-lifecycle failure outcome on a follow-up turn despite a successful first turn
- **WHEN** the default automated verification entry point runs to completion
- **THEN** it exits with non-zero status
- **AND** failure output indicates an unexpected connection-failure or transport-lifecycle outcome during multi-turn timed-gap regression

#### Scenario: Default gate passes healthy multi-turn chat

- **GIVEN** a reachable chat-completions endpoint and a correctly functioning fail-closed multi-turn transport lifecycle
- **WHEN** the default automated verification entry point runs to completion
- **THEN** captured output contains plain assistant reply text for at least three consecutive turns
- **AND** captured output does not contain connection-failure or transport-lifecycle failure outcome lines between those successful turns

#### Scenario: Default gate exercises timed idle gaps

- **GIVEN** the default automated verification entry point runs with a reachable chat-completions endpoint
- **WHEN** regression completes successfully
- **THEN** the verification path included bounded timed idle gaps at the input prompt between consecutive chat turns
- **AND** success is not determined solely by immediate host-side keyboard injection without guest-side idle gaps

### Requirement: Transport verification does not substitute for multi-turn chat gate

Passing transport-only automated verification SHALL NOT satisfy the default multi-turn chat health gate. Assessment of multi-turn chat transport health SHALL require successful timed-gap multi-turn chat regression in addition to any transport-only verification when both are available. Passing timed-gap chat regression SHALL NOT excuse transport-only verification failures when transport stack health is separately required, and passing transport-only verification SHALL NOT excuse timed-gap chat regression failures.

#### Scenario: Transport pass does not excuse chat multi-turn fail

- **GIVEN** transport-only verification reports aggregate pass
- **AND** timed-gap multi-turn chat regression fails with a connection-failure or transport-lifecycle outcome between turns
- **WHEN** assessing whether multi-turn chat transport is healthy
- **THEN** multi-turn chat health is not satisfied by transport verification alone
- **AND** the timed-gap chat regression failure remains blocking for chat health sign-off

#### Scenario: Chat regression pass does not excuse transport-only fail when transport health is assessed

- **GIVEN** timed-gap multi-turn chat regression reports aggregate pass
- **AND** transport-only verification reports aggregate fail
- **WHEN** assessing whether raw transport stack scenarios are healthy
- **THEN** transport stack health is not satisfied by chat regression alone
- **AND** the transport-only failure remains blocking for transport stack sign-off when that assessment is required

### Requirement: Inference endpoint pre-flight before automated verification

Before starting headless automated verification that depends on the configured chat or inference host—including default smoke, timed-gap chat regression, scripted full-app interaction, and transport-test entry points—the development test runner SHALL verify that the configured inference endpoint is reachable from the development workstation. If the endpoint is unreachable, the runner SHALL exit immediately with non-zero status and actionable error output **without** starting emulation, **without** running partial scenarios, and **without** treating unreachability as an optional skip. A reachable inference endpoint is a required precondition for verification; unreachability indicates environment or configuration that must be corrected before verification proceeds.

#### Scenario: Abort before emulation when inference endpoint is down

- **GIVEN** the configured chat or inference endpoint is not reachable from the development workstation within the bounded pre-flight check
- **WHEN** an agent invokes any automated verification entry point that depends on that endpoint
- **THEN** the runner exits immediately with non-zero status
- **AND** emulation does not start
- **AND** no transport or chat scenario is reported as passed due to skip or omission

#### Scenario: Actionable error when inference infrastructure is unavailable

- **GIVEN** inference endpoint pre-flight fails
- **WHEN** the runner exits
- **THEN** error output states that the configured inference endpoint is unreachable
- **AND** error output indicates that inference availability must be restored before verification can proceed
- **AND** an operator or agent can identify the failure as an environment problem rather than a product regression

#### Scenario: Pre-flight passes when inference is healthy

- **GIVEN** the configured inference endpoint accepts connections from the development workstation
- **WHEN** an agent invokes a dependent automated verification entry point
- **THEN** pre-flight succeeds and emulation proceeds
- **AND** verification continues under the harness or smoke contract for that entry point
