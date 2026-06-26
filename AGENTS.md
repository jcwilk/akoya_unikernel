# Agents and OpenSpec Flow

This file is the **operational contract** for agents working in a repository that ships or consumes the OpenSpec Flow bundle. Project owners **should extend** it with domain rules (security, environments, release process, naming conventions). **Do not** strip OpenSpec discipline when you extend; add sections, do not contradict **`openspec/`** workflow below.

## OpenSpec-first workflow

**OpenSpec is the control plane for intent** in repos that adopt this flow: goals, behavioral requirements, and how work gets approved are expressed under **`openspec/`** as structured artifacts. Casual chat, ad hoc markdown, and implementation notes are context; when they disagree with **reconciled** living specs under **`openspec/specs/`**, **treat the specs as authoritative** until a human updates them through a proper change and archive.

Work generally moves in this order:

1. **Shape** — Intent and the human review surface live in **`openspec/changes/<name>/`** (for example `proposal.md`, `design.md`, delta specs, `tasks.md`). Entry points: **`/osf-explore`** (read-only thinking) and **`/osf-propose`** (create or refine the change). This stage **does not** reconcile living specs or land on the default branch by itself.
2. **Apply** — **`/osf-apply-changes`** spawns **`osf-apply-start`** workers that implement approved **`tasks.md`** (including edits outside **`openspec/`** when tasks require them). Living specs stay read-only here; course corrections go through **`osf-apply-abort`** and revised **`/osf-propose`**, not silent edits to locked intent.
3. **Reconcile** — **`osf-apply-finish`** runs archive so deltas merge into **`openspec/specs/`**, then merges the execution branch into the **default branch** and pushes when that is the agreed completion. After archive, **`openspec/specs/`** is the durable source of behavioral truth for merged work.

**Apply-complete vs merge-complete:** merge and archive success is **merge-complete**. **Apply-complete** additionally requires every non-deferred **`tasks.md`** row—especially build/release and live-environment acceptance—to have class-appropriate evidence (or an explicit same-message override). If ops work is missing, workers **abort** rather than check boxes and finish; finish fails closed when verification notes lack ops proof. See **`OPENSPEC_FLOW.md`** → “Apply-complete vs merge-complete.”

Higher-level narrative, vocabulary, and slash-command overview: **`OPENSPEC_FLOW.md`**.

## Specs, changes, and review

- **Living specs** in **`openspec/specs/`** describe the system’s **current agreed behavior**. Prefer short Task prompts that point workers at the relevant spec files and **`openspec/changes/`** rather than re-typing requirements in chat.
- **Any change to reconciled spec content**—additions, edits, **removals**, retirements, or reorganizing spec domains—goes through **`openspec/changes/<name>/`** and the archive step. That includes work that feels like “cleanup” or “obvious,” if it would change what **`openspec/specs/`** is allowed to say. **Bypass** only when the human **explicitly** asks for a one-off direct edit (say so in chat).
- **Review bias:** Validate requirement deltas and task outcomes; do not assume humans will read every generated line. If implementation and specs disagree, fix it with a spec change and archive, not unreviewed drift.
- **OpenSpec CLI:** `npx @fission-ai/openspec@latest <command>` when `openspec` is not installed globally. **Node.js ≥ 20.19** is required for the CLI.

## `openspec/` directory discipline

- **Reading** **`openspec/specs/`** and active **`openspec/changes/`** for context is expected.
- **Do not** create, edit, move, or delete paths under **`openspec/`** outside the OpenSpec workflow. Use the OpenSpec CLI and the repo skills/agents (**`/osf-explore`**, **`/osf-propose`**, **`/osf-apply-changes`**, and the **`osf-apply-*`** Task agents) so scaffolding, sequencing, **`tasks.md`** checkoffs, and archive merges stay consistent with the tool.
- **Living specs (`openspec/specs/`)** change **only** as the documented result of **archiving** an approved change (merge into living specs via **`osf-apply-finish`** or the same outcome run explicitly by a human)—never because an agent “caught up” with hand edits.
- **During apply (`osf-apply-start`)**, routine edits inside **`openspec/changes/<name>/`** are **`tasks.md` checkbox** updates unless the human widens scope. **Do not** reconcile **`openspec/specs/`** during apply.
- **Narrow exceptions** (state in chat when you use one): the human orders a **one-off bypass**; or a skill/agent definition explicitly allows a mechanical edit (for example marking **`[x]`** in **`tasks.md`** per **`osf-apply-start`**). Those exceptions **are not** permission to rewrite **`openspec/specs/`** by hand.
- Skills **`/osf-propose`** and **`/osf-explore`** must not merge to the default branch or archive changes **unless** the human clearly asked for apply/finish in the same turn.

## Cursor integration and nomenclature

- **Chat skills** live under **`.cursor/skills/*/SKILL.md`** — they steer the conversational agent’s procedure.
- **Task agents** live under **`.cursor/agents/<name>.md`** — Cursor runs them via the **Task** tool with **`subagent_type`** equal to **`name`**. Opening an agent definition and replaying its steps in the parent thread **breaks isolation**; do not inline **`osf-apply-*`** workflows as a substitute for Task delegation.
- In this bundle, OpenSpec workflow artifacts use the **`osf-*`** prefix — skills at **`.cursor/skills/osf-*/SKILL.md`** and apply/finish/abort agents at **`.cursor/agents/osf-apply-*.md`**. Consumers may rely on that naming when installing the bundle.

**OSF apply workers MUST use Tasks:**

- **`osf-apply-start`**, **`osf-apply-finish`**, and **`osf-apply-abort`** MUST be invoked via the **Task** tool with the matching **`subagent_type`**. Do not read **`.cursor/agents/osf-*.md`** and replay them in the parent thread.
- Follow **`.cursor/skills/osf-apply-changes/SKILL.md`**.

## Git branches and default branch

Unless the user explicitly asks otherwise: **commit and push on the current working branch** for day-to-day work. **`osf-apply-finish`** merges the execution branch into the repository’s **default branch** when completing a change—only do that within that finish workflow or when the user explicitly requests it.

## Safety and environments

- No destructive actions on machines, data, or shared infrastructure **unless** the human named the target and risk is acceptable.
- Honor project-specific allowlists, staging requirements, and secrets handling; when in doubt, stop and ask.
- If the project records **environment or target policies** in specs or docs (for example allowed hosts or data classes), **follow those** during apply; do not substitute your own assumptions.

## Environment variables

Use **`.env`** for local secrets (API keys for optional tools). **`.env`** should remain gitignored in consuming projects.

## Reference

- **`OPENSPEC_FLOW.md`** — narrative, vocabulary, **`OPENSPEC_FLOW_VERSION`** (bundle version for install/upgrade), and capability table.
- **`CHANGELOG.md`** — human-readable history of bundle changes (keep in sync when bumping **`OPENSPEC_FLOW_VERSION`**).
- **`.cursor/skills/openspec-flow-install/SKILL.md`** — install or upgrade this bundle into another project.
