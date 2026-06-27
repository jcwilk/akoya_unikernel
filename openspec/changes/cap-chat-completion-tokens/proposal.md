## Why

Unbounded assistant completion output can flood the unikernel receive path, inflate HTTP response bodies, and stall or lock up interactive chat and automated verification. The project currently sends chat-completion requests without an explicit generation cap, leaving output size entirely to the inference service.

## What Changes

- Every outbound chat-completion request SHALL include a fixed upper bound of **500 tokens** on generated completion output for that turn.
- The bound applies to all turns in a session (first message and follow-ups alike).
- Lock the limit in the `http-chat-probe` behavioral contract so future changes cannot silently remove or raise it without review.

## Capabilities

### New Capabilities

_(none)_

### Modified Capabilities

- `http-chat-probe`: Chat-completion request payload must convey a five-hundred-token completion output cap on every inference turn.

## Impact

- HTTP chat client request body construction must include the completion output limit.
- Assistant replies may be truncated by the endpoint when responses would exceed the cap; console still shows plain reply text within that bound.
- Automated smoke and scripted harness runs inherit the cap without separate configuration.
