## Why

Interactive chat on the main kernel image fails reliably after the operator waits at the input prompt, with spurious connection-failure outcomes even when the inference endpoint remains reachable. The guest stack accumulated parallel chat paths, tuned polling limits, and defensive network hacks that did not fix the failure while automated verification diverged from the headful reproduction operators actually care about.

## What Changes

- **BREAKING:** Retire the timed-gap chat regression boot identity and its dedicated automated entry point; multi-turn idle-at-prompt health is verified on the **main interactive chat unikernel** instead.
- Simplify the interactive chat idle loop to two responsibilities: poll operator input and poll wired network, with one production turn path per message.
- Remove accumulated band-aids (per-turn address-cache invalidation, periodic announce refresh, parallel async turn machinery, redundant special-case timeout branches) in favor of one predictable poll/teardown policy while **keeping** bounded work per runtime visit.
- Collapse the development verification suite to a small set of focused gates: default multi-turn idle-at-prompt chat on the main image, transport-only verification, and documented headful manual check.
- Strengthen the default automated gate to match headful reproduction: short first message, **twenty-second** idle at the prompt, short second message, zero tolerance for connection-failure lines between successful turns.

## Capabilities

### New Capabilities

_None — behavior is consolidated into existing domains._

### Modified Capabilities

- `interactive-chat-session`: explicit idle-at-prompt follow-up reliability; simpler idle servicing contract.
- `dev-test-runner`: new default multi-turn gate on the main chat unikernel with host-timed idle gap; remove timed-gap regression runner requirements.
- `guest-event-runtime`: drop timed-regression-only idle scenario; retain bounded work per visit.
- `guest-timekeeping`: decouple millisecond accuracy scenarios from the retired regression image.

### Removed Capabilities

- `timed-gap-chat-regression`: entire capability retired; idle-at-prompt coverage moves to main-session verification.

## Impact

- Guest networking and chat modules, event runtime pump policy, TCP session lifecycle.
- Build outputs (one fewer boot image), Makefile `test` target, headless script fixtures and harness runners.
- Living specs and README verification narrative (via archive after apply).
