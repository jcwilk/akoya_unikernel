---
name: openspec-flow-install
description: Install or upgrade the OpenSpec Flow Cursor bundle from a reference repository into a target project. Compares OPENSPEC_FLOW_VERSION, copies or merges .cursor skills/agents and root docs, and reports drift. Use when the user says /openspec-flow-install, wants to sync OSF tooling from a reference repo, or needs to detect an out-of-date bundle.
disable-model-invocation: true
---

# `/openspec-flow-install` — sync OpenSpec Flow into a target project

**Goal:** Bring the **OpenSpec Flow (OSF)** integration (versioned in **`OPENSPEC_FLOW.md`**) from a **reference repository** into a **target** repository: first-time install, upgrade, or drift check.

**Prefer an agentic workflow:** read versions, list differences, apply file updates with human-visible summary. Optional shell helpers are listed at the end; use them only when they reduce copy error.

## Definitions

- **Reference repo:** Source of truth for the bundle — usually **this repository** holding **`OPENSPEC_FLOW_VERSION`**, **`AGENTS.md`**, **`OPENSPEC_FLOW.md`**, **`CHANGELOG.md`**, **`.cursor/skills/osf-*/`**, **`osf-propose/reference/`**, **`persist/`**, **`openspec-flow-install/`**, and **`.cursor/agents/osf-*.md`**.
- **Target repo:** Project that should gain or update OSF. Must have `.cursor/skills/` and `.cursor/agents/` (create if missing).

## Before changing anything

1. **Resolve paths** — Record absolute paths for reference root and target root.
2. **Read reference version** — From reference **`OPENSPEC_FLOW.md`**, parse YAML front matter for **`OPENSPEC_FLOW_VERSION`** (semver string).
3. **Read target version** — If target **`OPENSPEC_FLOW.md` exists**, parse its **`OPENSPEC_FLOW_VERSION`**. If missing, treat target as **uninstalled** (version ∅).
4. **Compare semver** — If target version **equals** reference and files appear present, **verify** Representative files match reference (sizes or checksums optional). If mismatch or older semver → **upgrade path**. If target **newer** than reference → **warn** (“target ahead of reference”) and stop unless the human asks to downgrade.
5. **Inventory** — List reference paths that constitute the bundle:

   ```
   OPENSPEC_FLOW.md
   AGENTS.md
   CHANGELOG.md                 # human-readable bundle history; optional but recommended
   README.md                    # optional to copy; useful for onboarding
   .cursor/skills/osf-explore/
   .cursor/skills/osf-explain/
   .cursor/skills/osf-propose/   # include reference/concepts.md
   .cursor/skills/osf-apply-changes/
   .cursor/skills/persist/
   .cursor/skills/openspec-flow-install/
   .cursor/agents/osf-apply-start.md
   .cursor/agents/osf-apply-finish.md
   .cursor/agents/osf-apply-abort.md
   ```

   Do **not** copy unrelated `.cursor/` entries from reference if the reference repo ever grows beyond OSF.

   Remove stale **`.cursor/build-local-tools.sh`** on upgrade if present.

## Install (target has no OSF or no version file)

1. Create missing **`.cursor/skills`** / **`.cursor/agents`** on target.
2. **Copy** (merge = replace-with-reference for these paths): each bundle path above → same relative path under target.
3. **`AGENTS.md`:**  
   - If target has **no** `AGENTS.md`, copy reference wholesale.  
   - If target **has** its own policies, **merge**: insert OpenSpec/OS sections from reference (workflow + Task discipline) without deleting unrelated sections; preserve project-specific bullets.
4. **`README.md`:** Optionally merge a short pointer to OSF; do not wipe an existing README.
5. Confirm target **`OPENSPEC_FLOW.md`** front matter includes **`OPENSPEC_FLOW_VERSION`** matching reference.

## Upgrade (target already has OSF)

1. **Backup** — Suggest `git status` clean or commit WIP; optionally duplicate modified skills to a branch.
2. **Replace** bundle files with reference copies for the inventory list (same relpaths). If the human customized a skill, **diff first** — merge customizations explicitly rather than silently overwriting (stop and ask if unclear).
3. **Bump recorded version:** Target **`OPENSPEC_FLOW_VERSION`** MUST match reference after upgrade.
4. **`concepts.md`:** Prefer reference copy; If the human refreshed concepts from upstream independently, reconcile with their consent.

## Drift-only check

If user asked “are we current?”:

- Compare **`OPENSPEC_FLOW_VERSION`**.
- Optionally diff **checksums or `diff -ru`** bundle directories.
- Report: current / behind / divergent (manual edits vs reference).

## After sync

1. Tell the human the **prior** vs **new** semver and list **files touched**.
2. Remind them: OpenSpec **`openspec/`** tree still lives **in each project**; use upstream **`npx @fission-ai/openspec@latest`** to init/validate there.
3. Run **`git status`** in target; no surprise deletions.

## Optional shell helpers (operator-run or agent-run)

Idempotent **rsync** from reference to target (review flags for your OS):

```bash
REF="/absolute/path/to/openspec-flow-reference"
TGT="/absolute/path/to/target/project"
rsync -a --delete "$REF/.cursor/skills/osf-explore/" "$TGT/.cursor/skills/osf-explore/"
rsync -a --delete "$REF/.cursor/skills/osf-explain/" "$TGT/.cursor/skills/osf-explain/"
rsync -a --delete "$REF/.cursor/skills/osf-propose/" "$TGT/.cursor/skills/osf-propose/"
rsync -a --delete "$REF/.cursor/skills/osf-apply-changes/" "$TGT/.cursor/skills/osf-apply-changes/"
rsync -a --delete "$REF/.cursor/skills/persist/" "$TGT/.cursor/skills/persist/"
rsync -a --delete "$REF/.cursor/skills/openspec-flow-install/" "$TGT/.cursor/skills/openspec-flow-install/"
rm -f "$TGT/.cursor/build-local-tools.sh"
install -m644 "$REF/.cursor/agents/osf-apply-"*.md "$TGT/.cursor/agents/"
install -m644 "$REF/OPENSPEC_FLOW.md" "$TGT/OPENSPEC_FLOW.md"
install -m644 "$REF/CHANGELOG.md" "$TGT/CHANGELOG.md"
# AGENTS.md and README.md: merge manually or copy if absent
```

**Do not treat rsync `--delete`** as mandatory if the human maintains extra files beside SKILL.md inside a bundle directory; prefer explicit file lists when uncertain.

## Reference

- **`OPENSPEC_FLOW.md`** — bundle version key.
- **`.cursor/skills/osf-propose/SKILL.md`** — what “installed” OSF is expected to unlock.
