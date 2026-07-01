## REMOVED Requirements

### Requirement: Distinct timed-gap chat regression boot identity

**Reason:** Superseded by main-kernel idle-at-prompt verification with maintained transport stack.

**Migration:** Default automated verification on the main chat unikernel.

### Requirement: Production chat turn path reuse

**Reason:** Retired with regression boot identity.

**Migration:** Main chat unikernel exercises production path directly.

### Requirement: Bounded timed idle between scheduled turns

**Reason:** Guest-scheduled gaps replaced by host-timed idle-at-prompt gate on main kernel.

**Migration:** Default dev-test-runner gate.

### Requirement: Multi-turn schedule with per-turn and aggregate reporting

**Reason:** Aggregate reporting belonged to retired regression image.

**Migration:** Per-turn console outcomes and runner pass/fail on captured output.
