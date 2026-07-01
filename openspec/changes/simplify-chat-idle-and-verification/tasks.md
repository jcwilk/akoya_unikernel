## Required for this change

### Guest chat and network simplification

- [ ] 1.1 Simplify interactive chat to one idle loop: poll operator input and wired network at the prompt; one blocking production turn per submitted message
- [ ] 1.2 Remove per-turn address-cache invalidation, periodic announce refresh, and chat-turn async TCP state machinery used only for mitigation
- [ ] 1.3 Unify TCP open/exchange/close/drain policy; resolve link-layer addresses on send when needed rather than invalidating valid cache entries at turn boundaries
- [ ] 1.4 Service wired receive during input-prompt idle without artificial per-iteration frame caps that can stale inbound readiness across long waits
- [ ] 1.5 Verify headful: first message, substantial wait at prompt, second message — no spurious connection-failure line when endpoint is reachable

### Verification consolidation

- [ ] 2.1 Retire timed-gap chat regression boot image from build outputs and remove its dedicated runner
- [ ] 2.2 Consolidate scripted fixtures to one idle-at-prompt definition (short message, ~20s host delay, short second message, exit)
- [ ] 2.3 Point default automated verification (`make test`) at main chat unikernel idle-at-prompt gate plus transport-only verification; remove redundant idle/repro runners and duplicate fixtures
- [ ] 2.4 Update README verification narrative to match the reduced gate set

### Build and regression evidence

- [ ] 3.1 `make build` succeeds without the regression boot identity
- [ ] 3.2 `make test` passes on a workstation with reachable inference endpoint (idle-at-prompt gate green)
- [ ] 3.3 Transport-only verification still passes when invoked as part of the default gate

## Explicitly deferred

- Mandating N consecutive automated runs as the default CI gate (local stress loop may remain documented but is not part of apply-complete)
- Deploy-faithful USB/ISO verification tier changes
- Guest-timekeeping calibration algorithm rework beyond removing regression-specific call sites
