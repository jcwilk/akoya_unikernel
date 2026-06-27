## Why

The bootstrap image currently sends one fixed chat message and halts. Operators need a conversational loop: type at the console, see the model reply, ask follow-ups, and have each inference request include the full conversation so far—matching how OpenAI-compatible chat APIs expect multi-turn context.

## What Changes

- Replace the single-shot chat probe with an interactive chat session after network diagnostics complete.
- Accept keyboard input from the Medion Akoya EX integrated keyboard—the standard built-in PS/2 keyboard controller path for this notebook class—not from serial console as the operator-facing input device on bare metal.
- Map key events to US keyboard layout semantics (printable ASCII, Enter to submit, Backspace to edit the current line).
- Maintain in-memory conversation history (user and assistant turns) and send the accumulated `messages` array on every chat-completion request.
- Print each assistant reply on the console before prompting for the next user line.
- Allow the operator to end the session explicitly (for example a quit command or equivalent) and then halt cleanly.
- Extend headless smoke testing to inject scripted keystrokes into the same emulated keyboard path the bare-metal target uses—not serial console as a stand-in for the built-in keyboard.
- Keep TLS, DNS, streaming responses, and persistent storage out of scope.

## Capabilities

### New Capabilities

- `interactive-chat-session`: Integrated keyboard input (US layout via deployment-target keyboard controller), line editing, multi-turn chat loop, and explicit session exit before halt.
- `deployment-target`: Interactive chat on bare metal uses the notebook integrated keyboard without requiring external USB or serial input devices.

### Modified Capabilities

- `http-chat-probe`: From one fixed diagnostic message to multi-turn requests that include full conversation history in each payload; per-turn success/failure reporting; session ends only when the interactive loop exits—not after the first exchange.
- `bootstrap-unikernel`: Application scope expands from a single chat exchange to an interactive session bounded by explicit user exit.
- `dev-test-runner`: Headless mode injects scripted keystrokes into the emulated deployment-target keyboard path; headful SDL uses the same path; timeouts account for interactive or scripted chat.
- `deployment-target`: Bare-metal interactive chat uses integrated notebook keyboard; no external input device required.
- `network-unikernel`: Orderly network completion occurs after the interactive chat session ends, not after the first HTTP exchange.

## Impact

- New PS/2-class keyboard input module (i8042 controller) aligned with Pentium M / ICH-era notebook integrated keyboard.
- HTTP chat client grows conversation buffer and multi-turn JSON assembly.
- `net_bootstrap()` hands off to interactive session instead of one-shot probe.
- QEMU runner injects scripted keystrokes into emulated PS/2 keyboard for headless automation; headful SDL uses the same guest keyboard path as bare metal.
- README and smoke-test expectations updated.
- Uncommitted port/model fixes on the working branch are separate implementation debt; this change defines the interactive behavior contract.
