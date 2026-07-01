## Context

The main kernel boots asynchronously, then runs an interactive chat session. Operators and headful testing show: first inference turn succeeds; after waiting at `> `, the next turn often prints `chat failed: connect` while the inference endpoint is still reachable from the host. Parallel mitigations were added (ARP cache invalidation before each turn, periodic link announce while waiting, bounded `link_poll` frame counts, separate timed-gap regression image with guest-side timers, multiple near-duplicate script fixtures). Automated runs sometimes pass in isolation but do not match the operator's 100% headful failure mode.

Living specs already require event-driven input wait, per-turn transport isolation, and no spurious connect failures. The gap is implementation complexity and a verification strategy that exercised different code paths than production headful use.

## Goals / Non-Goals

**Goals:**

- One interactive chat control flow: at prompt, poll keyboard and wired network; on submit, run one blocking production turn; return to prompt.
- Network stays live during idle-at-prompt waits without deliberate cache invalidation or announce hacks.
- One TCP lifecycle: open → exchange → close → drain to inactive before showing the next prompt.
- One default CI gate on the **main chat unikernel**: message → host-timed idle at prompt → message → exit; fail on any connection-failure line.
- Delete timed-gap regression boot image, its build flag, and redundant runners/fixtures.

**Non-Goals:**

- Rewriting HTTP parsing, DHCP, or the transport-test image semantics.
- Changing inference endpoint configuration or deployment hardware targets.
- Re-architecting the entire async boot state machine (only chat idle/turn path is in scope).
- Mandating N-consecutive-run flake gates in spec (implementation may document a local stress loop; default gate is single pass).

## Decisions

1. **Single turn executor** — Keep `http_chat_run_turn` (blocking) as the only chat exchange path after boot. Remove chat-turn `tcp_sm` scheduling and guest-side gap timers for verification. *Alternative rejected:* async per-turn state machine (current source of drift and false confidence).

2. **Idle loop = poll input + poll network** — While at the input prompt, each runtime iteration services keyboard and drains wired receive until quiescent for that iteration (no fixed 16-frame cap during prompt wait). *Alternative rejected:* periodic announce + ARP invalidate (treats symptoms, worsens connect races).

3. **ARP on demand** — Resolve next-hop L2 address only when sending requires it; do not invalidate a valid cache entry at turn boundaries. *Alternative rejected:* invalidate-before-connect (guarantees cold ARP on every turn).

4. **Unified wait policy** — Use wall-clock deadlines from the calibrated millisecond time base for ARP resolve, TCP open, and recv; avoid nested special-case drain budgets. Teardown uses one close-and-drain sequence. *Alternative rejected:* separate chat-drain vs transport-drain helpers with different failure semantics.

5. **Verification on production path** — Default `make test` runs the main kernel headlessly with one script: short message, ~20s host delay while guest sits at prompt, short second message, quit. Transport test remains a separate entry point. *Alternative rejected:* separate regression boot image with keyboard-free guest timers (different idle mechanics than headful).

## Risks / Trade-offs

- **[Risk] Full RX drain each iteration costs CPU during long idle** → Acceptable on target hardware; prefer correctness over calibrated caps; runtime still enters `hlt` when quiescent per existing policy.
- **[Risk] Removing regression image drops guest-timed gap coverage** → Host-delay script on main image is the authoritative repro; guest-timekeeping tolerance scenarios move to interactive verification wording.
- **[Risk] Flaky CI from emulation/macvtap** → Document single-run gate; optional local consecutive-run script stays out of default `make test` unless stabilized later.

## Migration Plan

1. Implement simplified chat idle and TCP/link policy on a working branch.
2. Remove regression image from build and delete redundant test scripts/fixtures; point `make test` at the consolidated gate.
3. Archive this change to update living specs (retire `timed-gap-chat-regression`, modify `dev-test-runner` and related domains).
4. Rollback: revert branch; restored regression image and old runners remain in git history.

## Open Questions

- Exact host idle duration for the default gate (proposal: 20 seconds — matches operator repro).
- Whether transport test stays in `make test` or becomes a documented secondary command (default: keep both in `make test` as today).
