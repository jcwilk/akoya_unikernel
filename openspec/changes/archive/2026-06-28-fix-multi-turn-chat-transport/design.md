# Design: Fix Multi-Turn Chat Transport

## Context

The bootstrap unikernel runs an interactive chat session over the production network stack. Living specs already require per-turn outbound transport isolation, full release before the next prompt, and multi-turn conversation history. Despite archived work on sequential chat TCP lifecycle and automated network test harnesses, operators still see `chat failed: connect` on turn 2+ in headful mode when the inference endpoint is reachable.

Investigation points to an incomplete teardown class:

- `http_chat_run_turn` refuses to start (and sometimes finish) a turn unless `tcp_transport_inactive()` reports a fully idle stack. Turn 2+ hits this guard when prior-turn TCP state, receive buffers, or connection flags linger after `tcp_session_close`.
- The interactive session loop blocks in `ps2_read_line` at the `> ` prompt with **no network polling**. If teardown requires additional RX processing or timer ticks to reach inactive state, those ticks do not run until the next turn begins — too late for the pre-turn guard.
- Transport-test image exercises raw TCP connect/close sequences without the HTTP chat path or the `tcp_transport_inactive()` guard, so it can pass while multi-turn chat fails.
- Default headless smoke can pass with a single-turn keyboard script while the multi-turn regression fixture (`scripts/fixtures/multi-turn-pong.akoya-script`) fails on turn 2.

Prior commits (`sequential-chat-tcp-lifecycle`, teardown hardening at e2125f4) improved ordering but did not fully eliminate the regression class.

## Goals / Non-Goals

**Goals:**

- Every non-exit chat turn completes (or fails with a categorized outcome reflecting real endpoint behavior) when inference is reachable; no spurious connect failure from stale transport state.
- Per-turn teardown reaches a state where `tcp_transport_inactive()` (or its behavioral equivalent) is true before the assistant outcome is printed and before the next input prompt.
- Ensure the network stack can open a fresh outbound connection on turn N+1 after turn N completes.
- Wire multi-turn scripted regression into the default automated verification gate (`make test` / primary headless smoke path) so CI and agents catch turn-2+ failures.
- Keep transport-test as a complementary check; document that it does not substitute for multi-turn chat regression.

**Non-Goals:**

- Headful automated assertion harness (headful remains operator-observed per operator deferral).
- Bare-metal Akoya EX automated verification.
- Replacing per-turn transport with persistent keep-alive sessions.
- Mock or forked TCP stack.
- Changing plain-reply console formatting, exit command semantics, or token cap behavior.

## Decisions

### Decision 1: Fix teardown at the TCP/session boundary, not the guard semantics

**Choice:** Preserve the pre-turn inactive guard in `http_chat_run_turn` as a safety check. Fix root cause by ensuring `tcp_session_close` and post-close polling drain all connection state, receive buffers, handler registration, and FIN/RST flags so inactive becomes true before returning to the session loop.

**Rationale:** The guard correctly detects incomplete teardown; removing or weakening it would mask bugs. Transport-test passing proves connect/close works in isolation; chat path must reach the same inactive predicate.

**Rejected — remove pre-turn inactive check:** Would allow turns to start with ghost state and defer failures to ambiguous wire behavior.

**Rejected — persistent session transport:** Operator and living specs forbid reuse across turns.

### Decision 2: Explicit post-close drain before session UI resumes

**Choice:** After `tcp_session_close` in the HTTP chat turn path, run a bounded drain/poll loop (same class of work transport-test uses waiting for inactive) until inactive or a short teardown timeout. Only then return status to `http_chat_session` for reply printing and next prompt.

**Rationale:** Closes the gap where close is initiated but stack state clears only after additional link RX processing — which does not happen during `ps2_read_line`.

**Alternative considered — poll network during input wait:** Larger session-loop change; deferred unless post-close drain alone is insufficient. May revisit if headful still fails after Decision 2.

### Decision 3: Audit global TCP singleton state in `tcp.c`

**Choice:** Review and fix any fields not cleared on close: `handler_registered`, `conn_*` flags, `recv_buf`/`recv_len`, local/remote ports, and half-open FIN sequences. Align `tcp_transport_inactive()` predicate with actual reusable-idle state.

**Rationale:** Guard reads global singleton state; partial clears explain turn-1 success (first connect from true idle) vs turn-2 failure (residual from turn 1).

### Decision 4: Default test gate uses multi-turn regression script

**Choice:** Point the default headless smoke path (including `make test` if defined as build+smoke) at the existing multi-turn fixture `scripts/fixtures/multi-turn-pong.akoya-script` (or equivalent inline contract): two PONG-style turns with per-turn reply assertions and rejection of connection-failure lines between turns.

**Rationale:** Matches reported failure mode; harness already exists from automated-network-test-harnesses archive. Single-turn default script is too weak.

**Rejected — transport-test as default gate:** Does not exercise HTTP chat or inactive guard.

**Rejected — new script format:** Reuse `.akoya-script` harness and fixture.

### Decision 5: Keep transport-test as secondary entry point

**Choice:** No new transport-test requirements. Note in tasks/README that transport pass + chat multi-turn pass are both useful; only chat multi-turn satisfies the default gate per delta spec.

## Architecture sketch

```
Turn N (http_chat_run_turn)
┌──────────────────────────────────────────────────────────────┐
│ 1. assert tcp_transport_inactive()  ──fail→ connect error    │
│ 2. tcp_session_open → HTTP exchange → tcp_session_close        │
│ 3. bounded drain until tcp_transport_inactive()              │
│ 4. return status to session loop                             │
└──────────────────────────────────────────────────────────────┘
         │
         ▼
http_chat_session: emit_ok/emit_fail → console → ps2_read_line prompt
         │
         ▼
Turn N+1 begins at step 1 with inactive stack
```

## Risks / Trade-offs

- **[Risk] Drain loop timeout too short on slow LAN** → Use bounded timeout aligned with existing chat timeout constants; tune in apply if flakes appear.
- **[Risk] Drain loop masks deeper TCP bug** → Multi-turn regression + transport-test both run; inactive predicate remains asserted.
- **[Risk] Longer teardown adds latency between turns** → Accepted for correctness on this unikernel.
- **[Trade-off] Default test slower (two inference calls)** → Required to catch regression; pre-flight still aborts when inference down.

## Migration Plan

1. Fix TCP teardown and post-close drain in kernel (`kernel/net/tcp/tcp.c`, `kernel/net/http/http_chat.c`); verify headful turn 2+ manually when inference healthy.
2. Update default smoke/Makefile test wiring to multi-turn fixture in `scripts/run-qemu.sh` / `scripts/chat-script-harness.sh` / build test target.
3. Run multi-turn regression and full default gate; confirm transport-test still passes independently.
4. Rollback: revert kernel teardown changes and default script wiring; restore prior single-turn default smoke.

## Open Questions

- Whether input-wait polling is needed in addition to post-close drain — resolve during apply based on headful verification.
- Exact teardown timeout constant — align with `AKOYA_CHAT_TIMEOUT_MS` fraction or fixed ms budget during apply.
