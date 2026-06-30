## Required for this change

### 1. Fail-closed chat transport lifecycle (kernel)

- [x] 1.1 Add chat-specific fail-closed drain helper that polls until outbound transport is fully inactive and returns failure without force-clearing state when the bounded budget expires (`kernel/net/http/http_chat.c`, `kernel/net/tcp/tcp.c`)
- [x] 1.2 Remove connect retry masking in `http_chat_run_turn` — single connect attempt per turn after inactive verification
- [x] 1.3 Add transport-lifecycle failure status and map to plain console category in session loop (`kernel/net/http/http_chat.c`, `kernel/net/netmain.c` or `http_chat_session`)
- [x] 1.4 Ensure failure paths (timeout, refused, HTTP error, parse error, transport-lifecycle) all require fully inactive transport before printing outcome or returning to input prompt

### 2. Timed-gap chat regression boot image

- [x] 2.1 Add timed-gap chat regression entry module with fixed schedule (≥3 turns, bounded inter-turn delay at input prompt) reusing production chat turn path (`kernel/net/chat_regression_main.c` or equivalent)
- [x] 2.2 Add Makefile/build target for `chat-regression-test` logical boot identity distinct from main chat unikernel and transport-test
- [x] 2.3 Implement per-turn pass/fail and aggregate summary console reporting per delta spec

### 3. Development test runner wiring

- [x] 3.1 Add documented timed-gap chat regression verification entry point in `scripts/` (build/select regression image, headless LAN run, capture output, non-zero on aggregate fail)
- [x] 3.2 Point default automated verification gate (`make test` or equivalent) at timed-gap chat regression (≥3 turns with inter-turn delay)
- [x] 3.3 Ensure runner auto-selects timed-gap regression logical identity for that entry point; transport-test and main chat unikernel remain separately selectable
- [x] 3.4 Update README with timed-gap regression entry point and note that transport-test alone does not satisfy chat health gate

### 4. Verification evidence (apply-complete)

- [x] 4.1 Run timed-gap chat regression verification entry point headlessly with reachable inference endpoint; capture aggregate pass with ≥3 consecutive turns and no connection-failure or transport-lifecycle lines between successful turns
- [x] 4.2 Run transport-test verification independently; confirm it still passes when transport stack is healthy
- [x] 4.3 Headful manual check: multi-turn interactive chat on main image succeeds for turn 2+ when inference endpoint is reachable (operator-observed evidence in apply handoff)

## Explicitly deferred

- Headful automated assertion harness for main chat unikernel (headful remains operator-observed)
- Bare-metal Akoya EX automated verification
- Removing force-release from transport-test drain path (chat path only in this change; global TCP helper alignment optional follow-on)
- Changing plain-reply formatting, exit command semantics, token cap, or keyboard input path
