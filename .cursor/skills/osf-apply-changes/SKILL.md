---
name: osf-apply-changes
description: Start implementation for one approved OpenSpec change on the current branch, using Task subagents. Use when the user says `/osf-apply-changes` or is ready to execute after `/osf-propose`. Orchestrates `osf-apply-start`, `osf-apply-finish`, and `osf-apply-abort`; does not duplicate per-change apply mechanics.
disable-model-invocation: true
---

# `/osf-apply-changes` — implementation orchestration

This skill is **just orchestration**. Branch/worktree choice and concurrency are **human preconditions**—apply runs on the **current branch**. Per-change implementation (work queue, validation, finish/abort delegation) lives entirely inside the **`osf-apply-start`** agent.

## Non-negotiable: Task-only subagents

- **`osf-apply-start` MUST be invoked only via the Task tool** (`subagent_type: osf-apply-start`). Never replay **`.cursor/agents/osf-apply-start.md`** in the parent thread.
- After spawning **`osf-apply-start`**, **wait until that Task completes** before treating the apply as done or re-implementing the same change in the parent. On failure or abort, the path is **`osf-apply-abort`** / **`/osf-propose`**—not unilateral parent completion.
- One change name → one worker until it returns. No parallel implementation of the same **`openspec/changes/<name>/`** in the parent thread.

## Task prompt contract

When constructing the Task prompt for **`osf-apply-start`**, treat approved **`tasks.md`** as the floor:

- **MAY add** constraints: working branch name (for debrief), validation commands, safety boundaries from **`AGENTS.md`**, environment allowlists.
- **MUST NOT subtract, downgrade, or waive** any `- [ ]` row unless the human explicitly opts out in the **same directive**.

| Forbidden (softens approved work) | Allowed (adds constraints only) |
|---|---|
| "Local smoke is enough; skip staging." | "Run task 4.2 against staging URL; credentials in `.env`." |
| "Checkbox the rest and call finish." | "Complete all sections 1–5; human waived section 6 in this message." |
| "Skip environment verification if blocked." | "If staging unreachable, **abort**—do not finish." |

## Single change procedure

1. Confirm the change name and that intent is approved under `openspec/changes/<name>/`.
2. Confirm the human has chosen branch/worktree context (default: **current branch**).
3. Spawn **one** Task with `subagent_type: osf-apply-start`. Prompt must be **self-contained**: repository root, change name, working branch, constraints from **`AGENTS.md`**, the **Task prompt contract** above, and instruction not to create branches/worktrees or spawn parallel apply lanes.
4. **Wait** for the Task; relay its debrief to the human verbatim.

## Finish and abort outcomes

| Outcome | Subagent |
|---------|----------|
| Tasks complete and verified | **`osf-apply-finish`** — verifies, archives on the working branch, merges into `main`, pushes |
| Cannot continue as written | **`osf-apply-abort`** — rolls back, checks out `main`, returns debrief; does **not** edit the change folder |

After abort, intent fixes happen only through **`/osf-propose`**, then a fresh **`/osf-apply-changes`** run.

## Reference

- Flow: **`OPENSPEC_FLOW.md`**. Discipline: **`AGENTS.md`**. Worker: **`.cursor/agents/osf-apply-start.md`**. Terminal: **`.cursor/agents/osf-apply-finish.md`**, **`.cursor/agents/osf-apply-abort.md`**.
