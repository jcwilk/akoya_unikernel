# Tasks: Sequential Chat Transport Lifecycle

## Required for this change

## 1. Remove persistent-transport experiment

- [ ] 1.1 Identify and remove or replace `tcp_session_*` (or equivalent) persistent outbound transport paths introduced on the working branch so no connection survives across inference turns
- [ ] 1.2 Confirm no global or session-scoped transport handle is reused when the operator submits a follow-up message

## 2. Per-turn transport lifecycle in HTTP chat layer

- [ ] 2.1 Refactor `kernel/net/http/http_chat.c` so each non-exit user message runs one turn-scoped exchange: activate transport from inactive state, send chat-completion request with full retained history, extract assistant text or failure, fully release transport before returning
- [ ] 2.2 Ensure failure paths (timeout, connection refused, HTTP error, parse error) complete transport teardown before returning outcome to the session layer
- [ ] 2.3 Verify conversation history buffer is unchanged across turns and still supplies the full `messages` array for each new turn

## 3. TCP teardown completeness

- [ ] 3.1 Update `kernel/net/tcp/tcp.c` (and related link helpers if needed) so turn teardown clears active connection, receive path, and turn-specific state with no leakage to the next turn
- [ ] 3.2 Add or adjust internal checks (debug or test hooks acceptable) that a second turn begins from inactive transport after a successful first turn

## 4. Session loop ordering

- [ ] 4.1 Refactor interactive session orchestration in `kernel/net/netmain.c` (or dedicated session module) to enforce ordering: submit → transport active → exchange complete → transport inactive → print reply/failure → show `> ` prompt
- [ ] 4.2 Confirm assistant reply and failure lines are not printed while outbound transport for the current turn remains active
- [ ] 4.3 Confirm the session does not accept the next operator line until the prior turn's transport is fully released and its outcome is printed

## 5. Build and headless smoke verification

- [ ] 5.1 Run `scripts/build.sh` successfully after transport lifecycle changes
- [ ] 5.2 Run headless smoke test via `scripts/run-qemu.sh` with default scripted input; assert captured output still includes plain assistant reply text, reachability line, and minimal `> ` prompt per existing dev-test-runner contracts (not transport persistence)
- [ ] 5.3 Run headless smoke test with a multi-turn `AKOYA_CHAT_SCRIPT` (or equivalent configured script) and confirm multiple plain reply lines or multi-turn evidence without changing assertion style to key=value framing

## Explicitly deferred

- Bare-metal Akoya EX validation of per-turn teardown on physical hardware (follow-on after QEMU evidence)
- Inventory documentation confirming keyboard controller details (unrelated to transport lifecycle)
