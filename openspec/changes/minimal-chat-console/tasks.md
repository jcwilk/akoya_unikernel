# Tasks: Minimal Chat Console

## Required for this change

## 1. Console clear primitive

- [ ] 1.1 Add `console_clear()` to `kernel/console/console.c` and declare it in `kernel/console/console.h`
- [ ] 1.2 Verify clear removes visible prior lines on VGA text console under QEMU serial capture

## 2. Network probe target and reachability output

- [ ] 2.1 Change `kernel/net/netmain.c` to probe the chat/inference host using `AKOYA_CHAT_HOST_IP0–3` octets instead of `AKOYA_PROBE_TARGET_*` (8.8.8.8)
- [ ] 2.2 Call `console_clear()` immediately before `icmp_ping` in `net_bootstrap`
- [ ] 2.3 Replace `net_ping=… status=ok rtt_ms=…` success output with `<host-label> reachable` (e.g. `192.168.1.110 reachable`)
- [ ] 2.4 Replace probe failure key=value lines with short readable lines (`<host-label> unreachable` / `<host-label> timeout`)

## 3. Minimal chat session output

- [ ] 3.1 Change `console_write_prompt("chat>")` to `console_write_prompt("> ")` in `kernel/net/http/http_chat.c`
- [ ] 3.2 Replace `emit_ok` / `emit_fail` in `http_chat.c` so successful turns print plain assistant reply text on its own line(s) without `chat_completion=` framing
- [ ] 3.3 Format chat failures as brief readable lines (e.g. `chat failed: timeout`) without key=value prefixes
- [ ] 3.4 Confirm multi-turn history, US keyboard layout, shift, and explicit exit behavior remain unchanged

## 4. Headless smoke test assertions

- [ ] 4.1 Update `scripts/run-qemu.sh` headless grep checks: require `${CHAT_HOST} reachable` instead of `net_ping=… status=ok rtt_ms=…`
- [ ] 4.2 Update chat success assertions to detect plain assistant reply text from multi-turn scripted input (default script produces at least two turns) without matching `chat_completion=ok reply=`
- [ ] 4.3 Reject captured output that still uses old `chat_completion=` diagnostic framing as pass evidence

## 5. Build and verification

- [ ] 5.1 Run `./scripts/build.sh` and confirm clean build
- [ ] 5.2 Run `./scripts/run-qemu.sh --headless` on a workstation with LAN DHCP and reachable chat endpoint at `${CHAT_HOST}:${CHAT_PORT}`; capture evidence in apply handoff

## Explicitly deferred

- Bare-metal Akoya EX acceptance on physical hardware (same console changes expected to work; no separate task row required unless operator requests hardware proof).
