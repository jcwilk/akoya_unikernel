## Context

The repository already builds a multiboot i686 bootstrap image and runs it via `scripts/run-qemu.sh` and Makefile targets (`test`, `run`, `run-vnc`). Recent ad-hoc edits added optional `--headful` and `--vnc` flags with implicit headless behavior for `make test`. The human wants that work folded into an intentional contract before it spreads further.

The build output directory currently holds `kernel.elf`, `kernel.bin`, logs, and other non-runnable files. Artifact auto-selection must distinguish **runnable boot images** from auxiliary outputs.

## Goals / Non-Goals

**Goals:**

- One script (e.g. `scripts/run-image.sh` or a refactored `run-qemu.sh`) as the documented run entry point.
- Mandatory `--headful` or `--headless` flag on every invocation; missing mode → usage error, non-zero exit.
- Optional `--image PATH` (or positional path); omitted path triggers scan of the configured build output directory for runnable images only.
- Auto-select when exactly one runnable image exists; fail fast listing candidates when zero or multiple.
- Headless mode retains smoke-test semantics: serial capture, bootstrap message assertion, bounded timeout.
- Headful mode opens an interactive display (native window or documented VNC fallback for Docker/remote hosts); no smoke-test timeout/assertion pass/fail gate—user observes and closes.
- Makefile targets become thin wrappers that pass explicit mode flags.

**Non-Goals:**

- Changing what the build pipeline emits or how images are named.
- Bare-metal boot automation.
- Supporting run modes beyond headful and headless (e.g. no default third mode).
- Multi-image catalogs or version pinning beyond the exactly-one auto-select rule.

## Decisions

### 1. Single run script, refactor in place

**Choice:** Evolve `scripts/run-qemu.sh` into the canonical runner (rename optional if clearer) rather than adding a second parallel script.

**Rationale:** Avoid duplicate QEMU flag wiring; existing Docker fallback and multiboot load path stay centralized.

### 2. Runnable image detection

**Choice:** Treat files matching `*.elf` and `*.bin` directly under the build output directory as runnable candidates; exclude `*.log` and other suffixes. Prefer `.elf` when both stem-named `.elf` and `.bin` exist (count as one image).

**Rationale:** Matches current build outputs without scanning recursively; prevents counting logs as images.

### 3. CLI shape

```
run-image.sh --headful [--image PATH]
run-image.sh --headless [--image PATH]
```

Mutually exclusive mode flags. Supplying neither or both is an error.

### 4. Makefile mapping

| Target | Maps to |
|--------|---------|
| `make test` | `run-image.sh --headless` (build first if needed) |
| `make run` | `run-image.sh --headful` (build first if needed) |

Remove separate `run-vnc` target; document `AKOYA_QEMU_VNC=1` or `--vnc` as headful fallback in README when native display fails (implementation detail in tasks, not spec).

### 5. Headless vs headful behavior split

| Behavior | Headless | Headful |
|----------|----------|---------|
| Display | none / serial only | SDL/GTK/VNC |
| Timeout + message assert | yes | no |
| Exit on bootstrap message seen | pass/fail gate | informational only |
| Serial on stdio | yes | yes (optional mirror) |

## Risks / Trade-offs

| Risk | Mitigation |
|------|------------|
| Ambiguous "one image" when build emits extras | Runnable suffix filter + elf/bin dedup by stem |
| Breaking existing muscle memory (`make test` implicit headless) | Makefile wrappers preserve targets; direct script callers must pass mode |
| Headful still flaky under Docker+X11 | Document VNC headful fallback; headless smoke test remains CI/agent path |

## Migration Plan

1. Implement new CLI contract in run script.
2. Update Makefile and README.
3. Remove ad-hoc uncommitted flag combinations that contradict mandatory mode.
4. Re-run `make test` (headless) for regression evidence.

## Open Questions

- Final script name: keep `run-qemu.sh` vs rename to `run-image.sh` (default: refactor in place unless rename aids clarity).
