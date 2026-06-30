## Context

**Current state** (from codebase review):

- Boot runs with interrupts disabled (`cli` in `entry.S`); no IDT or IRQ handlers.
- `kernel_main` calls blocking `net_bootstrap()` then `hlt` forever.
- Chat is `http_chat_session()`: `ps2_read_line()` busy-spins → blocking `http_chat_run_turn()` (TCP open/send/recv/close with inner `while(deadline){link_poll()}` loops).
- RTL8139 sets interrupt mask bits but all RX/TX is polled via `rtl8139_poll()` during spin loops.
- i8042 keyboard polled via status port; no scancode queue.
- `time_delay_ms()` is TSC busy-wait; `time_poll()` is an empty stub.
- IPv4 dispatch allows only one registered handler at a time (`link_register_ipv4_handler`).

**Deployment hardware** (`target/akoya_ex.yaml` observation batch):

| Device | Observation | Runtime implication |
|--------|-------------|---------------------|
| CPU | Pentium M 735, single core, TSC + PIT | Cooperative loop + `hlt` is appropriate; no SMP |
| Chipset LPC | Intel ICH4-M | Legacy 8259 PIC present; standard IRQ routing |
| Wired NIC | RTL8139 rev 10, BDF 01:02.0, **IRQ 11**, I/O BAR | Hardware signals RX/TX via IRQ; ring buffer already in driver |
| Keyboard | i8042 AT translated set 2 (`serio0`) | IRQ 1 on standard PC/ICH paths; enqueue scancodes in ISR |
| Touchpad | Synaptics PS/2 (`serio3`) | Out of scope; keyboard IRQ path only |

**QEMU harness** uses the same emulated RTL8139 and i8042 PS/2 paths; IRQ delivery works when IDT is installed—polling-only was a software choice, not a hardware limitation.

**Stakeholders**: operators on Akoya EX headful chat, headless CI (`make test`, timed-gap regression), and transport-test harness.

## Goals / Non-Goals

**Goals:**

- One cooperative **event runtime** owns all post-console-init work until orderly halt.
- **No blocking** on operator input, inbound frames, outbound TCP/HTTP progress, or timed idle—only immediate bounded hardware register operations in the hot path.
- **Round-robin or fair rotation** across active interfaces each loop iteration so no queue (keyboard RX ring, NIC RX ring, timer wheel) overflows or goes stale during chat or bootstrap.
- **CPU idle** when every interface is quiescent and no timer is due: `hlt` with interrupts enabled, wake on IRQ or timer tick.
- Preserve **all living-spec chat contracts**: per-turn inactive transport, fail-closed teardown, conversation history, US keyboard semantics, explicit exit.
- Preserve **network bootstrap observables**: link → DHCP → address line → console clear → single ICMP probe → chat.
- Fix or materially reduce **multi-turn `chat failed: connect`** class failures by eliminating prompt-wait spin during transport drain.

**Non-Goals:**

- WiFi, touchpad, USB, storage IRQs, or full chipset emulation.
- Listening servers, worker threads, or preemptive multitasking.
- Replacing fail-closed transport lifecycle with optimistic retry.
- IOAPIC/MSI, HPET-only timing, or ACPI-driven idle—8259 + PIT/TSC is sufficient for this unikernel.
- Headful automated assertion harness.
- Changing chat UX, inference payload shape, or token cap.

## Decisions

### D1 — Hybrid IRQ wake + cooperative event loop (not pure poll burn)

**Choice:** Install 8259 PIC + IDT. Minimal ISRs for **IRQ 1** (keyboard) and **IRQ 11** (RTL8139 on deployment unit) enqueue events and EOI; the main loop dequeues and advances state machines. When the loop finds no ready work and no timer due within a short horizon, execute `hlt` until the next IRQ.

**Alternatives:**

- *Pure polling loop with periodic `pause`* — rejected; user explicitly asked to avoid CPU burn and matches metal poorly when IRQs exist.
- *Fully IRQ-driven stack with no main loop* — rejected; TCP/HTTP/DHCP state machines still need a single cooperative scheduler; unikernel stays non-preemptive.

### D2 — Software event queues between ISR and loop

**Choice:** Fixed-size ring buffers for scancodes and NIC RX frame notifications (or frame metadata pointing into existing RTL8139 DMA ring). ISRs only mark pending work; parsing and protocol logic run in the loop.

