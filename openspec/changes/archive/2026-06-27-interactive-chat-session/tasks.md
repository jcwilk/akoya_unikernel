# Tasks

## Required for this change

## 1. PS/2 integrated keyboard input (Akoya-class path)

- [x] 1.1 Add i8042 PS/2 keyboard controller driver—the standard integrated keyboard path for Pentium M / ICH-era notebooks including Akoya EX
- [x] 1.2 Decode scan codes to US-layout printable ASCII, Backspace, and Enter for line editing
- [x] 1.3 Echo edited input line to console; ignore empty submissions
- [x] 1.4 Recognize exit commands (`quit` / `exit`, case-insensitive) without sending inference

## 2. Conversation history

- [x] 2.1 Add bounded in-memory store for user and assistant messages (documented max turns/size)
- [x] 2.2 Append each submitted user line and successful assistant reply to history in order
- [x] 2.3 Refuse or fail gracefully when history would exceed buffer budget

## 3. Multi-turn HTTP chat client

- [x] 3.1 Extend HTTP chat module to build OpenAI `messages` JSON from full session history
- [x] 3.2 Retain per-turn `chat_completion=ok|fail` console tokens and assistant reply printing
- [x] 3.3 Invoke one HTTP exchange per submitted user line (non-exit)

## 4. Interactive session orchestration

- [x] 4.1 Replace one-shot `http_chat_probe()` with session loop: prompt → read line from keyboard driver → infer → print → repeat until exit
- [x] 4.2 Run session only after successful network diagnostics; skip when DHCP/ping failed
- [x] 4.3 Halt cleanly after session exit (preserve QEMU debug exit path)

## 5. Dev test runner

- [x] 5.1 Headless: inject default scripted keystrokes into emulated PS/2 keyboard (e.g. QEMU monitor `sendkey`), not serial stdin as fake keyboard
- [x] 5.2 Support `AKOYA_CHAT_SCRIPT` or equivalent for custom keyboard injection scripts
- [x] 5.3 Adjust headless timeout for scripted chat; keep chat endpoint pre-flight assertion
- [x] 5.4 Headful: verify SDL key events reach guest i8042 path (same driver as bare metal)

## 6. Documentation

- [x] 6.1 Update README: integrated keyboard input (PS/2 path), US layout, exit commands, history behavior, headless keyboard injection

## 7. Build and acceptance

- [x] 7.1 `make build` succeeds
- [x] 7.2 `make test` passes with scripted keyboard input and reachable chat endpoint

## Explicitly deferred

- USB HID external keyboard support
- Non-US keyboard layouts and Unicode beyond 7-bit ASCII
- Touchpad/mouse as chat input
- TLS, streaming, DNS, persistent conversation storage
- Hardware inventory update confirming i8042 on Akoya (optional follow-on when machine observed)
- Multi-turn requirements in automated smoke beyond default script (custom script is optional enhancement)
