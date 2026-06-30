## Context

Today:

- `make build` produces kernel ELF/bin; `make usb` produces `build/akoya-boot.img` (+ `akoya-etcher.img` symlink).
- `osf-apply-start` may finish with a text debrief listing code changes without running packaging.
- Stale `akoya-boot.img` can remain on disk across kernel rebuilds until someone manually re-runs `make usb`.
- Humans use Balena Etcher or `dd` against the `.img` path documented in README.

Human request:

1. Never say "done, ready to review/flash" without also building the flash image.
2. Show the burn path prominently in the debrief.
3. Prefer async build in a subagent so debrief is not blocked; delete old image first.

## Goals / Non-Goals

**Goals:**

- Apply-complete debrief always includes deploy packaging outcome.
- Flash path is the most visible line in success debriefs (above fold, labeled for Etcher/dd).
- Stale disk image removed before new packaging starts.
- Async packaging via Task `shell` or `generalPurpose` subagent; parent polls or receives notification before final "ready to flash" claim—or states "building" with cleared stale path.
- Packaging uses existing USB disk-image entry point (main unikernel, MBR+GRUB), not ISO.

**Non-Goals:**

- Auto-flashing physical USB drives (no `dd` to `/dev/sdX` without explicit human).
- Building on every intermediate commit inside a long apply session—only at batch completion handoff.
- Replacing `make test` or verification gates.
- CI service integration; workstation-local only.
- Blocking merge/archive on packaging success (packaging failure should fail the debrief's "ready to flash" claim, not silently skip).

## Decisions

### D1 — Trigger: apply batch completion handoff

**Choice:** Run deploy packaging when `osf-apply-start` completes implementation and prepares its human debrief (before delegating finish). Parent `/osf-apply-changes` relay includes packaging section.

**Alternatives:** Only on `osf-apply-finish` — rejected; human reviews before merge and needs image then.

### D2 — Artifact: USB disk image, not ISO

**Choice:** Canonical flash artifact is the legacy-BIOS `.img` from disk-image packaging (Etcher symlink included in debrief).

**Alternatives:** ISO — wrong for legacy BIOS USB per project docs.

### D3 — Stale removal timing

**Choice:** At packaging start, delete `akoya-boot.img`, `akoya-etcher.img` symlink, and any well-known prior-path aliases in build output. If packaging fails, no restorable old image (intentional—forces awareness).

**Alternatives:** Rename to `.old` — rejected; human might still pick wrong file in file picker.

### D4 — Async packaging flow

```
Parent (apply worker)
  1. Delete stale .img
  2. Spawn background Task: make usb (or packaging script)
  3. Continue verification / task checkoffs / debrief draft
  4. Await background Task OR poll for img + success marker
  5. Debrief: FLASH PATH (success) | BUILD FAILED (failure) | BUILD IN PROGRESS (only if human asked early—default await before "ready to flash")
```

**Choice:** Default **await** background packaging before claiming "ready to flash"; async is for not blocking *unrelated* wrap-up work (validation notes, task markdown), not for skipping the await on the flash-ready claim.

**Alternatives:** Fire-and-forget debrief — rejected per user intent (must not say ready without build).

### D5 — Debrief presentation

**Choice:** Dedicated markdown section at top of apply debrief:

```
## Flash this image
`/abs/path/to/build/akoya-boot.img`
(Etcher: `.../akoya-etcher.img`)
```

Machine-parseable line: `AKOYA_DEPLOY_ARTIFACT=status=success;img=...`

### D6 — Implementation surface

**Choice:**

- Add `scripts/build-deploy-artifact.sh` wrapper: stale removal + invoke disk-image packaging + emit `AKOYA_DEPLOY_ARTIFACT=...`
- Update `osf-apply-start.md` agent instructions
- Update `osf-apply-changes/SKILL.md` relay expectations
- Optional `AGENTS.md` one-liner under apply-complete

## Risks / Trade-offs

| Risk | Mitigation |
|------|------------|
| Packaging slow (~minutes) | Async Task; debrief awaits before flash-ready claim |
| Packaging deps missing on workstation | Fail debrief section; do not claim ready to flash |
| Agent skips step | Spec + agent prompt hard requirement |
| Deleting stale image mid-Etcher | Rare; human shouldn't flash while agent rebuilds |

## Migration Plan

1. Add wrapper script + structured outcome line.
2. Update apply-start agent contract.
3. Update apply-changes skill relay.
4. Document in README under agent workflow (task in apply change).

## Open Questions

- Whether `osf-apply-finish` should re-run packaging after merge — **default no** (image built at review time; merge doesn't change bits if already built on branch).
