# Tasks

## Required for this change

## 1. TCP client layer

- [x] 1.1 Add `kernel/net/tcp/` with minimal outbound TCP client (SYN handshake, send, receive until FIN or buffer cap, single connection)
- [x] 1.2 Integrate TCP with existing IPv4 and link poll loop; wire into `scripts/build.sh` compile list

## 2. HTTP chat-completion client

- [x] 2.1 Add `kernel/net/http/http_chat.c` building OpenAI-compatible JSON POST with user message `hi` and default model string
- [x] 2.2 Parse HTTP/1.x response status and body; extract assistant `content` from `choices[0].message.content` with bounded scanner
- [x] 2.3 Emit console tokens `chat_completion=ok reply=...` or `chat_completion=fail reason=<category>` per design

## 3. Bootstrap orchestration

- [x] 3.1 Add build-time constants in `scripts/build.sh`: host `192.168.1.110`, path `/v1/chat/completions`, message `hi`, timeout (~60s)
- [x] 3.2 Extend `net_bootstrap()` to invoke chat probe after successful ICMP ping; skip when DHCP or ping failed
- [x] 3.3 Confirm guest still triggers QEMU debug exit after chat diagnostics (`kernel/main.c` unchanged or verified)

## 4. Dev test runner

- [x] 4.1 Increase headless timeout in `scripts/run-qemu.sh` to account for chat HTTP round-trip
- [x] 4.2 Add headless assertion for `chat_completion=ok` and non-empty reply when pre-flight detects `192.168.1.110:80` reachable from host
- [x] 4.3 When endpoint unreachable at pre-flight, log skip warning without failing network-only smoke test

## 5. Documentation

- [x] 5.1 Update `README.md` expected console output with chat-completion lines and llama.cpp prerequisite on `192.168.1.110`

## 6. Build and acceptance

- [x] 6.1 `make build` succeeds with new modules
- [ ] 6.2 `make test` passes headless smoke test on development workstation (with llama.cpp reachable at `192.168.1.110` when chat assertions enabled)

## Explicitly deferred

- TLS/HTTPS to the chat endpoint
- DNS-based endpoint configuration
- Bare-metal Akoya EX acceptance (QEMU LAN path is apply target)
- Streaming responses and multi-turn conversation
- General-purpose socket API for kernel consumers
