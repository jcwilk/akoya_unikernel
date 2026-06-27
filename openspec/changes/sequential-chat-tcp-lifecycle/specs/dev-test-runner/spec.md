## ADDED Requirements

### Requirement: Headless multi-turn smoke coverage with per-turn transport

Headless smoke tests SHALL remain able to validate multi-turn chat without manual operator input. Pass and fail assertions SHALL continue to use the existing plain-reply, reachability, and minimal-input-prompt contracts and SHALL NOT require persistent outbound transport across scripted turns.

#### Scenario: Default scripted session still completes an exchange

- **GIVEN** headless mode with default scripted keyboard input and a reachable chat-completions endpoint
- **WHEN** the run entry point completes successfully
- **THEN** captured output contains non-empty plain assistant reply text from at least one inference exchange on its own line(s)
- **AND** captured output includes evidence that the minimal chat input prompt appeared during the scripted session
- **AND** success is determined without requiring transport to remain active between scripted submissions

#### Scenario: Multi-turn script uses the same assertion contract

- **GIVEN** headless mode with a configured multi-line keyboard input script that submits more than one non-exit user message
- **WHEN** the run entry point completes successfully
- **THEN** captured output may contain assistant reply text from multiple turns
- **AND** pass/fail still relies on plain reply text and reachability output rather than transport persistence or key=value chat status framing
