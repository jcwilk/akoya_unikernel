## Context

The repository ships a minimal i686 multiboot bootstrap kernel with serial/VGA console output and a QEMU smoke-test runner. Hardware inventory confirms the Medion Akoya EX has 10/100 Mbit Ethernet (RJ-45) but does not yet identify the exact NIC controller chip. The bootstrap image currently halts after printing a fixed diagnostic string; no network code exists.

The human wants the next increment to exercise real DHCP and TCP/IP on both emulated and bare-metal paths, with modular structure for later protocols and drivers. The bootstrap image SHALL always be network-capable, and every development-workstation run path SHALL attach the guest to the workstation LAN so traffic reaches the same router and DHCP server as the host.

A prior apply attempt used Linux bridge enslavement of the workstation USB Ethernet interface. That approach broke host connectivity when the bridge profile had IPv4 disabled. The human requires a mechanism that gives the guest real LAN access without disrupting the workstation's own network access.

## Goals / Non-Goals

**Goals:**

- Single always-network bootstrap image: bring up wired Ethernet, complete DHCP for IPv4, print the leased address, and perform one ICMP echo (ping) to a configured public host, reporting reachability and round-trip time on the console.
- Organize kernel networking as layered modules with narrow interfaces (NIC driver ↔ link layer ↔ IPv4 ↔ DHCP ↔ ICMP ↔ bootstrap orchestration).
- Require LAN-attached emulation on every QEMU invocation—headless and headful—so the guest NIC is on the workstation LAN with a fixed guest MAC address.
- Preserve host network connectivity during ephemeral test setup, the test run, and teardown.
- Extend smoke tests so all run paths assert network diagnostic lines.
- Retain the initial bootstrap console message before any network activity begins.

**Non-Goals:**

- Console-only bootstrap build flavor or optional network disable switch.
- User-mode NAT, slirp, or other run paths that do not attach the guest to the workstation LAN.
- Linux bridge enslavement of the host's primary or backup network interfaces (rejected after failed apply).
- Attaching the guest passthrough to the workstation wireless interface.
- Wireless (Intel PRO/Wireless 2200BG), modem, Bluetooth, or IrDA in the guest stack.
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

### 5. Macvtap LAN passthrough on a designated wired interface

**Choice:** Every QEMU invocation—headless smoke test and headful interactive session—creates an ephemeral macvtap device in bridge mode on a designated workstation wired LAN interface (default: USB Ethernet `enx00e04c6801e8`). The guest receives its own MAC on the LAN segment and obtains DHCP from the same router as the workstation. The physical parent interface keeps its existing host IP and routing; no bridge enslavement or NetworkManager profile migration.

Setup and teardown wrap each QEMU run:

1. Create macvtap on the designated wired interface.
2. Pass the macvtap file descriptor to QEMU as a tap netdev with the fixed guest MAC.
3. Delete the macvtap device after QEMU exits (including on failure paths).

The wireless interface (`wlp82s0`) MUST NOT be used as the macvtap parent and MUST NOT be modified by setup scripts.

**Rationale:** Macvtap satisfies the behavioral requirement (real LAN, real DHCP, same router) without moving the host's IP onto a bridge or enslaving the physical NIC. This directly addresses the failed apply where bridge enslavement broke workstation connectivity.

**Alternatives considered:**

| Option | Why not now |
|--------|-------------|
| `-netdev user` with built-in DHCP | Does not reach the workstation router; rejected by intent |
| Linux bridge + TAP enslavement | Disrupted host connectivity during prior apply; high NM misconfiguration risk |
| Bridge only for headless / NAT for headful | Violates always-LAN-attached requirement |
| macvtap on WiFi parent | Wireless must stay untouched; USB wired is the lab port |

**Known limitation:** In macvtap bridge mode, the host and guest cannot exchange traffic directly on the same parent interface. This is acceptable because bootstrap diagnostics only require guest → router → internet reachability, not guest → host.

