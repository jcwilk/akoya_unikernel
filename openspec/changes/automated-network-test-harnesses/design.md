# Design: Automated Network Test Harnesses

## Context

The bootstrap unikernel ships an interactive chat session over the production network stack (link, IPv4, DHCP, TCP, HTTP). Development verification today relies on `scripts/run-qemu.sh` headless mode with `AKOYA_CHAT_SCRIPT` — space-separated QEMU `sendkey` names and basic plain-reply assertions. That path sometimes passes single-turn or default scripts while headful multi-turn chat fails on later turns with connection errors (for example `chat failed: connect`).

Agents cannot autonomously verify TCP lifecycle or multi-turn chat without human keyboard input. The operator principle: everything built must be exercisable by an agent via automated harnesses; if the shipping interactive product cannot be fully agent-controlled, separate test harnesses must exercise key code paths.

Recent work on per-turn TCP teardown (`sequential-chat-tcp-lifecycle`) clarified session ordering but did not add dedicated transport verification or assertion-rich scripting. Uncommitted experimental edits in `kernel/net/tcp/tcp.c` on branch `cursor/49a5d967` are out of scope for this proposal; apply for this change should not depend on committing or reverting that local state.

## Goals / Non-Goals

**Goals:**

- Deliverable A: a **second logical boot image** (`transport-test` identity) that links the same production network sources as the main kernel but runs a non-interactive timed scenario suite and prints per-scenario plus aggregate pass/fail.
- Deliverable B: a **host-side scripted interaction harness** wrapping the **main chat image** with expect/assert semantics between steps (delays, type lines, assert patterns, optional exit).
- Both harnesses runnable in one agent command via extensions to `scripts/build.sh` and `scripts/run-qemu.sh` (or thin wrappers) on the existing workstation LAN QEMU path.
- Headless-first for CI/agents; document whether headful keyboard injection is supported if attempted.

**Non-Goals:**

- Bare-metal Akoya EX automated harness (QEMU-first).
- Replacing operator headful chat experience.
- Mock/fake TCP stack.
- Fixing root-cause TCP bugs inside this change (harnesses expose failures; fixes may follow).

## Decisions

### Decision 1: Separate entry point, shared network objects

**Choice:** Add a second kernel entry (`kernel/net/transport_test_main.c` or equivalent) and link target in `scripts/build.sh` producing `build/transport-test.bin` (name illustrative) as a distinct **logical identity** from `build/kernel.bin`. Compile and link the same `kernel/net/link/`, `kernel/net/tcp/`, DHCP, and IPv4 objects used by the main kernel — no forked stack.

**Rationale:** Scenarios stress production TCP paths without chat/HTTP/UI noise; distinct identity satisfies runner auto-selection rules when two images coexist.

**Rejected — in-kernel test mode flag on main image:** Would conflate operator chat image with test behavior and complicate logical identity.

**Rejected — userspace mock TCP:** Violates operator requirement to reuse production stack.

### Decision 2: Timed delays via busy-wait or existing timer primitive

**Choice:** Implement `delay_ms(unsigned)` (or reuse an existing tick/delay helper) in the transport-test image. Scenario scheduler sleeps fixed durations between connection attempts to simulate slow operator gaps.

**Rationale:** No PS/2 readline loop; fully deterministic for agents.

### Decision 3: Transport scenario suite (initial set)

**Choice:** Hard-code or table-drive a bounded suite in the transport-test main module:

| Scenario | Intent |
|----------|--------|
| `sequential-same-host` | N≥2 outbound TCP connects to configured chat host:port in sequence, each fully closed before the next |
| `delayed-reconnect` | Connect, close, delay (e.g. 2–5s), connect again to same host |
| `refused-or-timeout` | Connect to a closed port or unroutable target; expect categorized failure; mark pass when failure is detected as expected |

Each prints `scenario <name>: PASS` or `scenario <name>: FAIL (<reason>)`. Final line: `transport-test: ALL PASS` or `transport-test: FAILED (<count> failures)`.

**Unreachable endpoint policy:** If pre-flight on the workstation shows chat host unreachable, transport-test entry point may skip HTTP-dependent scenarios but **must still run** TCP-to-configured-host scenarios when the host is reachable; refused-port scenario uses a dedicated unused port on localhost/LAN. Document skip vs hard-fail in runner README.

