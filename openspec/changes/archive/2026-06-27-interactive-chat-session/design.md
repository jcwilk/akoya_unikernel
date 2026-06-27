## Context

The kernel boots, runs network diagnostics (link, DHCP, ICMP ping), then performs one HTTP chat-completion with a build-configured message and halts. Console output is write-only today (VGA + serial transmit). QEMU headful and headless runs attach serial for output; headless smoke tests grep `chat_completion=ok`.

The human wants a REPL-style chat loop on US keyboard layout, with conversation history included in every inference request to the existing llama.cpp OpenAI-compatible endpoint (`192.168.1.110:11435` by default on the working branch).

Hardware inventory for the Medion Akoya EX documents a touchpad but does not name the keyboard controller explicitly. The machine is a 2005-era Centrino notebook (Intel 855GM / ICH-class chipset, Pentium M, Windows XP). Integrated laptop keyboards of this class attach through the standard PC **i8042 PS/2 keyboard controller** (ports 0x60/0x64), not through the serial console. USB ports exist but the built-in keyboard is not a USB HID device in normal operation.

## Goals / Non-Goals

**Goals:**

- After successful network diagnostics, enter an interactive chat session on the console.
- On bare metal and in QEMU, read input from the **integrated PS/2 keyboard path** (i8042 controller) that matches the Akoya-class notebook built-in keyboard.
- Translate key events to US QWERTY semantics: printable ASCII, Backspace edits the current line, Enter submits the line.
- Accumulate user and assistant messages in memory; each chat-completion HTTP request sends the full `messages` array built so far.
- Print assistant replies on the console before the next input prompt.
- Explicit session exit (e.g. user types `quit` or `exit`) then orderly halt.
- Headless smoke test: inject scripted keystrokes into the **emulated PS/2 keyboard** (same guest input stack as headful SDL typing), assert at least one successful completion in captured output.

**Non-Goals:**

- Non-US keyboard layouts, modifier keys beyond basic typing, or touchpad/mouse as chat input.
- Unicode beyond 7-bit printable ASCII for v1.
- USB HID keyboard support (external USB keyboards on Akoya ports).
- TLS/HTTPS, DNS, streaming, tool calls, or image attachments.
- Persisting conversation across reboots or to disk.
- Serial console as the operator keyboard path on bare metal (serial remains output + automation injection only where runner can feed the keyboard path).

## Decisions

### 1. i8042 PS/2 keyboard controller as the deployment-target input path

**Choice:** Implement keyboard input through the i8042 PS/2 controller—the standard integrated keyboard interface for Pentium M / ICH-era notebooks including the Medion Akoya EX class. Decode scan codes to US-layout characters in a dedicated input module. Do **not** treat serial receive as the primary operator keyboard on bare metal.

**Rationale:** Exact fit for built-in laptop keyboard on target hardware; QEMU `pc` machine model already emulates i8042, so headful SDL keyboard and headless `sendkey`/monitor injection exercise the same guest code path as bare metal.

**Alternatives:**

| Option | Why not |
|--------|---------|
| Serial stdin as primary input | Wrong device on Akoya; serial is COM1 diagnostic output, not the built-in keyboard |
| USB HID driver | Built-in keyboard is not USB on this class; adds enumeration stack |
| VGA text-mode polling only | Does not receive keyboard events without controller driver |

**Inventory note:** Keyboard controller is inferred from chipset era and notebook form factor, not a named inventory field. A follow-on inventory entry may document PS/2/i8042 when confirmed on hardware; do not invent controller part numbers in inventory from this change alone.

### 2. QEMU emulation mirrors bare-metal keyboard path

**Choice:** Keep `-serial stdio` for **output** capture. Headful: SDL delivers key events to emulated i8042 (default QEMU PC behavior). Headless: runner injects keystrokes via QEMU monitor `sendkey` (or equivalent) into emulated PS/2, not by piping bytes to serial stdin.

**Rationale:** Automated tests validate the same kernel keyboard driver the Akoya will use.

### 3. In-memory conversation buffer with bounded turns

**Choice:** Fixed-capacity store of message pairs (e.g. max 16 turns or ~8 KB JSON budget). Oldest turns dropped or session refuses new input when full—documented in README.

**Rationale:** No heap; predictable memory on i686 unikernel.

### 4. HTTP client reuse per turn

**Choice:** Each submitted line triggers one new TCP connection and POST (same as current single-shot client). Request body lists all messages in history order.

### 5. Per-turn console tokens

**Choice:** Keep `chat_completion=ok reply=...` / `chat_completion=fail reason=...` per exchange; add a visible prompt line (e.g. `chat>`) before each user input.

### 6. Session exit command

**Choice:** Lines `quit` or `exit` (case-insensitive) end the session without sending inference. Empty lines ignored or re-prompt.

### 7. Headless smoke test scripting

**Choice:** Default script types `hi` + Enter + `quit` + Enter via emulated keyboard injection when `AKOYA_CHAT_SCRIPT` unset. Assert first `chat_completion=ok` in log.

### 8. Bootstrap diagnostic path unchanged

**Choice:** Initial bootstrap message, DHCP, and ping remain before any interactive session.

## Risks / Trade-offs

- **[Risk] Inventory lacks explicit keyboard controller name** → Design records PS/2/i8042 as era-appropriate inference; bare-metal validation on Akoya confirms.
- **[Risk] i8042 driver complexity** → Minimal poll-based driver; scan code set 1 for US QWERTY; bounded line buffer.
- **[Risk] JSON size exceeds buffer** → Cap history; report `chat_completion=fail reason=overflow`.
- **[Risk] Headless sendkey timing** → Runner waits for chat prompt before injection; bounded timeout.
- **[Trade-off] New TCP connection per turn** → Acceptable for interactive use.

## Migration Plan

1. Add i8042 PS/2 keyboard input module and scan-code → US ASCII mapping.
2. Add session loop and conversation history.
3. Extend HTTP chat for multi-message JSON.
4. Update runner for keyboard injection in headless; verify headful SDL path.
5. Update README; run `make test`.

Rollback: restore single-shot probe and write-only console.

## Open Questions

- Exact max history size—implementation documents cap.
- Optional future inventory row confirming i8042 on Akoya EX when hardware is observed (not blocking apply).
