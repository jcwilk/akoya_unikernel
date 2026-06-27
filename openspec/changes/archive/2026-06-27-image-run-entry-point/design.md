## Context

The repository builds multiboot i686 boot images and runs them via a QEMU wrapper script and Makefile targets. Today `make test` implies headless smoke testing with no explicit mode flag, and the runner assumes a fixed kernel path rather than resolving what to run from the build output area.

A single build often emits **multiple on-disk files for one logical image** — for example `kernel.elf` (linked executable with boot metadata) and `kernel.bin` (flat binary derived from it). Future builds may follow the same pattern per version (`v1.elf` + `v1.bin`, `network.elf` + `network.bin`). Auto-selection must count **logical images** (by stem/identity), not raw file count.

The build output area also holds non-runnable artifacts (object files, logs). Discovery must ignore those.

## Goals / Non-Goals

**Goals:**

- One documented run script as the canonical emulation entry point.
- Mandatory `--headful` or `--headless` on every invocation; missing mode → usage error, non-zero exit.
- Optional `--image PATH`; omitted path triggers scan of the build output area for runnable images grouped by logical identity.
- **Stem dedup (confirmed):** `v1.elf` + `v1.bin` (and any other co-emitted variants sharing stem `v1`) count as **one** logical image `v1`. Same for today's `kernel` stem.
- Auto-select when exactly one logical image exists; fail fast listing logical identities when zero or multiple.
- Headless: smoke-test semantics (serial capture, bootstrap message assertion, bounded timeout).
- Headful: interactive or remotely viewable display; no automated pass/fail timeout gate.
- Makefile `test` / `run` become thin wrappers passing explicit mode flags.

**Non-Goals:**

- Changing what the build pipeline emits.
- Bare-metal boot automation.
- A third run mode or implicit default.
- Version catalogs beyond the exactly-one auto-select rule.

## Decisions

### 1. Single run script, refactor in place

**Choice:** Evolve the existing QEMU run script into the canonical runner rather than adding a parallel script.

**Rationale:** Centralizes multiboot load, Docker fallback, and display wiring.

### 2. Logical image identity (stem dedup)

**Choice:** Discover runnable candidates as `*.elf` and `*.bin` files directly under the build output directory. Group by **stem** (filename without suffix). Each stem is one logical image regardless of how many format variants exist.

| Files on disk | Logical images |
|---------------|----------------|
| `kernel.elf`, `kernel.bin` | 1 × `kernel` |
| `v1.elf`, `v1.bin` | 1 × `v1` |
| `v1.elf`, `v1.bin`, `v2.elf`, `v2.bin` | 2 × `v1`, `v2` → ambiguous, must specify path |
| `kernel.elf` only | 1 × `kernel` |

When starting emulation, prefer the `.elf` variant if present (multiboot metadata); otherwise use `.bin`.

Exclude non-runnable suffixes (`.o`, `.log`, etc.) and do not scan subdirectories in v1.

**Rationale:** Matches current and anticipated multi-format build outputs without false "multiple images" errors.

### 3. CLI shape

```
run-qemu.sh --headful [--image PATH]
run-qemu.sh --headless [--image PATH]
```

Mutually exclusive mode flags. Neither or both → error.

### 4. Makefile mapping

| Target | Behavior |
|--------|----------|
| `make test` | build if needed, then `--headless` |
| `make run` | build if needed, then `--headful` |

### 5. Headless vs headful behavior

| Behavior | Headless | Headful |
|----------|----------|---------|
| Display | none | window or documented remote viewer fallback |
| Timeout + message assert | yes | no |
| Session end | pass/fail on diagnostic message | user/operator ends session |

### 6. Ambiguity and error messages

When multiple logical images exist, the error SHALL list **stems** (e.g. `kernel`, `v1`, `v2`), not every on-disk filename, so the caller knows which identity to pass via `--image`.

## Risks / Trade-offs

| Risk | Mitigation |
|------|------------|
| Future layout uses subdirs per image | Out of scope for v1; would need a follow-on discovery rule |
| Caller passes `.bin` path but `.elf` also exists | Use the path given; discovery dedup only applies to auto-select |
| Headful flaky under Docker + native X11 | Document remote-viewer fallback inside headful; headless remains agent/CI path |

## Migration Plan

1. Implement CLI contract and stem-grouped discovery in the run script.
2. Update Makefile and README.
3. Re-run `make test` (headless) for regression evidence.

## Open Questions

- Keep script name `run-qemu.sh` vs rename to `run-image.sh` (default: keep unless apply finds it misleading).
