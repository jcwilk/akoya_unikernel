## Why

The bootstrap image currently sends one fixed chat message and halts. Operators need a conversational loop: type at the console, see the model reply, ask follow-ups, and have each inference request include the full conversation so far—matching how OpenAI-compatible chat APIs expect multi-turn context.

## What Changes

- Replace the single-shot chat probe with an interactive chat session after network diagnostics complete.
- Accept keyboard input on the console using a US keyboard layout (printable ASCII, Enter to submit, Backspace to edit the current line).
- Maintain in-memory conversation history (user and assistant turns) and send the accumulated `messages` array on every chat-completion request.
- Print each assistant reply on the console before prompting for the next user line.
- Allow the operator to end the session explicitly (for example a quit command or equivalent) and then halt cleanly.
- Extend headless smoke testing to drive scripted serial input for at least one automated exchange without requiring a physical keyboard.
- Keep TLS, DNS, streaming responses, and persistent storage out of scope.

## Capabilities

### New Capabilities

- `interactive-chat-session`: Console keyboard input (US layout), line editing, multi-turn chat loop, and explicit session exit before halt.

### Modified Capabilities

- `http-chat-probe`: From one fixed diagnostic message to multi-turn requests that include full conversation history in each payload; per-turn success/failure reporting; session ends only when the interactive loop exits—not after the first exchange.
- `bootstrap-unikernel`: Application scope expands from a single chat exchange to an interactive session bounded by explicit user exit.
- `dev-test-runner`: Headless mode supplies scripted console input for automated smoke validation; timeouts account for interactive or scripted multi-turn chat; headful mode is the primary human keyboard path.
- `network-unikernel`: Orderly network completion occurs after the interactive chat session ends, not after the first HTTP exchange.

## Impact

- New keyboard/input module and chat session orchestration in the kernel.
- HTTP chat client grows conversation buffer and multi-turn JSON assembly.
- `net_bootstrap()` hands off to interactive session instead of one-shot probe.
- QEMU runner may attach bidirectional serial for headless scripted input.
- README and smoke-test expectations updated.
- Uncommitted port/model fixes on the working branch are separate implementation debt; this change defines the interactive behavior contract.
