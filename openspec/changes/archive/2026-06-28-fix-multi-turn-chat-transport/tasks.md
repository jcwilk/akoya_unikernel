## 1. Kernel TCP and chat transport fix

- [x] 1.1 Audit `tcp_session_close` and global TCP singleton fields cleared on close; fix any state left set after a completed chat turn (`kernel/net/tcp/tcp.c`, `kernel/net/tcp/tcp.h`)
- [x] 1.2 Add bounded post-close drain/poll after HTTP chat turn until transport inactive before returning to session loop (`kernel/net/http/http_chat.c`)
- [x] 1.3 Verify `http_chat_run_turn` pre- and post-turn inactive guards pass on turn 2+ when inference endpoint is reachable (no spurious `HTTP_CHAT_FAIL_CONNECT`)
- [x] 1.4 Confirm session loop ordering unchanged: reply printed and `> ` prompt only after turn transport fully released (`kernel/net/http/http_chat.c` / session entry)

## 2. Default automated test gate (runner / Makefile)

- [x] 2.1 Wire default headless smoke / `make test` path to multi-turn regression script (`scripts/fixtures/multi-turn-pong.akoya-script` or equivalent contract) instead of single-turn default keyboard script
- [x] 2.2 Ensure default gate asserts plain assistant reply after each of at least two turns and fails on connection-failure lines between successful turns (`scripts/run-qemu.sh`, `scripts/chat-script-harness.sh`, build/test target as applicable)
- [x] 2.3 Document that transport-test entry point remains complementary and does not satisfy the default multi-turn chat gate (README or runner help text)

## 3. Build and verification

- [x] 3.1 Build kernel image successfully (`scripts/build.sh`)
- [x] 3.2 Run inference pre-flight; abort if endpoint unreachable before emulation
- [x] 3.3 Run multi-turn regression script headlessly against main chat image; confirm pass with reachable inference endpoint
- [x] 3.4 Run default automated verification gate (`make test` or documented equivalent); confirm pass with reachable inference endpoint
- [x] 3.5 Confirm transport-test still passes independently (secondary check, not substitute for 3.3–3.4)
- [x] 3.6 Manual headful smoke: complete at least two chat turns without spurious connect failure when inference endpoint is reachable (operator-observed; no automated headful harness)

## Explicitly deferred

- Headful automated assertion harness — headful verification remains operator-observed for this change
- Bare-metal Akoya EX automated verification — QEMU workstation path only
