## Context

The bootstrap unikernel targets Pentium M class i686 bare metal with an RTL8139 NIC (also emulated in QEMU). A ~3k-line in-house stack handles link ARP, IPv4, DHCP, ICMP, and TCP for HTTP chat. Turn-one chat works; after idle at `> `, turn two often fails with connection-failure text while the host inference endpoint is still reachable. Transport-only tests sometimes pass while interactive multi-turn chat fails — consistent with TCP/ARP lifecycle bugs, not driver RX failure.

Prior apply work (`simplify-chat-idle-and-verification`) simplified the chat loop and test harness but aborted because the bespoke TCP path remained flaky after a 20s idle gate.

## Goals / Non-Goals

**Goals:**

- Integrate **lwIP 2.x** in `NO_SYS=1` (no OS threads) as the production IPv4/TCP/ARP/DHCP/ICMP implementation for main chat and transport-test images.
- Keep **existing `rtl8139.c`** and `eth_device_t` poll/send API; add one **netif adapter** (frame ↔ stack buffers).
- Wire **runtime pump**: keyboard poll → NIC poll → `ethernet_input` → `sys_check_timeouts` → chat state.
- Rewire **http_chat** to open/send/recv/close via lwIP TCP API; keep JSON and console formatting.
- Pass **20s idle-at-prompt** automated gate on main kernel + transport-test suite.
- Vendor lwIP with version pin, `lwipopts.h` lean profile, and `LICENSE` / `NOTICE` attribution.
- Remove superseded in-house modules once parity verified.

**Non-Goals:**

- Replacing PS/2 keyboard, VGA console, or event-runtime architecture.
- IPv6, UDP services, TLS, or full BSD sockets surface (unless needed for chat).
- Swapping RTL8139 driver for a different OSS driver (current driver stays unless review finds defects).
- N-consecutive-run CI flake gate as apply-complete requirement.
- USB/ISO verification tier changes.

## Decisions

1. **lwIP `NO_SYS=1`** over continued in-house TCP patches — battle-tested ARP/TCP/DHCP/timer model; fits cooperative runtime. *Rejected:* more band-aids on `tcp.c`; uIP (too minimal for reliable HTTP chat).

2. **Keep RTL8139 driver** — link TX/RX works; adapter is thin. *Rejected:* importing GPL Linux `8139too`.

3. **Lean `lwipopts.h`** — enable: IPv4, ARP, DHCP, ICMP, TCP, timeouts; disable: IPv6, UDP apps, DNS, HTTP server, SLIP, PPP, sockets API unless chat path needs sequential API. Target minimal RAM on 512MB QEMU / 2GB target.

4. **Single stack for chat + transport-test** — both images link same vendored core + adapter; transport-test scenarios call lwIP TCP directly.

5. **Verification gate** — `make test` = main kernel `idle-at-prompt` script (20s host delay) + transport-test; retire `chat-regression-test` image and duplicate fixtures.

6. **Migration strategy** — integrate lwIP alongside old stack behind build flag only if needed for incremental bring-up; delete old stack before apply-complete (no long-term dual stack).

## Risks / Trade-offs

- **[Risk] Binary size / RAM** → Lean lwipopts; measure on `make build`; acceptable on 2GB target.
- **[Risk] Integration time** → Bounded vs open-ended TCP debugging; abort if gate not green within apply scope.
- **[Risk] Timer integration** → Map `time_millis()` to lwIP `sys_now`; call `sys_check_timeouts` every runtime pump; document in port layer.
- **[Trade-off] Modular-boundaries spec** → Still true at adapter boundary (driver ↔ netif ↔ app); internal lwIP modules are third-party.

## Migration Plan

1. Vendor lwIP; add port (`sys_arch`, `netif`, `lwipopts.h`).
2. Bootstrap DHCP + ICMP probe on lwIP; verify headless reachability line.
3. Chat TCP path on lwIP; headful idle repro.
4. Port transport-test scenarios; delete old `tcp.c`, `link.c` ARP path, custom DHCP once tests pass.
5. Consolidate test harness; archive change.

## Open Questions

- lwIP version pin (default: latest 2.2.x stable tag at apply time).
- Submodule vs `third_party/lwip/` vendor copy (default: vendored subtree with version file for offline builds).
