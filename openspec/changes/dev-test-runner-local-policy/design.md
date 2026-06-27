## Context

The project targets a single Linux development workstation with:

- Vendored or host-installed `i686-elf-*` cross toolchain (`tools/bin/` or system PATH)
- Host-installed `qemu-system-i386` (Debian `qemu-system-x86` package)
- SDL headful windows for operator inspection; no Docker emulator fallback, no VNC path

Conversational decisions already implemented in `scripts/run-qemu.sh`, `scripts/build.sh`, and `README.md` need durable spec backing.

## Goals / Non-Goals

**Goals:**

- Document dev-test-runner as pre-hardware 32-bit x86 emulation on the local workstation.
- Spec: headless auto-exits after bootstrap (smoke test); headful holds emulator open after guest halts.
- Spec: `--exit-on-guest-done` / `--hold` (or equivalent) override mode defaults.
- Spec: fail fast when host emulator or cross toolchain is missing — no silent container fallback.
- Replace both capabilities' `Purpose: TBD` stubs with real text at archive.

**Non-Goals:**

- Multi-machine CI farms, remote builders, or containerized dev environments.
- Remote desktop / VNC / browser-based viewers for headful mode.
- Changing bootstrap kernel or Akoya deployment-target requirements.

## Decisions

### 1. Guest shutdown via emulator configuration (not kernel changes)

**Choice:** Default headless attaches guest-shutdown→emulator-exit behavior; default headful omits it so the guest halts in place and the emulator stays open until the operator closes it.

**Rationale:** Matches operator feedback (headful window closed too fast) while preserving fast `make test` automation.

**Overrides:** Callers may force `--exit-on-guest-done` or `--hold` on either display mode.

### 2. Headful = local interactive display only

**Choice:** Remove “remotely viewable display” from the headful requirement; interactive workstation display is sufficient.

**Rationale:** Single-workstation policy; VNC/Docker display paths removed intentionally.

### 3. Local prerequisites are hard requirements

**Choice:** Build and run entry points fail with actionable errors when toolchain or emulator is absent. No Docker reintroduction as fallback.

**Rationale:** Fallback hid missing installs, added ~60s per run, and broke headful display reliability.

### 4. Purpose text (for archive reconciliation)

**dev-test-runner Purpose:**

> Pre-hardware validation by running built boot images under 32-bit x86 emulation on the development workstation, with explicit headful or headless modes, logical image selection, and smoke-test behavior for automation.

**unikernel-build-pipeline Purpose:**

> Cross-compilation of boot images for the Akoya deployment target on the development workstation, with memory-bounded builds and agent-parseable outcomes.

## Risks / Trade-offs

| Risk | Mitigation |
|------|------------|
| New contributors lack toolchain | README prerequisites; fail-fast errors name what to install |
| Spec flags don't mention CLI flag names | Spec describes behavior; design/tasks reference `--hold` / `--exit-on-guest-done` |

## Migration Plan

Apply is mostly spec reconciliation — implementation already on branch. Tasks verify README/scripts match deltas and update Purpose at archive.

## Open Questions

None — policy confirmed in conversation.
