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

- [ ] 3.1 Replace console-only bootstrap flow in `kernel/main.c` with always-network sequence: initial bootstrap message → link bring-up → DHCP → print assigned address → connectivity probe → orderly halt
- [ ] 3.2 Remove console-only bootstrap build flavor, flags, and dead code paths

## 4. QEMU runner and smoke test

- [ ] 4.1 Require LAN-attached tap/macvtap netdev on every `scripts/run-qemu.sh` invocation (headless and headful) with project-fixed guest MAC; remove user-mode/NAT/slirp and bridge-enslavement run paths
- [ ] 4.2 Add ephemeral macvtap setup/teardown helpers (libexec + scoped sudoers) on the designated wired interface only; never modify the wireless interface
- [ ] 4.3 Fail fast when macvtap prerequisites or permissions are missing, with actionable error text
- [ ] 4.4 Extend headless assertions to require bootstrap message, `net_ip=` line, and successful `net_ping=` line
- [ ] 4.5 Increase default headless timeout for DHCP + connectivity probe
- [ ] 4.6 Ensure `make test` always exercises the LAN-attached always-network bootstrap (no separate `test-net` target unless documented as alias only)

## 5. Documentation and workstation acceptance

- [ ] 5.1 Update `README.md` with macvtap setup, designated wired interface, fixed MAC note, and expected console output for smoke test on all run modes
- [ ] 5.2 Document bare-metal Ethernet checklist (RJ-45, LAN DHCP, ICMP allowance) without automating deploy
- [ ] 5.3 Run `make build && make test` on development workstation; verify host network access remains usable before, during, and after the test; capture evidence (log path, pass/fail) for apply handoff

## Explicitly deferred

- Bare-metal NIC driver for the Akoya EX controller (blocked until controller identity is confirmed on hardware or via Akoya EX–specific inventory source)
- Wireless, DNS client, TCP/UDP, HTTP, and persistent socket APIs
- CI/automation for LAN-attached network tests in sandboxed environments
- Hardware inventory updates inferring NIC controller part numbers without Akoya EX–specific corroboration
- Linux bridge enslavement of host interfaces (rejected after prior apply disrupted workstation connectivity)
