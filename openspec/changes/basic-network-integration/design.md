## Context

The repository ships a minimal i686 multiboot bootstrap kernel with serial/VGA console output and a QEMU smoke-test runner. Hardware inventory confirms the Medion Akoya EX has 10/100 Mbit Ethernet (RJ-45) but does not yet identify the exact NIC controller chip. The bootstrap image currently halts after printing a fixed diagnostic string; no network code exists.

The human wants the next increment to exercise real DHCP and TCP/IP on both emulated and bare-metal paths, with modular structure for later protocols and drivers.

## Goals / Non-Goals

**Goals:**

- Bring up wired Ethernet, complete DHCP for IPv4, print the leased address, and perform one ICMP echo (ping) to a configured public host, reporting reachability and round-trip time on the console.
- Organize kernel networking as layered modules with narrow interfaces (NIC driver ↔ link layer ↔ IPv4 ↔ DHCP ↔ ICMP ↔ bootstrap orchestration).
- Extend the QEMU runner so the guest NIC is bridged to the development workstation LAN with a fixed guest MAC address.
- Extend headless smoke tests to assert the network diagnostic lines when network bootstrap is the active image mode.
- Preserve existing console-only bootstrap behavior as a compile-time or build-flavor option if practical, so pipeline regressions remain cheap when network is not under test.

**Non-Goals:**

- Wireless (Intel PRO/Wireless 2200BG), modem, Bluetooth, or IrDA.
- DNS client beyond whatever DHCP supplies for the probe target (the probe target MAY be a fixed IPv4 address resolved at build time to avoid a DNS stack).
- TCP, UDP sockets API, HTTP, or persistent connections.
- ARP-less static-IP-only mode as the primary path (DHCP is the required bootstrap path).
- Automated bare-metal flashing or on-device test orchestration.
- Updating hardware inventory with inferred NIC controller part numbers without Akoya EX–specific sources.

## Decisions

### 1. Layered module layout under `kernel/net/`

**Choice:** Split responsibilities into:

- `kernel/net/eth/` — NIC driver abstraction and hardware-specific driver(s)
- `kernel/net/link/` — Ethernet framing, MAC address handling
- `kernel/net/ipv4/` — IPv4 send/receive, checksums
- `kernel/net/dhcp/` — DHCP discover/offer/request/ack client
- `kernel/net/icmp/` — echo request/reply for the connectivity probe
- `kernel/net/netmain.c` (or equivalent) — ordered bootstrap sequence invoked from `kernel_main`

**Rationale:** Keeps each layer independently replaceable; later changes can add TCP or swap drivers without rewriting DHCP.

**Alternatives considered:**

| Option | Why not now |
|--------|-------------|
| Monolithic `network.c` | Fast initially but becomes unmaintainable as features accrue |
| External lwIP port | Heavier dependency and build footprint for a single ping |

### 2. Two NIC targets: QEMU emulation first, bare-metal driver second

**Choice:** Implement and test against a QEMU-emulated PCI Ethernet device common in `-device` catalogs (e.g., RTL8139-class or e1000-class). Add a separate bare-metal driver once the Akoya EX NIC controller is confirmed (direct observation or Akoya EX–specific source).

**Rationale:** Inventory documents speed and connector, not controller silicon. Emulation unblocks workstation iteration; bare metal may require a follow-on change or inventory update.

**Alternatives considered:**

| Option | Why not now |
|--------|-------------|
| Guess Realtek 8139 because era-typical | Violates inventory provenance rules; risks wrong driver on hardware |
| Block all work until machine arrives | Delays DHCP/stack validation unnecessarily |

### 3. Connectivity probe target resolved at build time

**Choice:** Embed the probe destination as a build-time constant. Default human-facing target is `google.com`, resolved to IPv4 on the build host during compilation (or pinned to a well-known anycast address documented in README). The guest performs ICMP to that IPv4 without implementing DNS.

**Rationale:** Avoids DNS client scope while honoring the human's "ping google.com" acceptance criterion on the operator-facing side.

### 4. Fixed guest MAC for emulation

**Choice:** Assign a project-scoped locally administered unicast MAC (e.g., `52:54:00:xx:xx:xx` pattern under QEMU's OUI conventions) in the runner and document it. Same MAC every run so home/office DHCP servers reuse one lease.

**Rationale:** Prevents DHCP pool churn during repeated agent/human test loops.

### 5. QEMU bridged networking on the workstation

**Choice:** Attach the emulated NIC with `-netdev bridge,id=...,br=<bridge>` (or equivalent tap+bridge helper script) so DHCP requests reach the same LAN as the workstation. Provide setup notes for creating/using a bridge (may require elevated privileges once).

**Rationale:** User-mode NAT (`-netdev user`) would not exercise real DHCP behavior on the LAN; slirp/user networking hides differences that matter before bare metal.

**Alternatives considered:**

| Option | Why not now |
|--------|-------------|
| `-netdev user` with built-in DHCP | Does not match real LAN DHCP; poor stepping stone |
| macvtap-only without docs | Less portable across distros; bridge + helper script is clearer |

### 6. Extended headless timeout for network bootstrap

**Choice:** Increase default headless timeout when network diagnostics are expected (DHCP + ping may take tens of seconds). Keep separate, shorter timeout for console-only bootstrap flavor if retained.

**Rationale:** Existing 30s ceiling may be tight on slow DHCP servers.

### 7. Console diagnostic format (stable tokens)

**Choice:** Emit parseable, stable line prefixes for automation, e.g.:

- `net_ip=<dotted-quad>`
- `net_ping=<host-or-label> status=ok rtt_ms=<n>` or `status=fail reason=<short>`

**Rationale:** Headless runner can grep predictable tokens without fragile free-text parsing.

## Risks / Trade-offs

| Risk | Mitigation |
|------|------------|
| Unknown bare-metal NIC controller | Ship emulation path first; document blocker for hardware driver; do not invent inventory facts |
| Bridge setup requires root/CAP_NET_ADMIN | Document one-time setup; runner fails fast with actionable message if bridge missing |
| CI or sandbox lacks bridged LAN | Headless network smoke test is workstation-local acceptance; defer CI until environment policy exists |
| DHCP server absent or slow | Bounded DHCP wait with clear console failure; runner timeout accounts for wait |
| ICMP blocked on some LANs | Console reports `status=fail`; smoke test documents that outbound ICMP must be allowed for pass |
| google.com address changes | Build-time resolution; rebuild refreshes target; optional override via build env |

## Migration Plan

Incremental extension — no breaking change to build entry points.

**Apply sequence:**

1. Add `kernel/net/` module skeleton and build wiring.
2. Implement emulated NIC driver + DHCP + ICMP path; wire into `kernel_main`.
3. Extend `scripts/run-qemu.sh` for bridge netdev + fixed MAC + network log assertions.
4. Update README with bridge prerequisites and network smoke-test expectations.
5. Verify `make build && make test` (or dedicated `make test-net`) on a bridged workstation.
6. Document bare-metal Ethernet checklist (cable, DHCP on LAN, expected console lines) without automating deploy.

**Rollback:** Revert apply branch; console-only bootstrap remains available if dual-flavor build is kept.

## Open Questions

- Exact Akoya EX Ethernet controller (PCI ID / MMIO vs port I/O) — resolve when hardware is observed.
- Preferred bridge name on the development workstation (`br0`, `virbr0`, distro default).
- Whether to keep console-only and network images as separate build targets or one always-network image for v1.
