## Context

The bootstrap unikernel boots on i686 with a layered network stack (RTL8139 driver, Ethernet link, IPv4, DHCP, ICMP ping probe). Boot flow: console message → `net_bootstrap()` (link, DHCP, ping) → QEMU debug exit → halt loop.

The human wants the image to automatically call a llama.cpp server on the LAN at `http://192.168.1.110/v1/chat/completions` using OpenAI-compatible JSON, send user message `hi`, print the assistant reply on the console, then exit. This is the first TCP and HTTP traffic in the kernel.

Prior change `basic-network-integration` explicitly excluded TCP, HTTP, and DNS. DHCP and ICMP are proven; the LAN attachment path (macvtap on wired interface) is unchanged.

## Goals / Non-Goals

**Goals:**

- After existing ICMP connectivity probe, perform one HTTP POST to the configured chat-completions URL with OpenAI-shaped JSON (`messages: [{role: user, content: hi}]`).
- Parse a successful llama.cpp/OpenAI response enough to extract assistant `content` from the first choice and print it on the console with stable tokens for automation (e.g. `chat_completion=ok` / `chat_reply=...`).
- Add minimal TCP (single connection, no listen socket) and HTTP client (POST, bounded response buffer) as new layers under `kernel/net/`.
- Extend headless smoke test to assert chat-completion success when `192.168.1.110` is reachable from the guest on the workstation LAN.
- Preserve existing bootstrap message, DHCP reporting, and ping diagnostics ordering.

**Non-Goals:**

- TLS/HTTPS, HTTP/1.1 keep-alive, chunked encoding beyond minimal handling, or HTTP/2.
- DNS client (endpoint host is a build-time IPv4 constant: `192.168.1.110`).
- Authentication headers, API keys, model selection beyond a fixed default model string in JSON.
- Streaming (`stream: true`), tool calls, or multi-turn conversation.
- General socket API exposed to future kernel code.
- lwIP or other external network library port.
- Removing the ICMP ping probe (retained as a pre-check before HTTP).
- Bare-metal validation automation (QEMU LAN path is the apply acceptance target).

## Decisions

### 1. Keep ICMP probe, add HTTP after ping

**Choice:** Run chat-completion HTTP only after successful DHCP and ICMP probe, in the same `net_bootstrap()` orchestration.

**Rationale:** Ping confirms general LAN/router reachability cheaply; HTTP failure against a specific host is easier to diagnose when ping already succeeded.

**Alternatives:**

| Option | Why not |
|--------|---------|
| Replace ping with HTTP only | Loses fast ICMP signal when gateway is down |
| HTTP before ping | Harder to triage link vs endpoint failures |

### 2. Layer layout: `kernel/net/tcp/` and `kernel/net/http/`

**Choice:**

- `kernel/net/tcp/tcp.c` — minimal TCP client: SYN handshake, send segment(s), receive until FIN or buffer full, single outstanding connection.
- `kernel/net/http/http_chat.c` — build POST body, format HTTP/1.0 request, invoke TCP client, parse status line and body, extract JSON `choices[0].message.content` with a small hand-rolled scanner (no full JSON parser).

**Rationale:** Matches existing modular `dhcp/`, `icmp/` pattern; keeps HTTP parsing out of TCP layer.

**Alternatives:**

| Option | Why not |
|--------|---------|
| Monolithic `http.c` with embedded TCP | Harder to test/replace either layer |
| lwIP | Heavy for one POST |

### 3. Build-time endpoint constants

**Choice:** Compile-time defines (mirroring `AKOYA_PROBE_TARGET_IP*` pattern):

- `AKOYA_CHAT_HOST_IP0..3` default `192.168.1.110`
- `AKOYA_CHAT_PATH` default `/v1/chat/completions`
- `AKOYA_CHAT_USER_MSG` default `hi`
- `AKOYA_CHAT_MODEL` default `default` (llama.cpp accepts various model strings)

`scripts/build.sh` passes `-D` flags; README documents overrides.

**Rationale:** No DNS stack; same pattern as ping target resolution.

### 4. HTTP/1.0 POST with Content-Length

**Choice:** HTTP/1.0 request, `Connection: close`, explicit `Content-Length`, small fixed JSON body template:

```json
{"model":"default","messages":[{"role":"user","content":"hi"}]}
```

**Rationale:** llama.cpp OpenAI-compatible server accepts this shape; HTTP/1.0 + close simplifies response boundary (read until connection closes or buffer cap).

### 5. Console tokens for automation

**Choice:** Stable single-line tokens:

- Success: `chat_completion=ok reply=<escaped-or-truncated-text>`
- Failure: `chat_completion=fail reason=<category>` where category is one of `connect`, `timeout`, `http`, `parse`

Truncate reply in console if longer than ~256 bytes; full body cap in memory ~4 KB.

**Rationale:** Matches `net_ip=`, `net_ping=` pattern for `make test` grep assertions.

### 6. Smoke test policy when endpoint absent

**Choice:** Headless `make test` requires chat-completion success only when the build host can reach `192.168.1.110:80` at test time (pre-flight check in `run-qemu.sh`). If unreachable, skip chat assertion with a visible warning—not a hard fail—so CI laptops without llama.cpp still pass network smoke tests.

**Rationale:** Chat probe is LAN-environment-dependent unlike DHCP/ping on a typical home LAN.

**Alternative:** Always fail without llama.cpp — too brittle for contributors without local inference server.

Document in README that full acceptance requires llama.cpp on `192.168.1.110`.

### 7. Timeouts

**Choice:** Chat HTTP bounded wait ~60s total (TCP connect + request + response), configurable via `AKOYA_CHAT_TIMEOUT_MS`. Headless runner timeout bumped accordingly (e.g. +90s over current).

## Risks / Trade-offs

- **[Risk] llama.cpp not running on LAN during test** → Pre-flight skip with warning; human attestation task for full acceptance.
- **[Risk] Minimal TCP bugs under real LAN** → Iterative QEMU testing on macvtap; bounded buffers prevent heap use.
- **[Risk] JSON parsing fragility** → Scanner targets known llama.cpp response shape; `parse` failure category on mismatch.
- **[Risk] Large model responses exceed buffer** → Truncate console output; cap stored body size.
- **[Trade-off] No HTTPS** → LAN-only diagnostic; documented non-goal.

## Migration Plan

1. Implement TCP + HTTP layers and extend `net_bootstrap()`.
2. Update `scripts/build.sh` with chat constants.
3. Update `scripts/run-qemu.sh` timeout and assertions.
4. Update README expected output section.
5. Verify `make test` on workstation with llama.cpp reachable at `192.168.1.110`.

Rollback: revert kernel net modules and runner assertions; ICMP-only bootstrap restored.

## Open Questions

- Confirm llama.cpp on `192.168.1.110` uses port 80 (HTTP) vs another port — default assumes port 80 from URL without explicit port.
- Whether `make test` should hard-require chat success on this workstation (human has llama.cpp at that IP per request — design assumes yes for their environment).
