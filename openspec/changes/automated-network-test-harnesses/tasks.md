# Tasks: Automated Network Test Harnesses

## Required for this change

## 1. Transport-test boot image (Deliverable A)

- [ ] 1.1 Add `kernel/net/transport_test_main.c` (or equivalent) with non-interactive scenario scheduler, timed delays, and console pass/fail reporting per design
- [ ] 1.2 Link transport-test image against shared production network objects (`link`, IPv4, DHCP, `tcp`) — same sources as main kernel, no mock stack
- [ ] 1.3 Extend `scripts/build.sh` to emit `build/transport-test.bin` as a distinct logical identity from `build/kernel.bin`
- [ ] 1.4 Implement scenarios: sequential same-host connects, delayed reconnect, and synthetic refused/timeout target (closed port—not inference host offline); report FAIL if configured inference host is unreachable during guest-side scenarios that require it
- [ ] 1.5 Print per-scenario PASS/FAIL lines and final aggregate `transport-test: ALL PASS` or `transport-test: FAILED` before halt/idle

## 2. Transport-test runner entry point

- [ ] 2.1 Add `scripts/run-transport-test.sh` wrapper: build if needed, invoke headless LAN emulation for transport-test image, capture serial output
- [ ] 2.2 Exit non-zero when aggregate failure line appears; exit zero on aggregate pass
- [ ] 2.3 Ensure `--image` / logical identity selection works when both `kernel.bin` and `transport-test.bin` exist (no ambiguous auto-select)
- [ ] 2.4 Pre-flight: verify configured inference endpoint is reachable from the workstation; **abort with non-zero exit and actionable error before starting emulation** if unreachable (no skip, no partial run)

## 3. Scripted full-app interaction harness (Deliverable B)

- [ ] 3.1 Define line-oriented script format (`delay`, `type`, `expect`, comments) per design; add parser module under `scripts/` (e.g. `scripts/chat-script-harness.sh` or lib)
- [ ] 3.2 Extend `scripts/run-qemu.sh` with `--script FILE` headless mode: inject keystrokes via existing QEMU monitor sendkey path between steps
- [ ] 3.3 Implement assertion engine on captured serial log: substring/regex match, fail fast with step index and expected vs actual snippet
- [ ] 3.4 Preserve backward compatibility for default `AKOYA_CHAT_SCRIPT` string smoke without `--script`
- [ ] 3.5 Add sample fixture `scripts/fixtures/multi-turn-pong.akoya-script` covering reachability assert, PONG reply, second turn, and no `chat failed: connect` between turns
- [ ] 3.6 Pre-flight for `--script` mode: abort before emulation when configured inference endpoint is unreachable; align error messaging with transport-test and default smoke entry points

## 4. Build and verification

- [ ] 4.1 Run `scripts/build.sh` successfully; confirm both logical images are produced
- [ ] 4.2 Run `scripts/run-transport-test.sh` headlessly on workstation LAN; confirm aggregate PASS in captured output and exit code zero when scenarios succeed
- [ ] 4.3 Run `scripts/run-qemu.sh --headless --script scripts/fixtures/multi-turn-pong.akoya-script`; confirm pass with per-turn reply assertions and non-zero exit when an expected pattern is removed (negative test)
- [ ] 4.4 Document agent entry points and script format in README

## Explicitly deferred

- Bare-metal Akoya EX transport-test or scripted chat harness
- Headful display-mode script injection (implement only if QEMU sendkey works headful during apply; otherwise document headless-only in README)
