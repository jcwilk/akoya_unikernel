# Changelog

All notable changes to this **OpenSpec Flow reference bundle** (docs under this repo root plus `.cursor/skills/osf-*`, `.cursor/agents/osf-*`, and related companion skills documented in `OPENSPEC_FLOW.md`) are recorded here.

The **canonical bundle version** for install/upgrade checks is **`OPENSPEC_FLOW_VERSION`** in the YAML front matter of **`OPENSPEC_FLOW.md`**. This file is a human-readable history; when you cut a release, **bump `OPENSPEC_FLOW_VERSION` and add an entry below** so consumers can compare versions.

## [1.3.0] — 2026-06-23

### Changed

- **Apply simplification** — **`osf-apply-changes`** and **`osf-apply-start`** assume the **current branch**; OSF apply no longer prescribes branch/worktree creation, parallel multi-change orchestration, or duplicated worker-ownership lists. Finish/abort agents use **working branch** terminology.
- **`/osf-propose` lane lock** — upfront writable-scope section: only the active change folder is writable during proposal shaping; bundle/integration edits belong in approved **`tasks.md`** and apply.
- **`OPENSPEC_FLOW.md`** — **Forbidden lane transitions** subsection; capability table and standard flow updated for current-branch apply.

### Notes for consumers

Upgrading from 1.2.x: humans must choose branch/worktree **before** invoking apply; OSF skills no longer create `apply/<name>` branches or parallel apply lanes.

## [1.2.0] — 2026-05-31

### Changed

- **Operational completeness** — apply orchestration must not soften approved **`tasks.md`** scope; **`osf-apply-start`** classifies tasks (implementation, build/release, environment acceptance, tooling-only) with evidence rules and aborts when required environment work cannot run; **`osf-apply-finish`** gates archive on ops evidence, not checkboxes alone.
- **`/osf-propose`** — **`tasks.md`** discipline: ops tasks default unchecked; required vs **Explicitly deferred** structure; pre-check only with same-turn human attestation.
- **`/osf-explain`** — footer skim order **Ambiguities** → **Apply scope at shipping** → **Quick read** → minimal **Decide**; fast-pass reading order updated.
- **`OPENSPEC_FLOW.md`** / **`AGENTS.md`** — **apply-complete** vs **merge-complete** vocabulary.

### Notes for consumers

Upgrading from 1.1.x changes **behavioral expectations** for apply: verify-existing-work and narrow Task prompts no longer excuse skipping release or live acceptance tasks. Review in-flight changes for pre-checked ops rows and “optional follow-up” section titles.

## [1.1.1] — 2026-05-30

### Changed

- **`scripts/build-local-tools.sh`** — moved from `.cursor/build-local-tools.sh`; install skill and docs updated.

## [1.1.0] — 2026-05-30

### Added

- **Conversation sleuths** — skill **`/sleuths`**, Rust **`sleuth`** CLI (`.cursor/skills/sleuths/`), **`scripts/build-local-tools.sh`**, **`AGENTS.md`** guidance, gitignore for **`.sleuths/`** and build artifacts.

## [1.0.1] — 2026-05-28

### Removed

- **`spawn-subagent`** skill — Task delegation rules remain in **`osf-apply-changes`**, **`AGENTS.md`**, and **`OPENSPEC_FLOW.md`**; Cursor subagents no longer need a separate skill.

## [1.0.0] — 2026-05-10

### Added

- Initial **OpenSpec Flow** reference bundle for Cursor: `OPENSPEC_FLOW.md`, generalized **`AGENTS.md`**, skills `osf-explore`, `osf-propose`, `osf-explain`, `osf-apply-changes`, `openspec-flow-install`, Task agents `osf-apply-start`, `osf-apply-finish`, `osf-apply-abort`, plus companion `persist` as documented in `OPENSPEC_FLOW.md`.
- **`CHANGELOG.md`** to track bundle changes alongside **`OPENSPEC_FLOW_VERSION`**.
