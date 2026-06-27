# Proposal: Minimal Chat Console

## Why

The interactive chat session works functionally, but boot-time network diagnostics and chat output still read like a debug trace: external connectivity probes, verbose key=value status lines, and a branded input prompt clutter the console before and during conversation. Operators need a minimal, conversation-focused console that confirms LAN reachability of the inference host, clears prior boot noise, and presents chat as plain dialogue.

## What Changes

- Point the pre-chat ICMP connectivity probe at the **same configured chat/inference host on the LAN** used for HTTP chat-completion requests, not an unrelated external target.
- **Clear the console display** immediately before the reachability probe so prior boot and network messages do not remain visible during the chat UI.
- On successful reachability, print a **short human-readable success line** naming the configured host (exact wording in design/tasks).
- Replace the branded chat input prompt with a **minimal `> ` prompt** (greater-than plus trailing space).
- Print **assistant replies as plain conversation text** on their own lines; remove key=value diagnostic framing from successful turns. Keep failure messages brief and readable.
- **Preserve** multi-turn conversation history, integrated keyboard input (US layout with shift), explicit session exit, and headless multi-turn smoke testing — updating automated assertions to match the new minimal output contract.

## Capabilities

### New Capabilities

_(none — behavior refinements stay within existing capability domains)_

### Modified Capabilities

- `network-unikernel`: connectivity probe target aligns with chat/inference host; console cleared before probe; concise reachability success output.
- `http-chat-probe`: per-turn console reporting uses plain conversation text without diagnostic key=value framing.
- `interactive-chat-session`: minimal input prompt before each user turn.
- `bootstrap-unikernel`: boot-time console transition into the chat-focused UI (clear before probe) fits bounded bootstrap scope.
- `dev-test-runner`: headless smoke assertions match the minimal console output contract.

## Impact

- Kernel network bootstrap and console layers (connectivity probe target selection, display clear, probe success formatting).
- HTTP chat session console output formatting.
- Interactive session input prompt rendering.
- Headless QEMU smoke-test output matchers in the development run entry point.
