## Why

Agents routinely tell humans that implementation is complete and ready to review or flash, but the flash-ready USB disk image is often stale or missing. Operators can burn an old `build/` artifact while a new kernel was only compiled to ELF form, or flash mid-rebuild and brick confidence. Completion handoffs need a mandatory, visible deploy artifact—not an optional follow-up.

## What Changes

- Require agents completing an approved implementation batch to produce a fresh legacy-BIOS USB disk image (main unikernel payload) before claiming the work is ready to flash.
- Remove or invalidate the prior flash-ready disk image at the start of each new packaging run so superseded media cannot be selected accidentally.
- Prominently surface the absolute path to the flash-ready artifact in every human-facing completion debrief when packaging succeeds.
- Allow packaging to run asynchronously in a background worker so the parent agent can prepare its debrief without blocking on full image creation, while still clearing stale artifacts first and reporting in-progress or failed packaging state honestly.
- Integrate the contract into OpenSpec apply worker debriefs (`osf-apply-start` completion path).

## Capabilities

### New Capabilities

- `agent-deploy-artifact`: Agent completion contract for mandatory deploy-media build, stale-artifact hygiene, prominent flash-path presentation, and optional async packaging.

### Modified Capabilities

- `boot-media-packaging`: Canonical flash-ready artifact identity and invalidation semantics when a rebuild is requested.

## Impact

- OpenSpec apply agents and orchestration skills (`.cursor/agents/osf-apply-*.md`, `.cursor/skills/osf-apply-changes/SKILL.md`)
- `AGENTS.md` operational note for apply-complete handoffs (via apply tasks, not propose lane)
- Packaging entry point behavior for pre-build stale removal (likely `scripts/build-boot-usb.sh` or thin wrapper)
- Human/agent debrief templates in apply workers
- No change to deployment-target hardware requirements or guest runtime behavior
