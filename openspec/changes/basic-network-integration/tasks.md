## 1. Network module skeleton

- [ ] 1.1 Add `kernel/net/` directory layout (eth, link, ipv4, dhcp, icmp, orchestration) with headers defining narrow layer interfaces per `design.md`
- [ ] 1.2 Wire network objects into `Makefile` / `scripts/build.sh` with build-time probe target resolution (default: resolve `google.com` to IPv4 on the build host)
- [ ] 1.3 Emit stable console tokens (`net_ip=`, `net_ping=`) documented in README for runner parsing

## 2. Emulated NIC and stack

- [ ] 2.1 Implement QEMU-target Ethernet driver for the chosen emulated device (RTL8139-class or e1000-class per design)
- [ ] 2.2 Implement Ethernet framing and MAC handling in the link layer
- [ ] 2.3 Implement minimal IPv4 send/receive with checksum validation
- [ ] 2.4 Implement DHCP client (discover → offer → request → ack) with bounded timeout and failure reporting
- [ ] 2.5 Implement single-shot ICMP echo request/reply with latency measurement and failure categories

## 3. Bootstrap integration

- [ ] 3.1 Extend `kernel/main.c` (or network orchestration entry) to run: initial bootstrap message → link bring-up → DHCP → print assigned address → connectivity probe → orderly halt
- [ ] 3.2 Preserve console-only bootstrap as a build flavor or flag so existing smoke path remains available without bridged networking

## 4. QEMU runner and smoke test

- [ ] 4.1 Extend `scripts/run-qemu.sh` to attach bridged `-netdev` for network bootstrap images with project-fixed guest MAC
- [ ] 4.2 Fail fast when bridge/prerequisites are missing, with actionable error text
- [ ] 4.3 Extend headless assertions to require bootstrap message, `net_ip=` line, and successful `net_ping=` line when network mode is active
- [ ] 4.4 Increase default headless timeout for network bootstrap runs; keep shorter timeout for console-only flavor
- [ ] 4.5 Add Makefile target (e.g. `test-net`) or document when `make test` exercises network vs console-only

## 5. Documentation and workstation acceptance

- [ ] 5.1 Update `README.md` with bridge setup prerequisites, fixed MAC note, and expected console output for network smoke test
- [ ] 5.2 Document bare-metal Ethernet checklist (RJ-45, LAN DHCP, ICMP allowance) without automating deploy
- [ ] 5.3 Run `make build` and network headless smoke test on development workstation with bridged LAN; capture evidence (log path, pass/fail) for apply handoff

## Explicitly deferred

- Bare-metal NIC driver for the Akoya EX controller (blocked until controller identity is confirmed on hardware or via Akoya EX–specific inventory source)
- Wireless, DNS client, TCP/UDP, HTTP, and persistent socket APIs
- CI/automation for bridged network tests in sandboxed environments
- Hardware inventory updates inferring NIC controller part numbers without Akoya EX–specific corroboration
