## Required for this change

### Third-party stack integration

- [ ] 1.1 Vendor lwIP (version-pinned) with BSD license recorded in project notices
- [ ] 1.2 Add lean bare-metal configuration and port layer mapping guest millisecond time to stack timers
- [ ] 1.3 Implement netif adapter on existing wired device poll/send boundary (keep RTL8139 driver)
- [ ] 1.4 Service stack receive and timer work from each guest event runtime pump pass, including during interactive input wait

### Application and bootstrap rewiring

- [ ] 2.1 Move network bootstrap (DHCP, address report, ICMP reachability probe) onto the maintained stack
- [ ] 2.2 Rewire interactive HTTP chat turns to use maintained-stack TCP for connect, send, recv, and close
- [ ] 2.3 Port transport-test scenarios to the same maintained stack; preserve aggregate pass/fail reporting
- [ ] 2.4 Remove superseded in-house IPv4/TCP/ARP/DHCP modules and dead mitigation paths once parity is verified

### Verification consolidation

- [ ] 3.1 Retire timed-gap chat regression boot image, dedicated runner, and duplicate idle fixtures
- [ ] 3.2 Add single idle-at-prompt scripted gate (short message, 20s host-timed idle, short second message, exit) on main kernel
- [ ] 3.3 Point `make test` at idle-at-prompt gate plus transport-test; update README verification table

### Apply-complete evidence

- [ ] 4.1 `make build` succeeds (kernel + transport-test only; no regression boot identity)
- [ ] 4.2 `make test` passes with reachable inference endpoint
- [ ] 4.3 Headful smoke: first message, wait at prompt ≥20s, second message — no spurious connection-failure line

## Explicitly deferred

- N-consecutive automated runs as default CI requirement
- Replacing RTL8139 driver with a different OSS driver
- IPv6, TLS, UDP services, full sockets API
- USB/ISO verification tier changes
- Salvaging or merging branch `explore/simplify-chat-idle-and-verification-idle-at-prompt` except harness/fixture ideas useful to task 3.2
