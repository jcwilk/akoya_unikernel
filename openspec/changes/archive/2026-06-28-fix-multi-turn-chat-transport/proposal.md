## Why

Headful interactive chat fails on turn 2 and later with spurious connection-failure outcomes (`chat failed: connect`) even when the inference endpoint is reachable and turn 1 succeeds. Prior per-turn transport lifecycle work and transport-test harnesses exposed the gap but did not fix it: raw TCP transport tests pass while the HTTP chat path still leaks or blocks transport state across turns, and the default automated test gate (`make test`) can pass using a weaker single-turn script while a multi-turn regression fixture fails.

## What Changes

- Fix kernel chat and transport lifecycle so each completed inference turn fully releases outbound transport and the network stack can open a fresh outbound connection on every follow-up turn when the inference endpoint is reachable.
- Strengthen observable multi-turn success criteria: follow-up turns SHALL complete without spurious connection-failure outcomes when the endpoint is healthy.
- Promote multi-turn scripted regression to the required default automated test gate so `make test` fails when turn 2+ connection failures occur; transport-test alone remains insufficient proof of multi-turn chat health.
- **Explicitly deferred:** headful automated assertion harness (headful stays operator-observed); bare-metal Akoya EX automated verification.

## Capabilities

### New Capabilities

_(none — behavior and verification changes extend existing domains)_

### Modified Capabilities

- `http-chat-probe`: Require that follow-up inference turns succeed without spurious connection failures when the chat-completions endpoint is reachable; require that the network stack remains capable of opening fresh outbound connections after each completed exchange.
- `interactive-chat-session`: Require that the session loop on follow-up turns does not block waiting for operator input while transport or network polling for the prior turn remains incomplete; align turn-boundary behavior with fully released per-turn transport before the next prompt.
- `dev-test-runner`: Require that the default automated test gate exercises multi-turn chat regression (not only single-turn default keyboard script) and treats unexpected connection-failure outcomes between successful turns as failure; clarify that transport-test verification alone does not satisfy multi-turn chat health.

## Impact

- Kernel network stack: TCP teardown, HTTP chat client, and interactive session loop ordering between turns.
- Build and test runner: default `make test` / headless smoke entry point must run multi-turn regression contract.
- Operator workflow: headful chat should succeed on follow-up turns when inference is healthy; CI/agents gain a gate that catches the reported regression class.
- Out of scope this change: headful automated assertions, bare-metal harness, transport-test image behavior (may note relationship in design only).
