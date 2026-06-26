---
name: osf-propose
description: Shape, create, or refine an OpenSpec change. Use when the user says `/osf-propose`, wants a new change, revises an existing one under `openspec/changes/`, or critiques whether a breakdown still matches intent. Entry point aligned with `OPENSPEC_FLOW.md`. Finishes by persisting the change artifacts via `.cursor/skills/persist/SKILL.md` unless the user opts out.
disable-model-invocation: true
---

# `/osf-propose` — shape the change artifact

This skill is the **intent lane**: durable artifacts under `openspec/changes/<name>/` are the human-facing review surface. **`/osf-explore`** is the upstream "think first" companion; **`/osf-apply-changes`** is the downstream executor; **`/osf-apply-finish`** archives back into living specs at the end.

> **The most common failure of this skill is producing artifacts that pass `openspec validate` but bake implementation details, scripts, file paths, and YAML field names into `spec.md`. That is the mistake to actively refuse — it costs more than missing artifacts. Roughly half of this file is therefore about **spec quality**, not mechanics. Read it as such.**

## Lane lock

**Writable this turn:** only `openspec/changes/<name>/` for the active change you are shaping.

**Read-only for orientation:** everything else — living specs (`openspec/specs/`), skills (`.cursor/skills/**`), agents (`.cursor/agents/**`), bundle docs (`OPENSPEC_FLOW.md`, `AGENTS.md`, `CHANGELOG.md`), and application code. You may read these to inform proposal artifacts; do not edit them.

**Healthy end state:** validated change folder artifacts + `/osf-explain` debrief — **not** edited skills, agents, or bundle documentation. If your first planned edit is outside the active change folder, **stop** and record bundle/integration targets in `proposal.md`, `design.md`, delta specs, and `tasks.md` for human review; implementation happens via **`/osf-apply-changes`** after approval.

## Required reading before drafting

Before producing any artifact this turn, read these in order. **If you are about to draft a `spec.md` (delta or otherwise) and have not read these, stop and read them first.**

