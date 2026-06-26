---
OPENSPEC_FLOW_VERSION: "1.3.0"
OPENSPEC_CLI_PACKAGE: "@fission-ai/openspec"
description: |
  Human-facing overview plus machine-readable bundle version for the OpenSpec Flow
  Cursor integration (skills, agents, and workflow docs) shipped in this repository.
---

# OpenSpec Flow

This repository distributes a **reference OpenSpec Flow (OSF)** bundle for **Cursor**: agentic development with explicit human review of intent before reconcile. The human owns intent and acceptance; agents plan, investigate, implement, and verify through durable **`openspec/`** artifacts rather than unstructured prompting.

OpenSpec ([Fission-AI/OpenSpec](https://github.com/Fission-AI/OpenSpec)) is the spec and change-management layer. Cursor skills and Task subagents wrap it so reviewers invoke high-level flows without memorizing CLI flags.

**Bundle version:** **`OPENSPEC_FLOW_VERSION`** in the YAML front matter above identifies the **entire integration** (docs + `.cursor/skills/osf-*` + `.cursor/agents/osf-*` + companion **`persist`**). Consumers should compare it to their own recorded version (often copied into **`OPENSPEC_FLOW.md`**) before upgrading; see **`/openspec-flow-install`**.

## Prerequisites

- **Node.js** 20.19+ to run the OpenSpec CLI.
- **Cursor** (recommended), with **Task subagents** for **`osf-apply-*`**.
- **Git** for version control; branch and worktree choice is a **human precondition** before apply—not something OSF apply skills create.

```bash
npm install -g @fission-ai/openspec@latest   # optional; npx works without global install
```

## Two layers of work

| Layer | Where | Purpose |
|--------|--------|---------|
| **Living specs** | `openspec/specs/<domain>/spec.md` | The current accepted agreement about how the system behaves. Source of truth after a change is archived. |
| **Changes** | `openspec/changes/<change-name>/` | Proposed or in-progress work: proposal, design, delta specs, tasks. Stays separate until archive. |

Coordination happens through Cursor, conversation, and git branches or worktrees. When something needs human judgment, workers stop and surface context via **`/osf-apply-abort`**; they do not silently rewrite intent.

## What a spec actually is

This section most determines whether agent output stays reviewable. The full upstream guidance is OpenSpec's [Concepts → Specs](https://github.com/Fission-AI/OpenSpec/blob/main/docs/concepts.md#specs); the vendored mirror is **`.cursor/skills/osf-propose/reference/concepts.md`**.

### A spec is a behavior contract, not an implementation plan

Good spec content is what an outside observer can verify: observable behavior, inputs/outputs, error conditions, and external constraints.

**Avoid in `spec.md` files** (put these in **`design.md`** or **`tasks.md`** instead):

- internal symbols, scripts, or file paths,
- framework or dependency choices framed as obligations,
- specific shell commands unless they are externally visible behavioral contracts,
- low-level schema key names tied to today’s configs,
- step-by-step implementation plans,
- contributor workflow rules (“doing X MUST NOT require a change”) — those belong in **`AGENTS.md`**,
- duplicating catalogs or runbooks **`README`** or **`AGENTS.md`** already maintain — link instead.

**Quick test**: if implementation can change without changing externally visible behavior, the detail does not belong in the spec.

### One capability per spec

Specs are organized by domain. If one `openspec/specs/<domain>/spec.md` mixes several independently testable capabilities, split siblings in a later change rather than cramming.

### Use RFC 2119 keywords deliberately

| Keyword | Meaning |
|---------|---------|
| **MUST / SHALL** | absolute requirement |
| **MUST NOT / SHALL NOT** | absolute prohibition |
| **SHOULD** | recommended; exceptions need justification |
| **MAY** | optional |

### Lite by default

Most changes stay **Lite**: short behavior-first requirements, explicit non-goals, a few scenarios. Reach for heavier rigor for cross-cutting or high-ambiguity work.

### Delta specs (ADDED / MODIFIED / REMOVED)

In a change folder, `specs/<domain>/spec.md` is a **delta**:

- **`## ADDED Requirements`** — appended on archive.
- **`## MODIFIED Requirements`** — replace the named requirement on archive.
- **`## REMOVED Requirements`** — delete on archive.

Each requirement uses `### Requirement: <Name>` and at least one `#### Scenario:` with `GIVEN` / `WHEN` / `THEN`. **`/osf-apply-finish`** runs archive on the working branch so merges into living specs stay atomic with delivery.

## Cursor capabilities in this bundle

| Slash / entry | Kind | Role |
|---------------|------|------|
| **`/osf-explore`** | Skill | Read-only thinking partner; no implementation. |
| **`/osf-propose`** | Skill | Create or refine a change under `openspec/changes/<name>/`, validate, persist. |
| **`/osf-explain`** | Skill | Structured human review summary of a change; end-of-debrief skim: **Ambiguities** → **Apply scope at shipping** → **Quick read**. |
| **`/osf-apply-changes`** | Skill | Spawns **`osf-apply-start`** (Task-only) workers. |
| **`/osf-apply-start`** | Subagent | Implements one approved change on the **current branch** (working branch). |
| **`/osf-apply-finish`** | Subagent | Verify, archive, merge default branch, push. |
| **`/osf-apply-abort`** | Subagent | Stop safely; preserve investigation; debrief human. |

**`osf-apply-*`** MUST run via the Task tool; do not replay **`.cursor/agents/osf-apply-*.md`** in the parent thread (see **`osf-apply-changes`** and **`AGENTS.md`**).

## Standard flow

1. **Explore (optional):** **`/osf-explore`** to clarify intent without coding.
2. **Shape:** **`/osf-propose`** captures `proposal.md`, `design.md`, deltas, `tasks.md`.
3. **Apply:** **`/osf-apply-changes`** → **`osf-apply-start`** on the **current branch** (branch/worktree already chosen by the human).
4. **Finish:** **`osf-apply-finish`** archives, reconciles **`openspec/specs/`**, merges, pushes — or **`osf-apply-abort`** when intent must be revised.

**Success criterion:** a change is **apply-complete** only when every non-deferred **`tasks.md`** row has class-appropriate evidence (or an authorized override) and finish verification passes—not merely when the default branch has merged.

## Forbidden lane transitions

Slash commands are **lanes** with distinct writable scope. Crossing lanes without an approved apply run violates OSF discipline.

| Transition | Allowed? |
|------------|----------|
| **`/osf-propose`** → direct edits outside the active `openspec/changes/<name>/` folder (skills, agents, bundle docs, application code) | ❌ Record targets in change artifacts; implement via apply after review |
| **`/osf-propose`** → artifacts under `openspec/changes/<name>/` only | ✅ |
| Bundle/integration paths → edit after human approval via **`tasks.md`** in **`/osf-apply-changes`** | ✅ |

## Apply-complete vs merge-complete

| Term | Meaning |
|------|---------|
| **Merge-complete** | Archive ran, living specs reconciled, working branch merged into the default branch (and pushed when agreed). |
| **Apply-complete** | Merge-complete **and** every non-deferred task—including **build/release artifact** and **environment acceptance** work—has evidence in the apply/finish handoff or an explicit same-message human override; otherwise the apply unit should have **aborted** instead of finishing. |

Repository hygiene (checked boxes, validate, merge) can succeed while operational delivery is still missing. OSF treats that gap as a failure mode: orchestrators must not soften Task prompts, workers must not substitute weaker checks, and finish must not trust checkboxes alone for ops task classes.

## Blocked flow

If execution cannot honestly match approved intent, the worker invokes **`osf-apply-abort`**: blocker debrief, no silent edits inside the change folder, no merge of incompatible work onto the default branch. Revise intent with **`/osf-propose`**, then retry **`/osf-apply-changes`**.

## Further reading

- **`CHANGELOG.md`** — release history for this bundle (pair with **`OPENSPEC_FLOW_VERSION`** bumps).
- [OpenSpec — Concepts](https://github.com/Fission-AI/OpenSpec/blob/main/docs/concepts.md) (canonical).
- [OpenSpec — Getting Started](https://github.com/Fission-AI/OpenSpec/blob/main/docs/getting-started.md).
- **`.cursor/skills/osf-propose/reference/concepts.md`** — offline mirror with refresh recipe.
- [Beyond 2x — Keeping Agents in Line without Reading Every Line](https://jcwilk.com/beyond-2x-keeping-agents-in-line-without-reading-every-line/).
- **`AGENTS.md`** — repository-specific safety and `openspec/` discipline for this bundle or your project.