**Alternatives:** Process packets inside ISR — rejected (latency, reentrancy). Share existing `rtl8139_poll` body from loop only — preferred refactor path.

### D3 — Single runtime scheduler with interface slots

**Choice:** Each loop pass visits, in stable order: **timers** → **keyboard queue** → **NIC / link RX drain** → **active protocol machines** (DHCP, ARP, ICMP, TCP) → **chat session controller**. Track per-slot "active" flag; skip idle slots cheaply. If all slots idle and next timer > idle threshold (~1–5 ms tunable), `hlt`.

**Alternatives:** Priority queue of events — deferred; fixed slots match bounded unikernel scope.

### D4 — State machines replace blocking network/chat APIs

**Choice:** Convert `dhcp_acquire`, `icmp_ping`, `tcp_session_*`, and `http_chat_run_turn` into resumable states invoked from the loop. Synchronous wrappers may remain temporarily for transport-test only if needed, but main kernel and regression identities use async path exclusively.

**Alternatives:** Fibers/coroutines — rejected; explicit states are debuggable in freestanding C.

### D5 — IPv4 demultiplexing for concurrent bootstrap/chat

**Choice:** Replace single global IPv4 handler with a small dispatcher fan-out (TCP, ICMP, DHCP/UDP) or a registered-handler list so RX during TCP close does not starve timer processing.

**Alternatives:** Keep one handler — rejected; blocks async bootstrap + chat coexistence.

### D6 — Timer wheel backed by calibrated TSC

**Choice:** Keep PIT-cross-check calibration at boot (`guest-timekeeping` unchanged in outcome). Runtime deadlines (DHCP timeout, TCP recv timeout, chat turn timeout, timed-gap idle) become **timer events** checked each loop iteration; `time_delay_ms` narrowed to calibration and immediate post-reset hardware settle only.

**Alternatives:** PIT periodic IRQ — acceptable follow-up; TSC compare in loop is simpler first step.

### D7 — Chat input without busy-spin

**Choice:** Line editor consumes scancodes from the keyboard queue when the session is in INPUT state; between keys the loop services network and timers so prior-turn transport drain can complete while the prompt is visible.

**Alternatives:** Block in `ps2_read_line` until Enter — rejected by user intent.

### D8 — Preserve fail-closed transport semantics in async form

**Choice:** Chat turn controller states mirror today's sequence: verify inactive → connect → send → recv → parse → close → verify inactive. Timeouts transition to categorized failure states; no silent connect retry. Prompt only after inactive + outcome printed.

**Alternatives:** Background connect while typing — rejected; violates strict ordering in living specs.

## Risks / Trade-offs

| Risk | Mitigation |
|------|------------|
| IRQ handler bugs cause lost keyboard or RX frames | Bounded rings with overflow counters; regression tests; headful manual smoke |
| State-machine complexity regressions | Incremental migration: loop + IRQ first, then DHCP/ICMP, then TCP/chat; keep transport-test identity |
| `hlt` race with timer due | Always check timer wheel before `hlt`; minimum idle threshold |
| QEMU vs metal IRQ routing differs | Verify in QEMU first; document IRQ line in design notes; metal smoke on deployment unit when available |
| Single-handler removal breaks assumptions | Audit all `link_register_ipv4_handler` call sites during apply |

## Migration Plan

1. Land IDT/PIC/EOI + empty loop + `hlt` idle (no behavior change visible).
2. Keyboard IRQ + queue; refactor line editor to event-driven.
3. NIC IRQ + RX notification; loop drains RTL8139 ring.
4. Timer events; replace `time_delay_ms` in runtime paths.
5. Async DHCP → ARP → ICMP bootstrap chain.
6. Async TCP/HTTP chat turn machine; wire `http_chat_session`.
7. Update timed-gap regression to schedule gaps via timer events.
8. Run `make test`, `make verify-usb`, headful multi-turn smoke; fix connect-on-turn-3 class if still present.

**Rollback:** Revert branch; prior synchronous stack remains in git history.

## Open Questions

- Exact idle threshold before `hlt` (1 ms vs 5 ms) — tune during apply against `make test` wall time and headful responsiveness.
- Whether transport-test image shares the full runtime or keeps a thin sync shim for isolation — prefer shared runtime if it does not expand scope.
