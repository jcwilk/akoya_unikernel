## Why

The bootstrap image already proves wired Ethernet, DHCP, and ICMP reachability, but it stops before exercising application-layer traffic on the LAN. A minimal HTTP chat-completion probe against a local llama.cpp server validates that the guest can carry real workloads beyond ping and establishes an observable end-to-end path from boot through inference response on the console.

## What Changes

- After existing network diagnostics, the bootstrap image SHALL perform one outbound HTTP request to a configured chat-completions endpoint on the LAN, submit a fixed user message (`hi`), and print the server's response body (or a concise failure reason) on the console before halting.
- Extend the in-kernel network stack with TCP and a minimal HTTP client sufficient for a single POST with a small JSON payload and bounded response capture—no general socket API or persistent connections.
- Use an OpenAI-compatible chat-completions request shape so llama.cpp's `/v1/chat/completions` endpoint accepts the payload without custom adapters.
- Default endpoint target: `http://192.168.1.110/v1/chat/completions` (build-time configurable for other LAN hosts).
- Extend headless smoke-test expectations so automated runs assert chat-completion diagnostic console output when the endpoint is reachable on the workstation LAN.
- Keep TLS/HTTPS, streaming responses, DNS resolution, authentication headers, retries, and multi-turn conversation out of scope for this iteration.

## Capabilities

### New Capabilities

- `http-chat-probe`: Observable single-shot HTTP chat-completion request after network bring-up, console reporting of response or failure, and orderly shutdown.

### Modified Capabilities

- `network-unikernel`: Network bootstrap sequence adds TCP transport and HTTP client layers; orderly completion occurs after chat probe output, not immediately after ICMP diagnostics.
- `bootstrap-unikernel`: Bootstrap scope expands to include one application-layer chat probe; deterministic scope still bounded to boot-time diagnostics with no persistent services.
- `dev-test-runner`: Headless smoke tests assert chat-completion diagnostic lines when the configured endpoint is available; bounded runtime accounts for HTTP round-trip.

## Impact

- New kernel modules under `kernel/net/` for TCP and minimal HTTP client.
- `kernel/net/netmain.c` orchestration extended after ICMP probe (or replacing ICMP as final step—see design).
- Build-time constants for endpoint host, path, and user message.
- `scripts/run-qemu.sh` / `Makefile` smoke assertions updated for new console tokens.
- README expected-output section updated.
- No change to QEMU LAN attachment, macvtap setup, or deployment-target hardware assumptions.
