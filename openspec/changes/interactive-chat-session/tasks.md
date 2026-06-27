# Tasks

## Required for this change

## 1. Console keyboard input

- [ ] 1.1 Add serial (or equivalent) receive path for console input with US-layout printable ASCII, Backspace, and Enter handling
- [ ] 1.2 Echo edited input line to console; ignore empty submissions
- [ ] 1.3 Recognize exit commands (`quit` / `exit`, case-insensitive) without sending inference

## 2. Conversation history

- [ ] 2.1 Add bounded in-memory store for user and assistant messages (documented max turns/size)
- [ ] 2.2 Append each submitted user line and successful assistant reply to history in order
- [ ] 2.3 Refuse or fail gracefully when history would exceed buffer budget

## 3. Multi-turn HTTP chat client

- [ ] 3.1 Extend HTTP chat module to build OpenAI `messages` JSON from full session history
- [ ] 3.2 Retain per-turn `chat_completion=ok|fail` console tokens and assistant reply printing
- [ ] 3.3 Invoke one HTTP exchange per submitted user line (non-exit)

## 4. Interactive session orchestration

- [ ] 4.1 Replace one-shot `http_chat_probe()` with session loop: prompt → read line → infer → print → repeat until exit
- [ ] 4.2 Run session only after successful network diagnostics; skip when DHCP/ping failed
- [ ] 4.3 Halt cleanly after session exit (preserve QEMU debug exit path)

## 5. Dev test runner

- [ ] 5.1 Attach bidirectional serial in headless mode; default script `hi` + exit command
- [ ] 5.2 Support `AKOYA_CHAT_SCRIPT` or equivalent for custom scripted input
- [ ] 5.3 Adjust headless timeout for scripted chat; keep chat endpoint pre-flight assertion
- [ ] 5.4 Headful: ensure keyboard input reaches guest serial/console path

## 6. Documentation

- [ ] 6.1 Update README: interactive chat usage, US keyboard, exit commands, history behavior, scripted headless input

## 7. Build and acceptance

- [ ] 7.1 `make build` succeeds
- [ ] 7.2 `make test` passes with scripted headless input and reachable chat endpoint

## Explicitly deferred

- PS/2 keyboard driver for bare-metal without serial console
- Non-US keyboard layouts and Unicode beyond 7-bit ASCII
- TLS, streaming, DNS, persistent conversation storage
- Multi-turn requirements in automated smoke beyond default script (custom script is optional enhancement)
