# Design: Minimal Chat Console

## Context

The bootstrap unikernel currently probes **8.8.8.8** (Google DNS) for ICMP reachability while HTTP chat-completion traffic targets **192.168.1.110** on the LAN via `AKOYA_CHAT_HOST_IP*` build-time constants in `kernel/net/http/http_chat.c`. Boot and network layers emit key=value diagnostic lines (`net_link=ok`, `net_ip=…`, `net_ping=… status=ok rtt_ms=…`) that remain on screen when the interactive chat session starts. Chat output uses `chat_completion=ok reply=…` / `chat_completion=fail reason=…` framing and the input prompt is `chat>`.

The operator wants a conversation-focused console: probe the inference host, clear boot noise, confirm reachability briefly, then present plain dialogue with a `> ` prompt. Headless smoke tests in `scripts/run-qemu.sh` currently grep for `net_ping=… status=ok rtt_ms=…` and `chat_completion=ok reply=…` patterns.

## Goals / Non-Goals

**Goals:**

- Align ICMP probe target with the configured chat/inference host (same IP octets as `AKOYA_CHAT_HOST_IP0–3`).
- Clear the VGA text console immediately before the probe so prior boot/network lines are not visible during chat.
- Print probe success as a short line: `<host-label> reachable` (e.g. `192.168.1.110 reachable`).
- Print probe failure as a short readable line (e.g. `<host-label> unreachable` or `<host-label> timeout`).
- Change the chat input prompt to `> ` via `console_write_prompt("> ")`.
- Print assistant replies as plain text on their own lines; print failures as brief readable sentences without `key=value` prefixes.
- Update `scripts/run-qemu.sh` headless assertions for the new output shapes while preserving multi-turn scripted input (`AKOYA_CHAT_SCRIPT` default `h i ret w h a t ret q u i t ret`).

**Non-Goals:**

- Changing OpenAI-compatible HTTP request/response handling or conversation history retention.
- Changing keyboard input path, US layout, shift handling, or exit command semantics.
- Adding new build-time configuration knobs beyond reusing existing chat-host constants for the probe.
- Redesigning DHCP or link-layer diagnostics (they still run; they are just cleared from display before chat).

## Decisions

### Decision: Reuse chat-host IP constants for ICMP probe target

**Choice:** Remove separate `AKOYA_PROBE_TARGET_IP*` defaults in `kernel/net/netmain.c` and derive the probe target from the same `AKOYA_CHAT_HOST_IP0–3` octets (and a shared host label string) used by the HTTP client.

**Rationale:** Guarantees probe and chat traffic target the same LAN inference host without drift between two build-time constant sets.

**Alternative considered:** Keep separate probe constants defaulting to chat host — rejected because dual constants invite silent mismatch.

### Decision: Add `console_clear()` before probe in `net_bootstrap`

**Choice:** Implement `console_clear()` in `kernel/console/console.c` (VGA text-mode scroll/clear or equivalent) and call it in `netmain.c` after DHCP address reporting and gateway resolution, immediately before `icmp_ping`.

**Rationale:** Spec requires prior boot trace not visible when reachability output and chat UI appear. Clearing after address report preserves serial log capture for smoke tests while cleaning the on-screen display.

**Alternative considered:** Suppress earlier prints — rejected; boot diagnostics are still useful in serial capture and debugging.

### Decision: Minimal reachability and chat output strings

**Choice:**

| Event | New console line (examples) |
|-------|----------------------------|
| Probe success | `192.168.1.110 reachable` |
| Probe unreachable | `192.168.1.110 unreachable` |
| Probe timeout | `192.168.1.110 timeout` |
| Chat success | assistant reply text only, then newline |
| Chat failure | brief sentence, e.g. `chat failed: timeout` |

**Rationale:** Matches operator intent for plain conversation UI; exact format lives here and in tasks, not in spec deltas.

### Decision: Headless smoke assertion updates

**Choice:** In `scripts/run-qemu.sh` headless pass/fail checks:

- Replace `net_ping=… status=ok rtt_ms=…` grep with a line matching `${CHAT_HOST} reachable` (using `AKOYA_CHAT_HOST_IP` env default `192.168.1.110`).
- Replace `chat_completion=ok reply=.` count with detection of plain reply content — e.g. require at least two non-empty assistant reply lines or match known reply substrings from the default multi-turn script, and reject presence of `chat_completion=` framing.
- Optionally retain `net_ip=` assertion in serial log (still emitted before clear).

**Rationale:** Aligns automation with the minimal output contract without dropping multi-turn coverage.

## Risks / Trade-offs

- **[Risk] Serial log still contains pre-clear diagnostics while display is clean** → Acceptable; headless assertions read the full serial log, not the cleared screen state.
- **[Risk] Plain-text failure lines harder to machine-parse** → Mitigated by brief, consistent prefixes in design (`chat failed: …`) and grep updates in the runner.
- **[Risk] `console_clear` implementation varies by emulator vs bare metal** → Test under QEMU headless and headful; VGA text clear is already the console backend.

## Migration Plan

1. Implement kernel console clear and output formatting changes.
2. Align probe target constants with chat host.
3. Update `run-qemu.sh` assertion patterns.
4. Run `./scripts/build.sh` then `./scripts/run-qemu.sh --headless` on a workstation with LAN DHCP and reachable chat endpoint.

Rollback: revert kernel and script commits; prior key=value output and 8.8.8.8 probe restore previous behavior.

## Open Questions

- None blocking apply; exact grep heuristics for plain-text multi-turn replies may be refined during headless smoke test implementation if the default script replies are too variable.
