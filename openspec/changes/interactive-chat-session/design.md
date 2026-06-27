## Context

The kernel boots, runs network diagnostics (link, DHCP, ICMP ping), then performs one HTTP chat-completion with a build-configured message and halts. Console output is write-only today (VGA + serial transmit). QEMU headful and headless runs attach serial for output; headless smoke tests grep `chat_completion=ok`.

The human wants a REPL-style chat loop on US keyboard layout, with conversation history included in every inference request to the existing llama.cpp OpenAI-compatible endpoint (`192.168.1.110:11435` by default on the working branch).

## Goals / Non-Goals

**Goals:**

- After successful network diagnostics, enter an interactive chat session on the console.
- Read keyboard input with US QWERTY semantics: printable ASCII from key presses, Backspace edits the current line, Enter submits the line.
- Accumulate user and assistant messages in memory; each chat-completion HTTP request sends the full `messages` array built so far.
- Print assistant replies on the console before the next input prompt.
- Explicit session exit (e.g. user types `quit` or `exit` on an empty or dedicated command) then orderly halt.
- Headless smoke test: inject scripted lines via QEMU serial stdin (e.g. `hi` then `quit`) and assert at least one successful completion in captured output.

**Non-Goals:**

- Non-US keyboard layouts, modifier keys beyond what is needed for basic typing, or mouse input.
- Unicode beyond 7-bit printable ASCII for v1.
- TLS/HTTPS, DNS, streaming (`stream: true`), tool calls, or image attachments.
- Persisting conversation across reboots or to disk.
- Bare-metal keyboard controller bring-up beyond what QEMU serial or a minimal PS/2 path provides for the Akoya target (implementation chooses serial-first for workstation iteration).

## Decisions

### 1. Serial console as primary input path for v1

**Choice:** Read user input from the same serial console used for output (QEMU `-serial stdio` bidirectional). Map incoming bytes as US keyboard input when they arrive as typed characters; for headful SDL, QEMU typically forwards keyboard to serial when configured.

**Rationale:** Avoids PS/2 controller work for the first interactive increment; matches existing dev-test runner serial attachment.

**Alternatives:**

| Option | Why not now |
|--------|-------------|
| PS/2 driver only | More hardware-specific; serial unblocks QEMU and scripted headless |
| VGA text-mode keyboard scan codes | Duplicates serial path in emulation |

### 2. In-memory conversation buffer with bounded turns

**Choice:** Fixed-capacity store of message pairs (e.g. max 16 turns or ~8 KB JSON budget). Oldest turns dropped or session refuses new input when full—documented in README.

**Rationale:** No heap; predictable memory on i686 unikernel.

### 3. HTTP client reuse per turn

**Choice:** Each submitted line triggers one new TCP connection and POST (same as current single-shot client). Request body lists all messages in history order.

**Rationale:** Matches existing minimal TCP client; no keep-alive complexity.

### 4. Per-turn console tokens

**Choice:** Keep `chat_completion=ok reply=...` / `chat_completion=fail reason=...` per exchange; add a visible prompt line (e.g. `chat>`) before each user input.

**Rationale:** Preserves smoke-test grep patterns; operators see clear turn boundaries.

### 5. Session exit command

**Choice:** Lines `quit` or `exit` (case-sensitive or case-insensitive—pick insensitive in implementation) end the session without sending inference. Empty lines ignored or re-prompt.

### 6. Headless smoke test scripting

**Choice:** `run-qemu.sh` headless passes stdin to QEMU serial when `AKOYA_CHAT_SCRIPT` unset uses default `hi\nquit\n`. Assert first `chat_completion=ok` in log; do not require multi-turn in automated smoke unless script provides multiple lines.

**Rationale:** Keeps CI deterministic; human multi-turn remains headful or custom script.

### 7. Bootstrap diagnostic path unchanged

**Choice:** Initial bootstrap message, DHCP, and ping remain before any interactive session—same ordering as today.

## Risks / Trade-offs

- **[Risk] Headless hang without scripted input** → Default chat script in runner; bounded session timeout optional fallback in design only if needed.
- **[Risk] JSON size exceeds buffer** → Cap history; report `chat_completion=fail reason=overflow` and skip send.
- **[Risk] SDL headful without serial keyboard** → Document QEMU flags; verify `-serial stdio` or chardev multiplexing in tasks.
- **[Trade-off] New TCP connection per turn** → Simpler but slower; acceptable for interactive use.

## Migration Plan

1. Add input module and session loop.
2. Extend HTTP chat to build multi-message JSON from history.
3. Replace one-shot `http_chat_probe()` call with `chat_session_run()`.
4. Update runner for bidirectional serial + default script.
5. Update README; run `make test`.

Rollback: restore single-shot probe and write-only console.

## Open Questions

- Whether Akoya bare metal will use serial-only input initially (assumed yes for v1).
- Exact max history size—proposal leaves to implementation with documented cap.
