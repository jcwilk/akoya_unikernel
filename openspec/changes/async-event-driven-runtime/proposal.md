## Why

The bootstrap unikernel today runs network bootstrap and the interactive chat session as a stack of synchronous poll-and-spin loops: keyboard input busy-spins on the integrated controller, TCP and HTTP turns block until each phase completes, and millisecond delays burn TSC cycles. That model is hard to extend, wastes CPU on a single-core Pentium M class target, and is a likely contributor to multi-turn transport timing failures when prior-turn teardown overlaps with the next prompt wait. An event-driven runtime—servicing keyboard, wired network, timers, and chat state from one cooperative loop with hardware wakeups—matches how the deployment unit's wired NIC and integrated keyboard actually signal work (IRQ-driven on metal) while preserving today's externally visible chat and network contracts.

## What Changes

- Introduce a **guest event runtime**: a single main loop that advances non-blocking work for every active interface (keyboard input, wired receive/transmit, protocol state machines, chat session state, scheduled timers) and yields CPU when nothing is pending.
- Replace **blocking waits** for operator input, inbound network data, outbound transport progress, and timed idle gaps with **event-scheduled progression**; only operations that complete in bounded immediate hardware time may stall the loop.
- Add **interrupt infrastructure** on the guest so wired NIC and keyboard activity can wake the runtime from idle without continuous polling (aligned with deployment-unit IRQ assignment for the RTL8139 and standard AT keyboard path).
- Refactor **network bootstrap and chat** into explicit state machines driven by the event runtime while keeping the same console-visible bootstrap order, per-turn transport lifecycle, and session semantics.
- Update **timed-gap regression** so inter-turn idle uses the same event-scheduled timing model as production (still within guest-timekeeping tolerance).
- Preserve **headless automation**, transport-test identity, and all fail-closed per-turn transport requirements—this is an execution-model change, not a relaxation of chat correctness.

## Capabilities

### New Capabilities

- `guest-event-runtime`: Central cooperative event loop, non-blocking execution policy, interface servicing order, CPU idle when quiescent, and hardware-wakeup integration for keyboard and wired network.

### Modified Capabilities

- `interactive-chat-session`: Input-wait and turn progression expressed as event-runtime work; observable prompt, editing, per-turn lifecycle, and fail-closed transport rules unchanged.
- `network-unikernel`: Bootstrap and upper-layer protocols advance through the event runtime without blocking the guest on network or timed waits.
- `bootstrap-unikernel`: Primary control flow is the event runtime until orderly halt after session end; clarify that the runtime is not a forbidden background service.
- `guest-timekeeping`: Millisecond deadlines and idle gaps scheduled through the event runtime rather than busy-wait loops.
- `timed-gap-chat-regression`: Inter-turn timed idle gaps use event-scheduled waiting with the same duration tolerance as today.

## Impact

- **Kernel core**: new interrupt and event-loop layer; `kernel_main` and boot entry transition to `sti` after setup.
- **Input**: keyboard path moves from busy-spin readline to queued scancodes drained by the loop.
- **Network**: DHCP, ARP, ICMP probe, TCP, and HTTP chat become state machines; link layer gains demultiplexing or fan-out suitable for concurrent timer and RX events.
- **Time**: `time_delay_ms` busy-wait replaced or narrowed to boot-time calibration; runtime delays become timer events.
- **Tests**: `make test`, timed-gap regression, transport-test, and headful smoke must pass with the new runtime; multi-turn chat connect failures observed in headful testing become an explicit acceptance target.
- **Out of scope this change**: WiFi, touchpad, IRQ-driven optimizations beyond wired NIC + keyboard, SMP, persistent background daemons, changing inference protocol or chat UX.
