# Design: Strict Fail-Closed Chat Transport

## Context

The bootstrap unikernel runs network diagnostics, then an interactive chat session. Living specs already require per-turn outbound transport isolation, full release before reply print and next prompt, and multi-turn conversation history. Archived changes (`sequential-chat-tcp-lifecycle`, `fix-multi-turn-chat-transport`) established ordering and multi-turn regression gates, but implementation still uses pragmatic teardown:

- `HTTP_CHAT_TEARDOWN_MS` (1000 ms) bounded drain after close in `http_chat_prepare_transport()`.
- `tcp_drain_until_inactive()` calls `tcp_transport_release()` (force-clear) when the deadline expires, then returns whether inactive is true—allowing proceed-after-force-release.
- `http_chat_run_turn()` retries `tcp_session_open()` once after prepare when the first open fails—masking incomplete prior-turn teardown as a transient connect failure.

Headful operators still see turn 2+ connect failures when inference is reachable. Transport-test passes because it exercises raw TCP connect/close without the HTTP chat path or the session idle-at-prompt gap. Host-side keyboard scripts inject the next message immediately and do not reproduce the timed idle at `> ` where no link polling runs.

The operator has directed a conservative fail-closed model: transport MUST reach fully inactive before proceeding OR fail loudly with a categorized outcome—no silent force-release, no connect retry masking, no best-effort proceed to next turn.

## Goals / Non-Goals

**Goals:**

- Fail-closed turn boundary: inactive transport before reply print and before next input prompt; categorized transport-lifecycle failure when inactive cannot be reached.
- Remove force-release-on-timeout and connect-retry masking from the chat turn path.
- Timed-gap chat regression boot identity reusing production chat turn code (`http_chat_run_turn`, session loop semantics) with bounded timed blocking at the input prompt between auto-submitted turns.
- Default automated verification gate runs timed-gap multi-turn regression (minimum three consecutive turns with inter-turn delay); transport-test remains complementary, not sufficient.

**Non-Goals:**

- Persistent keep-alive outbound sessions across turns.
- Replacing per-turn transport with a mock or forked stack.
- Headful automated assertion harness; bare-metal Akoya EX automated verification.
- Changing plain-reply console formatting, exit command semantics, US keyboard layout, or token cap behavior.
- Spec-level naming of C symbols, timeout constants, or file paths (those belong here and in tasks).

## Decisions

### Decision 1: Fail-closed inactive predicate at turn boundary

**Choice:** After close, poll until `tcp_transport_inactive()` is true or a bounded teardown budget expires. On expiry, return a dedicated transport-lifecycle failure category (e.g. `HTTP_CHAT_FAIL_TRANSPORT_LIFECYCLE` mapped to console text like `chat failed: transport-lifecycle`) and do **not** call `tcp_transport_release()` to force-clear and proceed. Do **not** print assistant reply or show next prompt as if teardown succeeded.

**Rationale:** Force-release hides incomplete teardown and defers failure to ambiguous connect errors on the next turn. Fail-closed surfaces the defect at the turn where it occurs.

**Rejected — keep force-release in `tcp_drain_until_inactive` for chat path:** Masks bugs; operator explicitly rejected this pattern for chat turns.

**Rejected — remove inactive guard entirely:** Would allow turns to start with ghost state.

### Decision 2: Remove connect retry masking in `http_chat_run_turn`

**Choice:** Single connect attempt per turn after verifying inactive state. If open fails, categorize and return without a silent retry after prepare. Prior-turn incomplete release must fail at prior turn boundary (Decision 1), not be recovered by retry.

**Rationale:** Retry after `http_chat_prepare_transport()` was added to paper over turn-2 connect failures from stale state; it violates fail-closed intent.

**Alternative — retry only for genuine wire timeout:** Rejected; indistinguishable from stale-state failure without inactive guarantee.

### Decision 3: Timed-gap chat regression as distinct boot identity

