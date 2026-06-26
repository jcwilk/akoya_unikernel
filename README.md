# akoya_unikernel

Unikernel project with [OpenSpec Flow](https://github.com/Fission-AI/OpenSpec) for agentic development in Cursor.

## OpenSpec Flow

This repo uses the **OpenSpec Flow (OSF)** bundle for structured propose → apply → finish workflows.

| Path | Role |
|------|------|
| **`OPENSPEC_FLOW.md`** | Workflow overview and **`OPENSPEC_FLOW_VERSION`** (bundle version). |
| **`CHANGELOG.md`** | Human-readable history of bundle releases. |
| **`AGENTS.md`** | Agent rules: `openspec/` discipline, Task-only apply agents, git posture. |
| **`.cursor/skills/osf-*/`** | Slash-driven skills (`/osf-propose`, `/osf-explore`, …). |
| **`.cursor/agents/osf-*.md`** | Task definitions for apply/finish/abort. |
| **`.cursor/skills/persist/`** | Commit/push hygiene used by propose/finish flows. |
| **`.cursor/skills/openspec-flow-install/`** | Install or upgrade the OSF bundle from a reference repo. |
| **`openspec/`** | Living specs and active changes for this project. |

## Prerequisites

- Node.js **20.19+** (for `npx @fission-ai/openspec@latest …`).
- Cursor with **Task** subagents enabled.

## Quick start

1. Use **`/osf-explore`** and **`/osf-propose`** with this project's **`openspec/`** layout.
2. Use **`/osf-apply-changes`** to implement approved changes on the current branch.
3. Compare **`OPENSPEC_FLOW_VERSION`** in **`OPENSPEC_FLOW.md`** when upgrading the bundle via **`/openspec-flow-install`**.

