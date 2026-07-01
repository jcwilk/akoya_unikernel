## Why

Interactive chat on the main kernel fails reliably after the operator waits at the input prompt, with spurious connection-failure outcomes while the inference endpoint remains reachable. Iterative fixes to the in-house IPv4/TCP/ARP stack have not stabilized the behavior. The wired driver and guest input/runtime paths work; the failure class lives in connection lifecycle and link-layer resolution between turns.

## What Changes

- **BREAKING:** Replace the in-house IPv4, ARP, DHCP client, ICMP probe transport, and TCP implementation used for production chat and transport-test images with a **maintained embedded networking stack** (vendored third-party, BSD-licensed) integrated in bare-metal cooperative mode.
- **Keep:** existing RTL8139 driver surface, PCI bring-up, PS/2 keyboard, console, guest event runtime, and HTTP chat application logic (request/response framing and JSON).
- Add a thin link adapter between the existing Ethernet poll/send device and the vendored stack; service stack timers on each runtime pump pass.
- Retire custom `tcp`, `link` ARP/TCP machinery, and related mitigation code paths once parity is verified.
- Consolidate automated verification: main kernel idle-at-prompt gate (20s host-timed wait between turns), transport-test image, headful manual check.
- Retire timed-gap chat regression boot identity and duplicate test runners/fixtures (carries forward intent from the aborted simplify-chat-idle-and-verification apply).

## Capabilities

### New Capabilities

_None — behavior stays under existing network and chat domains._

### Modified Capabilities

- `network-unikernel`: production IPv4/TCP/ARP/DHCP/ICMP probe delivered via maintained stack; driver boundary preserved.
- `network-transport-test`: scenarios still exercise the same production stack (new implementation underneath).
- `interactive-chat-session`: reliable follow-up turns after substantial idle at the input prompt.
- `guest-event-runtime`: stack timer and receive servicing integrated with input-wait loop.
- `dev-test-runner`: default gate on main kernel with twenty-second idle-at-prompt between turns; retire timed-gap regression runner.
- `guest-timekeeping`: millisecond deadlines used by stack timers remain credible (wording cleanup only).

### Removed Capabilities

- `timed-gap-chat-regression`: retired; idle-at-prompt verified on main kernel.

## Impact

- Large swap of `kernel/net/` implementation; build adds vendored third-party sources and license notice.
- Transport-test and main chat images share one stack.
- Supersedes further patching of the in-house TCP state machine. Partial harness work may be salvaged from branch `explore/simplify-chat-idle-and-verification-idle-at-prompt`.

## Supersedes

- **`simplify-chat-idle-and-verification`** (aborted apply): same acceptance bar, different root fix — replace stack instead of tuning the bespoke one.
