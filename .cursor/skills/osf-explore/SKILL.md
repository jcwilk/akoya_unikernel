---
name: osf-explore
description: Thinking-partner mode for shaping intent before (or during) an OpenSpec change. Use when the user says `/osf-explore`, wants to think through an idea, investigate code, clarify requirements, or compare options without implementing. Aligned with `OPENSPEC_FLOW.md` shape phase.
disable-model-invocation: true
---

# `/osf-explore` — think with the human, do not implement

Explore is a **stance**, not a workflow. There are no required steps, no mandatory outputs. The goal is to help the human **shape intent** until it is ready for **`/osf-propose`** to capture as durable artifacts under `openspec/changes/<name>/`.

## Hard rule

**Do not implement.** Read files, search code, sketch ASCII diagrams—**never** write application code, edit non-OpenSpec files, or mutate **`openspec/specs/`**. If the user asks for code, say explore is the wrong mode and point at **`/osf-propose`** + **`/osf-apply-changes`**.

You **may** create or revise OpenSpec change artifacts under **`openspec/changes/<name>/`** if and only if the user explicitly asks to capture thinking that way—prefer offering and letting them say yes. Use **`/osf-propose`** for full artifact pipelines.

## Stance

- **Curious, not prescriptive** — questions that emerge from the conversation, not a script.
- **Open threads, not interrogations** — surface multiple directions; let the human follow what resonates.
- **Visual** — ASCII diagrams when they help (state machines, comparison tables, data flows).
- **Adaptive** — pivot when new information lands.
- **Patient** — let the shape of the problem emerge.
- **Grounded** — read the actual codebase and existing specs/changes; don’t theorize in a vacuum.

## What you might do

- **Problem space**: clarifying questions, challenge assumptions, reframe, find analogies.
- **Codebase**: map architecture relevant to the discussion, find integration points, surface hidden complexity.
- **Compare options**: brainstorm approaches, comparison tables, sketch tradeoffs, recommend if asked.
- **Visualize**: ASCII diagrams over walls of prose.
- **Risks/unknowns**: what could go wrong, gaps, suggest spikes.

## OpenSpec context

At the start, quickly orient:

```bash
npx @fission-ai/openspec@latest list --json
```

- Check if any active change is relevant.
- If the user mentions a change name, read its artifacts (`proposal.md`, `design.md`, delta `specs/`, `tasks.md`) before opining.
- Read **`openspec/specs/<domain>/spec.md`** for living truth before suggesting it should change.

### When no change exists

Think freely. When insights crystallize, **offer**:

- "This feels solid enough to start a change. Want me to run **`/osf-propose`**?"
- Or keep exploring—no pressure to formalize.

### When a change exists

Reference its artifacts naturally in conversation. When a decision is made, **offer** to capture it:

| Insight type | Where to capture |
|--------------|------------------|
| New requirement discovered | `openspec/changes/<name>/specs/<capability>/spec.md` |
| Requirement changed | same delta file |
| Design decision | `design.md` |
| Scope changed | `proposal.md` |
| New work identified | `tasks.md` |
| Assumption invalidated | the relevant artifact |

The user decides. Offer and move on. Don't auto-capture.

## Ending

There is no required ending. Discovery may:

- **Flow into a proposal**: hand off to **`/osf-propose`**.
- **Result in artifact updates** on an existing change.
- **Just provide clarity**: the human moves on.
- **Continue later**: pick it up in a future turn.

A summary at the end is optional. Sometimes the thinking IS the value.

## Guardrails

- **Don't implement** — never write code or mutate non-OpenSpec files; OpenSpec artifact creation only on explicit user ask, otherwise hand off to **`/osf-propose`**.
- **Don't fake understanding** — if something is unclear, dig deeper.
- **Don't rush** — discovery is thinking time, not task time.
- **Don't force structure** — let patterns emerge.
- **Don't auto-capture** — offer, don't just do.
- **Do read the codebase** — ground discussions in reality.
- **Do question assumptions** — including yours and the user's.
- **Do hand off cleanly** — when intent is ready, say so and route to **`/osf-propose`**.

## Reference

- Flow narrative: **`OPENSPEC_FLOW.md`** (shape phase).
- Living vs change specs and `openspec/` discipline: **`AGENTS.md`**.
- Capture pipeline after explore: **`/osf-propose`** (`osf-propose`).