1. **`OPENSPEC_FLOW.md` → "What a spec actually is"** — the repo-local summary of OpenSpec best practices and the vocabulary the human will use to review your work.
2. **`./reference/concepts.md`** — vendored copy of the upstream OpenSpec [Concepts](https://github.com/Fission-AI/OpenSpec/blob/main/docs/concepts.md) doc. Required sections: **`## Specs`** (especially `### What a Spec Is (and Is Not)` and `### Keep It Lightweight: Progressive Rigor`) and **`## Delta Specs`**.
3. **The relevant `openspec/specs/<domain>/spec.md`** for any domain you are about to touch — living truth before any delta.
4. **Active siblings under `openspec/changes/`** — to avoid colliding with in-flight work.

### Refresh upstream concepts when uncertain

The vendored `./reference/concepts.md` is a snapshot. **If you suspect it is stale or you are uncertain about correct spec form/content, refresh it from upstream before drafting.**

```bash
# From repo root:
curl -fsSL \
  https://raw.githubusercontent.com/Fission-AI/OpenSpec/main/docs/concepts.md \
  -o .cursor/skills/osf-propose/reference/concepts.md
```

(Or fetch via the `WebFetch` tool against the same URL and re-save.) After refreshing, restore the leading HTML-comment header so future agents still see the source URL, capture date, and refresh recipe. **Doing this whenever uncertainty exists is encouraged, not just when something is obviously wrong.**

## Mandatory routing

If the user says **`/osf-propose`**, attaches this skill, or otherwise indicates this skill should run: whatever they asked for **must be handled through an OpenSpec change** (`openspec/changes/<name>/`, new or existing) — not by jumping straight to the outcome.

- **Do not** implement the request directly (deleting paths under **`openspec/specs/`**, rewriting living **`spec.md`** files, or editing application code) from this skill's turn. Route into proposal / design / delta specs / `tasks.md` so a human can review intent first; **`/osf-apply-changes`** then executes `tasks.md` and **`/osf-apply-finish`** archives, reconciling **`openspec/specs/`**.
- **Do** create and revise files under `openspec/changes/<name>/` as the OpenSpec CLI directs (see Workflow below). That is this skill's lane; it is **not** a license to hand-edit `openspec/specs/`.
- **Retirements, removals, "cleanup"** are still OpenSpec changes — express them as `## REMOVED Requirements` in delta specs and tasks, not as ad-hoc filesystem edits.
- **Reading** `openspec/specs/` and `openspec/changes/` for context is fine; mutating `openspec/specs/` here is not. Follow `AGENTS.md` (`openspec/` is workflow-driven).
- **Bypass** only if the human explicitly asks to skip OpenSpec for a one-off direct edit. Say in chat that you are bypassing `/osf-propose` routing, then follow their instruction. The persist close-out below applies only to artifacts produced in this skill, not arbitrary direct edits.

## Hard stop: propose is not apply

**`/osf-propose` ends in a reviewable change under `openspec/changes/<name>/`. It does not land behavior on `main` or reconcile `openspec/specs/` unless the human explicitly orders a bypass or a combined “propose + apply” in one turn.**

Unless the human **clearly** asks you to archive, finish, merge, push, or “just do it all” in the **same** message, you **must not**:

- Run **`npx @fission-ai/openspec@latest archive …`** (or any archive flow that moves the change into `openspec/changes/archive/` and merges deltas into **`openspec/specs/**`).
- Edit, create, or delete paths under **`openspec/specs/`** (living specs merge **only** through documented archive/finish workflow per **`AGENTS.md`**).
- Treat **`tasks.md` “archive” steps** as instructions to execute **during this skill**—those steps are for **`/osf-apply-changes`** / **`osf-apply-start`** / **`osf-apply-finish`**, not for the propose lane.
- Fold implementation (code, scripts, config) **and** spec reconciliation into the same turn as propose when the human only asked for **`/osf-propose`** or “shape / propose” spec work—“**otherwise fix the rest**” means **non-spec** paths only; it is **not** permission to archive or edit **`openspec/specs/`** without an explicit apply/finish request.

**Healthy end state for `/osf-propose`:** validated artifacts only under **`openspec/changes/<name>/`**, **`openspec validate "<name>" --type change`** passes, **`openspec list`** still shows the change as active (not only under **`archive/`**), debrief points the human to **`/osf-apply-changes`** when they are ready.

If you are about to run `archive` or touch **`openspec/specs/`** and the user did not ask for apply/finish, **stop** and confirm in chat.

## Spec quality bar (half the work)

A change that passes `openspec validate` but breaks these rules is **not done**. These are the rules a human will judge your output by.

### A spec is a behavior contract, not an implementation plan

Good `spec.md` content (living spec or delta):

- observable behavior users or downstream systems can rely on,
- inputs, outputs, error conditions,
- external constraints — security, privacy, reliability, compatibility,
- scenarios that can be tested or explicitly validated.

**Stay out of `spec.md`** (these belong in the change's `design.md` or `tasks.md`):

- internal class, function, module, or script names (`scripts/foo.py`, `lib/bar.ts`),
- library or framework choices,
- specific commands a tool happens to invoke (`lsblk`, `df`, `curl`, …),
- specific YAML/JSON field keys or other on-disk schema spelling,
- step-by-step implementation details,
- workflow or authoring rules dressed as system requirements ("doing X SHALL NOT require an OpenSpec change") — those belong in `AGENTS.md`,
- self-referential requirements about how the spec file itself is allowed to look.

**The quick test** (apply to every requirement and every scenario before writing it): *if implementation can change without changing externally visible behavior, the detail does not belong in the spec.* If a rename of a script or a swap of a Linux command would force a `MODIFIED` requirement with no behavioral consequence, you have leaked implementation into the spec.

### One capability per spec, one bounded context per delta file

Specs are organized by domain — feature area, component, or bounded context. Before adding a new requirement, ask: *does this requirement describe the same domain as the rest of this `spec.md`?* If not, propose splitting into a sibling spec (`openspec/specs/<other-domain>/`) rather than cramming. Cohesion matters more than colocation; a spec describing two or three independently testable capabilities is a smell to refuse.

### Default to Lite

Most changes should stay **Lite**: short behavior-first requirements, clear scope and non-goals in `proposal.md`, a few concrete acceptance scenarios per requirement. Reach for **Full** rigor only for cross-cutting changes, contract/migration work, or work where ambiguity is likely to cause expensive rework. **Long specs are not better specs.** When in doubt, write less.

### Every requirement, run this checklist before saving

- [ ] States behavior, not mechanism (no script names, command names, library names, schema keys).
- [ ] Uses RFC 2119 keywords deliberately (`MUST`/`SHALL` for absolute, `SHOULD` for genuine recommendation, `MAY` for optional).
- [ ] Has at least one `#### Scenario:` with `GIVEN` / `WHEN` / `THEN` bullets.
- [ ] The scenario is testable — an outside observer could write a check for it.
- [ ] The requirement belongs to the domain of this `spec.md`, not a sibling.
- [ ] Passes the quick test: changing implementation alone would not require editing this requirement.

### Drift to actively refuse

If the human's request implies any of these, push back in chat and propose a behavior-shaped alternative *before* writing files:

- naming a specific script, file path, command, or library inside `spec.md`,
- naming specific YAML/JSON field keys inside `spec.md`,
- workflow/authoring rules ("editing X SHALL NOT require an OpenSpec change") in `spec.md`,
- requirements about how `openspec/specs/<...>/spec.md` itself should look,
- a single delta touching multiple unrelated bounded contexts in one capability,
- "cleanup" framed as bypassing the OpenSpec flow (it isn't; cleanup goes through `## REMOVED Requirements`).

### Delta sections

Inside a change, `specs/<domain>/spec.md` is a **delta**, never a full spec. Use the canonical headings:

- **`## ADDED Requirements`** — new behavior; appended on archive.
- **`## MODIFIED Requirements`** — replacement behavior for an existing requirement; replaces on archive. Note the previous wording in parentheses for reviewer context.
- **`## REMOVED Requirements`** — deprecated behavior; deleted on archive. Note why.

Under each section, the same `### Requirement:` + `#### Scenario:` shape as a living spec. **`/osf-apply-finish`** is what merges these into `openspec/specs/` at archive time.

## Pre-flight

If the request feels under-specified, offer to drop into **`/osf-explore`** first. Explore is the cheap loop for shaping intent before a proposal exists.

## Workflow (the other half)

### A. New change (full artifact pipeline)

1. **Resolve a name.** If the user did not give a kebab-case change name, derive one from their description (`add user authentication` → `add-user-auth`). If intent is too unclear to name, ask one focused question or suggest **`/osf-explore`** — do not proceed blind.

2. **Scaffold.**

   ```bash
   npx @fission-ai/openspec@latest new change "<name>"
   ```

   Creates `openspec/changes/<name>/` with `.openspec.yaml`.

3. **Build artifacts in dependency order.**

   ```bash
   npx @fission-ai/openspec@latest status --change "<name>" --json
   ```

   Parse `applyRequires` (artifact IDs needed before implementation; typically includes `tasks`) and `artifacts` (status + dependencies).

   Loop while there is a `ready` artifact:

   - Get instructions:

     ```bash
     npx @fission-ai/openspec@latest instructions <artifact-id> --change "<name>" --json
     ```

     The JSON contains `context` (project background — **constraints for you**, do not copy into the file), `rules` (artifact-specific — same), `template` (file structure to fill), `instruction` (schema-specific guidance), `outputPath`, and `dependencies` (read these completed artifacts first).

   - Read each completed dependency artifact for context.
   - Write the artifact at `outputPath` using `template` as the structure. Apply `context` and `rules` as constraints; **do not** copy `<context>`, `<rules>`, or `<project_context>` blocks into the file.
   - When drafting `specs/<domain>/spec.md`, **run the Spec Quality checklist above on every requirement.**
   - Re-run `status --json`; stop when every artifact in `applyRequires` is `done`.

   If a particular artifact requires user input, stop and ask one focused question — prefer reasonable defaults to keep momentum, but never invent constraints when human judgment is needed.

4. **Validate.**

   ```bash
   npx @fission-ai/openspec@latest validate "<name>" --type change
   ```

   Fix any reported issues. **Then re-read every `spec.md` you produced once more against the Spec Quality checklist** — `openspec validate` checks structure, not content quality.

### B. Refine an existing change

1. Read the current files under `openspec/changes/<name>/`.
2. Use `openspec status --change "<name>" --json` and `openspec instructions <artifact-id> --change "<name>" --json` to drive only the artifacts that need updating.
3. Apply the same Spec Quality checklist when editing any `spec.md`.
4. Validate as in A.4.

## `tasks.md` discipline

Operational work (build artifacts, release publication, deploy, live acceptance) defaults to **`- [ ]`** even when implementation already exists on a branch. **Pre-checking** `- [x]` is allowed only when the human **in the same turn** attests a **named environment** is already verified for this change (what ran, against which target).

**Structure:**

- **`## Required for this change`** (or numbered groups **without** “optional” in the title) — in-scope for apply; every row must complete with evidence or the run **aborts**.
- **`## Explicitly deferred`** — out of scope for this apply run; explain and finish debriefs use **deferred by intent**, not “optional follow-up.”
- **Do not** use section titles like “optional follow-up” for in-scope production work.
- Migration narrative stays in **`design.md`**; anything apply must execute appears as checkable tasks.

**Self-check before persist:** if any build, deploy, or environment-acceptance row is `- [x]` without human attestation in the **same turn**, flag it in chat and revert to `- [ ]` before committing artifacts.

## Repository conventions

- **Living specs:** `openspec/specs/`. Changes stay in `openspec/changes/` until archived by **`/osf-apply-finish`**.
- **Agent rules:** `AGENTS.md`. **High-level flow + spec best-practices summary:** `OPENSPEC_FLOW.md`.

## Persist: commit only the new or updated change(s)

After OpenSpec artifacts for this request are complete and validated:

0. **Baseline.** If you might commit this turn, capture `git status` (and staged vs unstaged for any **out-of-scope** paths) early so "exact prior state" for unrelated files is unambiguous.

1. **Read and follow** `.cursor/skills/persist/SKILL.md`.

2. **Scope:** the persist step applies **only** to files that are OpenSpec change artifacts for the change(s) created or revised this turn — typically paths under `openspec/changes/<name>/` for that `<name>` (including `.openspec.yaml`, `proposal.md`, `design.md`, `tasks.md`, and `specs/**` inside that change directory). Anything outside that scope is **out of band** unless the human explicitly widened the task.

   **A pure `/osf-propose` persist MUST NOT stage `openspec/specs/**`, `openspec/changes/archive/**`, or anything under `.cursor/agents/` / application code** — those appear only after an explicit **apply / archive / finish** path. If your `git diff` shows living spec or archive paths after a propose turn, you violated **Hard stop: propose is not apply** above; revert those paths before committing.

3. **Unrelated staged or modified files.** If `git status` shows changes outside that scope:
   - **Negotiate.** Tell the user what you see; confirm whether those paths were meant to ride along. Default: `/osf-propose` does not broaden scope — they stay out of the commit.
   - **Isolate and restore.** Before or after the persist commit (whichever preserves git state correctly), put the repo back so out-of-scope paths match their exact prior state — same working-tree contents, same staged-vs-unstaged split. Use safe mechanics (`git stash` with path scope, selective restore/checkout). **Never** drop the user's unrelated work.
   - If you cannot restore cleanly without ambiguity, **stop and ask**. Do not commit a mix of unrelated files "to be helpful."

4. **Order.** Complete the OpenSpec-only commit (and push, per persist) for the intended change(s); **then** ensure unrelated paths match prior state.

## Debrief — hand off to `/osf-explain`

After artifacts are validated and persisted, render the human-facing closing summary by reading and following **`.cursor/skills/osf-explain/SKILL.md`** with the just-created or refined change as the scope (use the **change-scope template**, not a single-artifact template). **Do not freelance a debrief** — every `/osf-propose` close-out goes through `/osf-explain` so the reviewer sees the same shape every time.

The explain footer (**Ambiguities**, **Apply scope at shipping**, **Quick read**) carries skim and approval context—**do not** instruct agents to echo that footer in **What the human needs to decide**; Decide is action lines only.

If the user explicitly opted out of persist, still run `/osf-explain` against the in-place change folder before ending the turn.

## Handoff to implementation

When artifacts are apply-ready and the human **approves execution**, implementation belongs in **`/osf-apply-changes`** (Task **`osf-apply-start`**, then **`osf-apply-finish`**), **not** in this skill. **Do not** substitute “I’ll validate and archive now to save a round-trip”—that is apply/finish work.

## Reference

- **`OPENSPEC_FLOW.md`** → "What a spec actually is" — the repo-local distillation; read first.
- **`./reference/concepts.md`** — vendored OpenSpec concepts doc (refresh recipe in its header).
- Upstream canonical: <https://github.com/Fission-AI/OpenSpec/blob/main/docs/concepts.md#specs>
- **`AGENTS.md`** — `openspec/` workflow discipline (read-only living specs, archive-only merges).
- **`/osf-explore`** (`.cursor/skills/osf-explore/SKILL.md`) — upstream thinking-partner mode for when intent is fuzzy.
- **`/osf-explain`** (`.cursor/skills/osf-explain/SKILL.md`) — canonical closing-debrief format; called from this skill's persist close-out and available standalone for any change review.
