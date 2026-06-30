## Why

Prior fixes for multi-turn chat transport still rely on a pragmatic teardown model: bounded best-effort drain, force-release when inactive state is not reached in time, and connect retry on the next turn. That approach can mask incomplete per-turn transport release, let turn boundaries proceed as if teardown succeeded, and allow turn 2+ regressions to slip past transport-only verification. Operators need a conservative, fail-closed per-turn outbound transport lifecycle and a regression harness that exercises the production chat path across timed idle gaps between turns—the same conditions headful operators experience at the input prompt.

## What Changes

- Replace best-effort teardown, silent force-release, and connect-retry masking with a fail-closed turn boundary: outbound transport MUST reach fully inactive before printing an assistant outcome or resuming the input prompt, OR the turn MUST fail loudly with a categorized transport-lifecycle outcome.
- Strengthen interactive session and HTTP chat probe requirements so incomplete prior-turn release cannot be hidden by retry or forced state clearing at turn boundaries.
- Introduce a timed-gap chat regression boot identity (or equivalent documented pattern) that reuses the production chat turn path but auto-advances turns using bounded timed blocking at the input prompt instead of keyboard input, mimicking human pacing and headful idle-at-prompt.
- Wire timed-gap multi-turn chat regression into the default automated verification gate with multiple consecutive turns and inter-turn delays; transport-only verification alone remains insufficient for chat health sign-off.

## Capabilities

### New Capabilities

- `timed-gap-chat-regression`: Distinct boot identity that runs the production interactive chat turn lifecycle for multiple consecutive turns with bounded timed idle gaps at the input prompt, reports per-turn and aggregate outcomes on the console, and is selectable by automated runners without starting the main operator-driven chat unikernel.

### Modified Capabilities

- `interactive-chat-session`: Fail-closed per-turn transport lifecycle—no proceeding to reply print or next input wait when transport is not fully inactive; no masking incomplete release across turn boundaries.
- `http-chat-probe`: Fail-closed outbound transport per inference turn—provably inactive start state, no connect retry to mask incomplete prior-turn release, categorized failure when inactive state cannot be reached before turn boundary.
- `dev-test-runner`: Default automated verification gate requires timed-gap multi-turn chat regression (multiple consecutive turns with inter-turn delay); dedicated entry point for the timed-gap regression boot identity; reinforce that transport-only verification does not satisfy chat health.

## Impact

- Kernel network stack and interactive session loop: teardown semantics, turn-boundary guards, and failure categorization for incomplete transport release.
- Build pipeline: new timed-gap chat regression boot identity alongside main chat unikernel and transport-test image.
- Development test runner: default gate, new verification entry point, and runner selection semantics for timed-gap regression.
- Operator workflow: headful multi-turn chat failures surface as explicit transport-lifecycle outcomes rather than masked connect retries; CI/agents gain a gate that mimics idle-at-prompt pacing.
- Out of scope: persistent keep-alive transport, TLS/DNS, headful automated assertion harness, bare-metal Akoya EX automated verification.
