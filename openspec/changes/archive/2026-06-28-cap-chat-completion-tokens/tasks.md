# Tasks: Cap Chat Completion Output Tokens

## Required for this change

## 1. Request payload

- [x] 1.1 Extend chat-completion JSON construction in `kernel/net/http/http_chat.c` to include a completion output limit of 500 on every request (first and follow-up turns)
- [x] 1.2 Wire default 500 through `scripts/build.sh` as a build-time define (e.g. `AKOYA_CHAT_MAX_TOKENS`) so the deployment profile matches the spec

## 2. Build and verification

- [x] 2.1 Run `scripts/build.sh` successfully
- [x] 2.2 Run headless smoke via `scripts/run-qemu.sh --headless` and confirm at least one successful plain assistant reply (endpoint accepts capped payload)
- [x] 2.3 Inspect built request path or add a brief code comment only if needed—no new diagnostic console output required for this change

## Explicitly deferred

- Console truncation indicators when the endpoint clips output at the cap
- Runtime or operator-adjustable token limits
