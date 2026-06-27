## Why

Experimental work on the working branch assumed a persistent outbound transport reused across chat turns. The operator intent is the opposite: each submitted user message must run through a complete, isolated transport lifecycle—activate, exchange, fully release—before the console resumes conversation UI. Without explicit behavioral contracts, apply workers may ship keep-alive sessions or print assistant replies while transport is still active, violating the intended turn ordering.

## What Changes

- Define a strict per-turn lifecycle for interactive chat: operator submits a message, outbound transport runs one chat-completion exchange to completion, transport fully deactivates, then the assistant reply prints, then the session waits for the next operator input.
- Require fresh outbound transport for every non-exit user message—no reuse of connections or turn-specific transport state left active from a prior turn.
- Preserve in-memory conversation history across turns; only transport is per-turn isolated. Follow-up requests still include the full retained context in each inference payload.
- Clarify that assistant reply printing and the next input prompt occur only after transport for that turn is fully released.
- Document rejected architectural assumptions (persistent keep-alive, shared transport across turns, streaming reply while transport active) in design migration notes—not in spec requirements.
- Keep headless smoke tests able to validate multi-turn chat without manual input; assertions continue to follow existing plain-reply and reachability contracts.

## Capabilities

### New Capabilities

_(none — behavior is expressed as modifications to existing chat and session capabilities)_

### Modified Capabilities

- `interactive-chat-session`: Per-turn ordering—transport completes and fully deactivates before assistant reply is printed and before the next input wait; session loop semantics aligned with isolated transport per message.
- `http-chat-probe`: Each non-exit user message triggers one self-contained outbound exchange from a clean inactive transport state through response extraction and full transport release before the turn ends.
- `dev-test-runner`: Headless scripted multi-turn chat remains valid; smoke assertions unchanged in contract (plain assistant reply text, reachability, minimal input prompt).

## Impact

- HTTP chat and TCP transport layers must align with per-turn activate–exchange–teardown instead of persistent session reuse.
- Interactive session loop must not print assistant replies or accept the next operator line until outbound transport for the current turn is fully released.
- Uncommitted experimental persistent-transport work on the working branch is superseded by this intent; apply should replace that direction, not extend it.
- Headless runner scripted keyboard input and timeout budgets should remain sufficient for at least one multi-turn exchange without changing smoke-test pass criteria.