**Choice:** Add `chat-regression-test` logical boot identity (name in tasks/Makefile) linking the same production chat sources as the main unikernel. Entry runs a fixed schedule: submit predetermined user messages, wait bounded duration at input prompt using guest-side timed blocking (same delay primitive class as transport-test), repeat for N turns, print per-turn pass/fail and aggregate summary, then halt.

**Rationale:** Exercises production chat turn path including idle-at-prompt gap without keyboard input or host-side script timing alone. Distinct identity avoids polluting main operator image and matches transport-test pattern.

**Rejected — host-only script delays in `.akoya-script`:** Host injection timing does not reproduce guest blocking at prompt with no link polling; insufficient for this regression class.

**Rejected — extend transport-test image:** Does not run HTTP chat or session loop.

### Decision 4: Default gate uses timed-gap regression with N≥3 turns

**Choice:** Point default automated verification (`make test` / primary headless gate) at timed-gap chat regression entry point with at least three consecutive turns and inter-turn delay (e.g. 5 s guest-side, tunable in design/tasks only). Assert plain assistant reply per turn and no connection-failure or transport-lifecycle failure lines between successful turns.

**Rationale:** Two-turn keyboard scripts miss timing-sensitive teardown bugs; three turns with idle gaps better mimic headful pacing and reduce coin-flip false passes.

**Rejected — transport-test as default gate:** Does not exercise chat path (unchanged from prior archive).

### Decision 5: Separate drain helper for chat vs transport-test

**Choice:** Introduce chat-specific drain/wait that fails closed (no force-release) for `http_chat_prepare_transport()`. Leave transport-test drain behavior unchanged or align later; chat path must not inherit force-release semantics.

**Rationale:** Transport-test may still use force-release for scenario progression; chat must not.

## Per-turn fail-closed sequencing

```
Turn N (non-exit message)
┌──────────────────────────────────────────────────────────────┐
│ 1. Assert outbound transport fully inactive (else fail loud) │
│ 2. Activate outbound transport (single connect attempt)      │
│ 3. HTTP chat-completion exchange through response extraction │
│ 4. Close connection; poll until inactive or budget expires   │
│    └─ on expiry → categorized transport-lifecycle failure    │
│       (NO force-release proceed)                             │
│ 5. On inactive: print assistant reply or categorized failure │
│ 6. Show input prompt; idle gap (no active outbound transport)│
└──────────────────────────────────────────────────────────────┘
         │
         ▼ (timed-gap regression: wait N ms at prompt, auto-submit)
Turn N+1 begins at step 1 from provably inactive state
```

## Risks / Trade-offs

- **[Risk] Stricter teardown causes more visible failures during apply** → Expected; surfaces real bugs rather than masking. Multi-turn regression gate catches regressions.
- **[Risk] Teardown budget too short on slow LAN** → Tune constant in apply; fail-closed still preferable to silent proceed.
- **[Risk] Three-turn default gate slower** → Accepted; pre-flight still aborts when inference down.
- **[Trade-off] No connect retry increases fragility to genuine transient wire glitches** → Accepted; operator prefers correctness over retry masking on this unikernel.

## Migration Plan

1. Add chat-specific fail-closed drain; remove force-release proceed and connect retry from `kernel/net/http/http_chat.c`.
2. Map new transport-lifecycle failure to plain console category in session loop (`kernel/net/netmain.c` / `http_chat_session`).
3. Implement timed-gap chat regression boot image (`kernel/net/chat_regression_main.c` or shared module) and Makefile target.
4. Add dev-test-runner entry point; wire default gate to timed-gap regression (≥3 turns, inter-turn delay).
5. Verify transport-test still passes independently; run headful manual multi-turn check when inference healthy.
6. **Rollback:** Revert kernel teardown strictness and regression image wiring; restore prior pragmatic teardown (not recommended).

## Open Questions

- Exact teardown budget for chat turns — align with existing chat timeout fraction during apply; must be long enough for real FIN/RST drain on LAN.
- Whether `tcp_drain_until_inactive` force-release should be removed globally or only bypassed on chat path — resolve during apply (Decision 5 leans chat-specific helper).
- Minimum inter-turn delay for regression (5 s proposed) — confirm during apply if flakes appear.
