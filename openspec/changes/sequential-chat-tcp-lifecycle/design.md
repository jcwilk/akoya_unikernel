# Design: Sequential Chat Transport Lifecycle

## Context

The bootstrap unikernel runs network diagnostics, then an interactive chat session. Living specs already require multi-turn conversation history, plain console replies, and one inference request per non-exit user message. Uncommitted work on branch `cursor/49a5d967` introduced experimental **persistent outbound transport** (`tcp_session_*` style reuse across turns) and related refactors (including a PS/2 readline split).

The operator has corrected architectural intent: **transport is per-turn, not session-persistent.** Each submitted user message must spin up outbound transport from a clean inactive state, complete one chat-completion exchange through response extraction, fully tear down transport, and only then resume conversation UI (print reply, show input prompt). Conversation history remains in memory across turns; only transport is isolated per turn.

This change **supersedes** the persistent-transport direction. Apply should implement sequential lifecycle behavior, not extend keep-alive sessions.

## Goals / Non-Goals

**Goals:**

- One outbound transport lifecycle per non-exit user message: activate → exchange → extract response → fully deactivate.
- Strict turn ordering aligned with specs: no assistant reply printed while transport for that turn is still active; no next operator input accepted until the prior turn's transport is fully released and its outcome printed.
- Preserve in-memory conversation history and full `messages` payload on every turn.
- Keep headless smoke tests able to drive multi-turn scripts without manual input; runner assertions stay on plain reply text, reachability, and prompt visibility.

**Non-Goals:**

- Persistent keep-alive outbound sessions shared across turns.
- Showing assistant reply while transport is still active (streaming or early print before teardown).
- Changing keyboard input path, US layout, minimal `> ` prompt, exit command semantics, or plain-reply console formatting from the minimal-chat-console direction.
- TLS, DNS, streaming, or on-disk conversation persistence.
- Spec-level naming of TCP APIs, connection headers, ports, or file paths (those belong here and in tasks).

## Decisions

### Decision 1: Per-turn transport scope, not session scope

**Choice:** Outbound chat transport is owned by a single inference turn. At turn start, transport is inactive. At turn end (success or failure), transport is fully released with no active connection, receive path, or turn-specific state carried into the next turn.

**Rationale:** Matches operator intent; prevents cross-turn coupling bugs; keeps failure domains local to one message.

**Rejected alternative — persistent session transport:** Reuse one TCP connection (or shared session object) across turns. **Rejected** because the operator explicitly forbids shared transport across turns and reply printing while transport remains active.

### Decision 2: Strict turn ordering in the session loop

**Choice:** Session loop orchestration follows this sequence for each non-exit message:

```
operator submit
  → activate outbound transport (clean state)
  → run chat-completion exchange through response extraction
  → fully deactivate outbound transport
  → print assistant reply or failure outcome (plain text)
  → show input prompt and wait for next operator input
```

**Rationale:** Specs encode observable ordering; implementation must not interleave UI updates with active transport for the same turn.

**Rejected alternative — print while connected:** Emit assistant text before transport teardown to reduce perceived latency. **Rejected** — violates operator ordering requirement.

### Decision 3: History buffer independent of transport

**Choice:** Conversation history (user/assistant turns in memory) accumulates across the session. Each turn builds the JSON payload from history, then runs an isolated transport lifecycle. History retention MUST NOT depend on keeping transport open.

**Rationale:** Separates conversational state from network state; follow-up coherence unchanged.

### Decision 4: Headless smoke tests unchanged in contract

**Choice:** `scripts/run-qemu.sh` (and related runner logic) continues to assert plain assistant reply lines, reachability output, and minimal prompt visibility. Multi-turn default/custom scripts remain valid; no new assertion that transport stays open between injected keystrokes.

**Rationale:** Transport lifecycle is an internal implementation constraint; external smoke behavior stays conversation-focused.

## Per-turn sequencing diagram

```
Turn N (non-exit message)
┌─────────────────────────────────────────────────────────────┐
│ 1. Operator submits message                                 │
│ 2. Transport: inactive → active (fresh for this turn)       │
│ 3. HTTP chat-completion exchange runs to completion         │
│ 4. Assistant text extracted (held until step 5 completes)   │
│ 5. Transport: active → fully inactive (teardown complete)   │
│ 6. Print reply or failure (plain conversation text)         │
│ 7. Show `> ` prompt; wait for operator input                │
└─────────────────────────────────────────────────────────────┘
         │
         ▼
Turn N+1 begins at step 2 from clean inactive transport
(history from turns 1..N still in memory for JSON payload)
```

## Transport teardown checklist (implementation)

Before returning from an inference turn to session UI logic, verify:

- [ ] No active outbound connection for this turn
- [ ] No turn-specific receive buffer or parser state retained
- [ ] No session-scoped transport handle reused for the next turn
- [ ] Failure paths (timeout, refused, HTTP error, parse error) also complete teardown before printing outcome

## Risks / Trade-offs

- **[Risk] Experimental persistent-transport code on branch conflicts with this design** → Apply removes or replaces session-reuse paths; do not merge both models. See Migration Plan.
- **[Risk] Teardown bugs leave ghost state affecting turn N+1** → Teardown checklist + headless multi-turn script catches cross-turn leakage.
- **[Trade-off] New connection per turn adds latency** → Accepted; operator prefers correctness and isolation over keep-alive performance on this unikernel.

## Migration Plan

1. **Revert or replace persistent transport experiment** on the working branch (`tcp_session_*`, shared connection across `http_chat_*` calls). Replace with per-turn activate/exchange/teardown helpers scoped to one message.
2. **Refactor interactive session loop** (`kernel/net/netmain.c` or chat session module) so reply printing and next prompt occur only after turn transport is fully released.
3. **Align HTTP chat client** (`kernel/net/http/http_chat.c`) with turn-scoped transport: build payload from history, run exchange, extract assistant text, tear down before returning to session layer.
4. **TCP layer** (`kernel/net/tcp/tcp.c`): ensure close/teardown clears connection and receive state; no global session singleton surviving across turns.
5. **Verify headless runner** still passes with default/custom multi-turn scripts; update timing waits if teardown adds delay, without changing assertion contracts.
6. **Rollback:** Restore prior per-turn new-connection behavior without persistent session (if persistent code landed); do not roll back to persistent reuse.

## Open Questions

- Exact teardown API shape (function boundaries between TCP, HTTP client, and session loop) — resolve during apply; behavior is fixed by specs.
- Whether experimental `ps2_readline` split rides along in the same apply branch or a follow-on — does not change transport lifecycle requirements.