**Rationale:** Targets the reported pain point (turn 2+ connect failures) without requiring chat HTTP.

### Decision 4: Script format for full-app harness (Deliverable B)

**Choice:** Introduce a line-oriented script file (e.g. `*.akoya-script`) parsed by host-side harness code invoked from `run-qemu.sh` or sibling `scripts/run-chat-script.sh`:

```
# comments allowed
delay 2000          # ms before next action
expect "is reachable"
type "reply only with the word PONG"
expect "PONG"
delay 1000
type "second message"
expect_line reply   # heuristic: next non-empty line after prompt not starting with "chat failed"
type "/quit"
expect "bootstrap ok"  # optional shutdown marker
```

Keystroke injection continues via QEMU monitor `sendkey` for headless (same path as `AKOYA_CHAT_SCRIPT`). **Assertion engine** watches accumulated serial log with substring or regex match; on mismatch, kill QEMU and exit non-zero with step number and expected vs actual snippet.

**Default smoke:** Keep existing `AKOYA_CHAT_SCRIPT` string format for backward compatibility; new `--script FILE` flag opts into assert semantics.

**Headful:** If QEMU sendkey works only headless in practice, document headless as primary; headful may reuse the same monitor socket when display mode is headful — verify during apply task 5.x.

**Rejected — expect-lite only in env var:** Multi-turn assert scripts need structure; file format is clearer for agents.

### Decision 5: Runner entry points

**Choice:**

- `scripts/run-qemu.sh --headless --image build/transport-test.bin` (or `--logical transport-test`) — existing image selection rules.
- `scripts/run-transport-test.sh` — thin wrapper: build transport image if missing, run headless, grep aggregate line, exit code.
- `scripts/run-qemu.sh --headless --script path/to/script.akoya-script` — full-app harness.

**Rationale:** One command per deliverable for agents; reuses LAN macvtap, timeout, and serial capture from existing runner.

### Decision 6: Build layout

**Choice:** Extend `scripts/build.sh`:

```
build/kernel.bin          # main chat unikernel (existing)
build/transport-test.bin  # transport test image (new target)
```

Shared objects compiled once where Makefile/linker allows; otherwise duplicate compile of shared `.c` files into both links — prefer single compile, dual link if toolchain supports.

## Architecture sketch

```
┌─────────────────────────────────────────────────────────────┐
│ scripts/build.sh                                            │
│   ├─ kernel.bin      ← netmain.c + chat + http              │
│   └─ transport-test.bin ← transport_test_main.c + shared net│
└─────────────────────────────────────────────────────────────┘
         │                              │
         ▼                              ▼
┌─────────────────────┐    ┌──────────────────────────────┐
│ run-qemu.sh         │    │ run-transport-test.sh        │
│  --script FILE      │    │  (headless LAN smoke)        │
│  sendkey + expect   │    │  assert aggregate PASS       │
└─────────────────────┘    └──────────────────────────────┘
         │
         ▼
   serial log → assertion engine → exit 0/1
```

## Risks / Trade-offs

- **[Risk] Two images break auto-selection** → Explicit `--image` / logical name in wrappers; error message lists both identities (existing dev-test-runner contract).
- **[Risk] Timing flakes on slow LAN** → Configurable delays in script and transport scenarios; generous headless timeout (existing 300s default).
- **[Risk] sendkey race with guest prompt** → Harness waits for `expect` patterns before typing; optional `delay` lines.
- **[Trade-off] Hard-coded transport scenarios vs configurable** → Start hard-coded; extensibility via table in transport_test_main if needed later.
- **[Trade-off] Script parser maintenance** → Minimal grammar first; avoid Turing-complete shell.

## Migration Plan

1. Land build target and transport-test image; verify headless aggregate PASS on healthy LAN.
2. Add script harness without changing default `run-qemu.sh` behavior for existing smoke.
3. Add sample multi-turn script (`scripts/fixtures/multi-turn-pong.akoya-script`) for agent regression.
4. Document new entry points in README (apply task).

Rollback: remove new build target and runner flags; main chat image unchanged.

## Open Questions

- Exact unused port for refused-connection scenario on operator LAN (pick high ephemeral, document in README).
- Whether transport-test should share chat host IP env vars (`AKOYA_CHAT_HOST_IP`, `AKOYA_CHAT_PORT`) — **presumed yes** for consistency.
