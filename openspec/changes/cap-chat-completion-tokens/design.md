# Design: Cap Chat Completion Output Tokens

## Context

The main chat unikernel builds OpenAI-shaped JSON bodies in the HTTP chat client and posts them to the configured LAN inference endpoint. There is no generation cap today; verbose model replies can produce large HTTP bodies that stress the fixed receive buffer and slow multi-turn sessions.

The operator requested a **500-token** cap for now, expressed as a durable spec requirement rather than an ad-hoc code tweak.

## Goals / Non-Goals

**Goals:**

- Include a completion output limit of **500** in every chat-completion request body.
- Use the same limit on first and follow-up turns.
- Default build/profile uses 500 unless a future change explicitly revises the spec.

**Non-Goals:**

- Streaming responses, dynamic per-message limits, or operator-configurable caps via chat UI.
- Changing receive buffer sizes or TCP behavior in this change.
- Endpoint-side enforcement guarantees beyond what the request asks for.

## Decisions

### Decision 1: Add limit to JSON body alongside model and messages

**Choice:** Extend the existing chat-completion JSON builder to append a numeric completion output limit field (OpenAI-compatible `max_tokens` spelling) set to **500** for all requests.

**Rationale:** Standard field understood by the configured endpoint; minimal diff in `kernel/net/http/http_chat.c`.

**Rejected — HTTP header or query param:** Endpoint expects JSON body fields; non-standard.

### Decision 2: Compile-time default of 500

**Choice:** Default **500** via build define (e.g. `AKOYA_CHAT_MAX_TOKENS`) wired through `scripts/build.sh`, overridable only at build time for development experiments—not at runtime from chat UI.

**Rationale:** Spec locks 500 for deployment profile; build-time override keeps apply simple without violating operator intent for production images.

### Decision 3: No console indication of truncation

**Choice:** If the endpoint truncates due to the cap, print whatever assistant text is returned (existing plain-reply path). Do not add truncation banners in this change.

**Rationale:** Scope is request-side cap; truncation visibility is a follow-on if needed.

## Risks / Trade-offs

- **[Risk] Legitimate long answers clipped at 500 tokens** → Acceptable for now on resource-constrained unikernel; raise only via spec change.
- **[Risk] Endpoint ignores limit field** → Mitigated by choosing compatible inference service; harness tests still bounded if server honors request.

## Migration Plan

1. Update JSON builder and build define.
2. Rebuild main kernel; run existing headless smoke to confirm requests still succeed.
3. Archive spec delta.

Rollback: remove limit field from JSON builder.

## Open Questions

- None for apply; 500 is fixed per operator request.
