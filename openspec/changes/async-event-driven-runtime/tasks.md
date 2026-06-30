## 1. Interrupt and idle foundation

- [ ] 1.1 Add IDT, 8259 PIC, and EOI helpers under `kernel/arch/` (or equivalent); map IRQ 1 keyboard and IRQ 11 wired NIC per deployment observation
- [ ] 1.2 Enable interrupts (`sti`) after early console init; preserve `cli` only where required for critical sections
- [ ] 1.3 Implement `kernel/event/` runtime skeleton: loop entry, active-slot tracking, `hlt` idle when quiescent with configurable short horizon
- [ ] 1.4 Wire `kernel_main` to enter event runtime after console init instead of calling blocking `net_bootstrap()` directly

## 2. Timer events

- [ ] 2.1 Add timer wheel or ordered deadline list using calibrated TSC base; expose schedule/cancel/check APIs
- [ ] 2.2 Route runtime millisecond waits (gaps, DHCP/TCP/chat timeouts) through timer events; restrict `time_delay_ms` to boot calibration and immediate hardware settle per design
- [ ] 2.3 Unit smoke: scheduled 500 ms gap within 10% under `make test` timed-gap path

## 3. Keyboard path

- [ ] 3.1 IRQ1 handler enqueues scancodes to fixed ring; overflow counted for diagnostics
- [ ] 3.2 Refactor line editor to consume queue in INPUT state (US layout, echo, Backspace, Enter unchanged)
- [ ] 3.3 Remove busy-spin from `ps2_read_line` call path used by chat session

## 4. Wired network path

- [ ] 4.1 IRQ11 handler acknowledges RTL8139 and signals RX/TX pending; keep frame parsing in loop context
- [ ] 4.2 Refactor `rtl8139_poll` drain to be callable from event loop without inner guest-wide spin loops
- [ ] 4.3 Replace single global IPv4 handler with dispatcher fan-out (TCP, ICMP, UDP/DHCP) per design D5

## 5. Async network bootstrap

- [ ] 5.1 Convert DHCP acquisition to resumable state machine driven by event runtime
- [ ] 5.2 Convert ARP resolve and single ICMP connectivity probe to resumable state machines
- [ ] 5.3 Preserve console-visible bootstrap order: link → DHCP → address line → console clear → probe → chat handoff
- [ ] 5.4 Run headless bootstrap smoke; capture serial log showing unchanged diagnostic sequence

## 6. Async chat and transport

- [ ] 6.1 Convert TCP session lifecycle (open, send, recv-until, close, inactive check) to resumable states
- [ ] 6.2 Convert `http_chat_run_turn` and `http_chat_session` to chat controller states preserving fail-closed per-turn ordering
- [ ] 6.3 Ensure input-wait at prompt services transport release and timers concurrently (targets turn-3 connect class)
- [ ] 6.4 Update timed-gap regression entry to schedule inter-turn gaps via timer events

## 7. Validation and documentation

- [ ] 7.1 `make test` — timed-gap regression and transport paths pass
- [ ] 7.2 `make verify-usb` — deploy-faithful boot tier passes
- [ ] 7.3 Headful manual smoke: three-turn chat without spurious `chat failed: connect` when endpoint reachable; document command and outcome in apply notes
- [ ] 7.4 README: brief note that runtime is event-driven with IRQ wake + idle; headless automation unchanged
- [ ] 7.5 `npx @fission-ai/openspec@latest validate async-event-driven-runtime --type change`

## Explicitly deferred

- Touchpad or WiFi IRQ integration
- IOAPIC/MSI, HPET periodic IRQ, or ACPI C-states beyond `hlt`
- IRQ-driven TX completion separate from existing slot polling (unless metal bring-up requires)
- Headful automated assertion harness
- Preemptive threads, fibers, or second CPU support
