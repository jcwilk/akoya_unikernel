## REMOVED Requirements

### Requirement: Distinct timed-gap chat regression boot identity

**Reason:** Verification and implementation complexity did not improve idle-at-prompt reliability; coverage moves to the main interactive chat unikernel.

**Migration:** Run multi-turn idle-at-prompt verification on the main chat unikernel via the development test runner default gate.

### Requirement: Production chat turn path reuse

**Reason:** Retired with the regression boot identity; production path is exercised directly by the main session.

**Migration:** None beyond using the main chat unikernel for verification.

### Requirement: Bounded timed idle between scheduled turns

**Reason:** Guest-scheduled gap timers duplicated host-timed idle-at-prompt verification and used different mechanics than headful operator waits.

**Migration:** Host-timed idle at the input prompt in the default scripted verification gate.

### Requirement: Multi-turn schedule with per-turn and aggregate reporting

**Reason:** Aggregate reporting belonged to the retired regression image; main session uses per-turn conversation outcomes and runner pass/fail on captured output.

**Migration:** Assert plain replies and absence of connection-failure lines in the development test runner scripted gate.
