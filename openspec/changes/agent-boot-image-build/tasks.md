## 1. Packaging entry point hygiene

- [ ] 1.1 Add `scripts/build-deploy-artifact.sh` wrapper: remove canonical flash-ready disk image and imaging-tool alias before invoking disk-image packaging; emit `AKOYA_DEPLOY_ARTIFACT=status=...;img=...` summary line on completion
- [ ] 1.2 Update `scripts/build-boot-usb.sh` to delete stale canonical image at start if wrapper does not centralize removal (single code path only—no duplicate deletion logic)
- [ ] 1.3 Verify wrapper runs unprivileged and produces `build/akoya-boot.img` on success

## 2. Apply agent contract

- [ ] 2.1 Update `.cursor/agents/osf-apply-start.md`: on implementation batch completion, run deploy packaging (await background Task if used) before flash-ready handoff; require prominent `## Flash this image` section with absolute path; forbid ready-to-flash language on packaging failure
- [ ] 2.2 Document async pattern: spawn packaging Task after stale removal, await before flash-ready claim; interim updates may say packaging in progress only
- [ ] 2.3 Update `.cursor/skills/osf-apply-changes/SKILL.md`: parent relay must include deploy artifact section from worker debrief verbatim

## 3. Documentation

- [ ] 3.1 Add README subsection: agent apply completion always builds flash image; canonical path; Etcher alias; stale images cleared on rebuild
- [ ] 3.2 Add `AGENTS.md` brief note under apply-complete expectations pointing at `agent-deploy-artifact` behavior

## 4. Validation

- [ ] 4.1 Manual apply simulation: run wrapper after `make build`; confirm stale removal, success summary, and absolute path in output
- [ ] 4.2 Run `npx @fission-ai/openspec@latest validate agent-boot-image-build --type change`

## Explicitly deferred

- Automatic `dd` to physical USB devices
- Re-building deploy media during `osf-apply-finish` merge step
- CI pipeline integration for deploy artifacts
- ISO optical media as the default agent handoff artifact
