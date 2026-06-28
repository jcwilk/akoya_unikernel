## Why

Agents and CI cannot reliably verify network, TCP, and multi-turn chat behavior today: headless scripted smoke sometimes passes while headful multi-turn chat fails on later turns (for example connection errors after the first exchange). The project lacks automated harnesses that exercise production code paths without human keyboard intervention. Every shipped capability must be exercisable by an agent in a bounded, pass/fail manner.

## What Changes

- Introduce a **standalone network transport test boot image** with a distinct logical identity from the main chat unikernel. It reuses the production link, IPv4, DHCP, and TCP stack—not a mock or reimplementation—and runs a bounded, non-interactive suite of transport scenarios using timed delays instead of operator input.
- Extend the development test runner so agents can **build and run** the transport test image on the existing workstation LAN emulation path and capture an aggregate pass/fail result with actionable console output.
- Add a **scripted full-app interaction harness** that wraps the main chat unikernel image (the same image operators use headfully). The harness accepts timed interaction scripts (delays, typed strings, optional exit) and **asserts expected console output** between steps—reachability, plain assistant replies, multi-turn success without connection-failure lines—not merely fire-and-forget keystroke injection.
- Provide a single agent-runnable entry point per harness that returns non-zero on failure without manual steps.
- **Hard pre-flight on inference endpoint reachability:** Any automated verification that depends on the configured chat/inference host (transport-test suite, default smoke, scripted full-app harness) **MUST abort before starting emulation** when that endpoint is unreachable from the development workstation. Unreachable inference infrastructure is never a skippable or partial-run condition—it is an environment failure that must be fixed before verification proceeds.

## Capabilities

### New Capabilities

- `network-transport-test`: Non-interactive standalone boot image that exercises production network transport through timed, bounded scenarios and reports per-scenario and aggregate pass/fail on the console.

### Modified Capabilities

- `dev-test-runner`: Agent-runnable verification for both logical boot images; scripted full-app interaction with output assertion semantics between steps; selection when multiple logical images exist; headless-first automated pass/fail with actionable failure output; **mandatory inference-endpoint pre-flight that aborts when unreachable**.

## Impact

- Build system must emit a second logical boot image for transport testing while sharing production network stack sources with the main kernel.
- Runner and host-side harness logic under `scripts/` must grow assertion and script-format support without replacing the operator headful experience.
- Existing default headless smoke behavior for the main chat image should remain available; new harnesses add agent-grade verification rather than removing manual paths.
- Bare-metal Akoya EX automated harness is out of scope (QEMU-first).

## Non-Goals

- Replacing the human headful chat experience for operators who want it.
- Mock or fake TCP stack implementations.
- Bare-metal Akoya EX automated test harness in this change.