### 6. Scoped elevated permissions for macvtap lifecycle

**Choice:** Install root-only setup/teardown helpers under a fixed libexec path with a matching sudoers drop-in granting passwordless execution only for those scripts. Scripts accept an environment variable for the repository root so state files land in the project build directory.

**Rationale:** Creating and deleting macvtap devices requires `CAP_NET_ADMIN`. Scoped sudo avoids interactive password prompts during automated `make test` while keeping the privilege surface smaller than unrestricted sudo or permanent bridge profiles.

### 7. Extended headless timeout for network bootstrap

**Choice:** Increase default headless timeout to account for DHCP negotiation and the connectivity probe (tens of seconds). Headful mode retains manual session control without an automated pass/fail timeout gate.

**Rationale:** Existing 30s ceiling may be tight on slow DHCP servers; network diagnostics are now always on the critical path.

### 8. Console diagnostic format (stable tokens)

**Choice:** Emit parseable, stable line prefixes for automation, e.g.:

- `net_ip=<dotted-quad>`
- `net_ping=<host-or-label> status=ok rtt_ms=<n>` or `status=fail reason=<short>`

**Rationale:** Headless runner can grep predictable tokens without fragile free-text parsing.

### 9. Single always-network bootstrap image

**Choice:** Remove the console-only bootstrap path. Every build of the bootstrap image includes the network stack and runs the full diagnostic sequence after the initial console message.

**Rationale:** Matches human intent; avoids dual maintenance and ambiguous `make test` behavior.

**Alternatives considered:**

| Option | Why not now |
|--------|-------------|
| Dual build flavors (console vs network) | Adds complexity; human chose always-network |

## Risks / Trade-offs

| Risk | Mitigation |
|------|------------|
| Unknown bare-metal NIC controller | Ship emulation path first; document blocker for hardware driver; do not invent inventory facts |
| Macvtap setup requires root/CAP_NET_ADMIN | Scoped sudoers for libexec helpers only; runner fails fast with actionable message if permissions missing |
| Wrong parent interface selected | Document default wired interface; allow override via environment variable; scripts refuse wireless parent |
| Host–guest cannot ping each other on macvtap | Acceptable for bootstrap scope; document limitation |
| CI or sandbox lacks LAN-attached test | Smoke test is workstation-local acceptance; defer CI until environment policy exists |
| DHCP server absent or slow | Bounded DHCP wait with clear console failure; runner timeout accounts for wait |
| ICMP blocked on some LANs | Console reports `status=fail`; smoke test documents that outbound ICMP must be allowed for pass |
| google.com address changes | Build-time resolution; rebuild refreshes target; optional override via build env |

## Migration Plan

Breaking behavioral change for bootstrap and test runner: console-only smoke path is replaced by always-network + always-LAN-attached emulation.

**Apply sequence:**

1. Add `kernel/net/` module skeleton and build wiring.
2. Implement emulated NIC driver + DHCP + ICMP path; wire into `kernel_main` as the sole bootstrap path.
3. Replace `scripts/run-qemu.sh` networking so every invocation uses macvtap netdev + fixed MAC + network log assertions.
4. Add ephemeral macvtap setup/teardown helpers and scoped sudoers documentation; remove any bridge-based runner paths.
5. Remove console-only bootstrap code and any runner paths that omit LAN attachment.
6. Update README with macvtap prerequisites, designated wired interface, and expected console output.
7. Verify `make build && make test` on the development workstation; confirm host connectivity remains usable before, during, and after the test.
8. Document bare-metal Ethernet checklist (cable, DHCP on LAN, expected console lines) without automating deploy.

**Rollback:** Revert apply branch; restores prior console-only bootstrap and non-networked runner.

## Open Questions

- Exact Akoya EX Ethernet controller (PCI ID / MMIO vs port I/O) — resolve when hardware is observed.
