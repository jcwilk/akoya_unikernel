<!--
  Adapted from:  https://raw.githubusercontent.com/Fission-AI/OpenSpec/main/docs/concepts.md
  Captured:      2026-05-05
  License:       MIT (Fission-AI/OpenSpec)

  This is an *adapted* offline reference for agents using this OpenSpec Flow
  Cursor bundle (see **`OPENSPEC_FLOW.md`** for **`OPENSPEC_FLOW_VERSION`**), not a
  verbatim mirror.

  Where the upstream doc references OpenSpec's stock slash commands
  (/opsx:propose, /opsx:new, /opsx:ff, /opsx:continue, /opsx:apply,
  /opsx:verify, /opsx:archive) or describes process specifics that don't
  apply here, this file substitutes our /osf-* skills/agents and our
  branch-isolated, atomic-merge flow:

    /opsx:propose, /opsx:new, /opsx:ff, /opsx:continue
        -> /osf-propose                (and optionally /osf-explore upstream)
    /opsx:apply
        -> /osf-apply-changes (orchestrator) which spawns
           /osf-apply-start (one worker per change, on an isolated branch)
    /opsx:verify, /opsx:archive
        -> /osf-apply-finish (verifies, archives on the branch so deltas
                              merge into openspec/specs/, merges the
                              execution branch into main, pushes —
                              all atomically; there is no separate verify
                              or archive step a human runs)
    blocked work
        -> /osf-apply-abort (rolls back unapproved work, returns a
                             debrief, never merges into main)

  The multi-repo "Coordination Workspaces" section has been collapsed to
  a stub when this bundle ships without multi-repo tooling.

  Spec-authoring guidance — the substance of "What a Spec Is", delta
  sections, RFC 2119, Lite-vs-Full, scenario shape, and so on — is
  preserved as written. That is the value-add of this file.

  The /osf-propose skill (../SKILL.md) instructs agents to read this
  file before drafting any change artifacts.

  Refresh recipe (when you suspect upstream spec-authoring guidance has
  evolved and want to re-port the changes here):

    1. Fetch upstream to a temp file:

         curl -fsSL \
           https://raw.githubusercontent.com/Fission-AI/OpenSpec/main/docs/concepts.md \
           -o /tmp/openspec-concepts-upstream.md

    2. Diff against this file:

         diff -u .cursor/skills/osf-propose/reference/concepts.md \
                 /tmp/openspec-concepts-upstream.md

    3. Port any new spec-authoring guidance into this file by hand,
       preserving our /osf-* command substitutions and the collapsed
       Coordination Workspaces stub. DO NOT just clobber this file with
       the upstream copy — that would re-introduce the /opsx:* references
       and the multi-repo workspace section.

    4. Bump the "Captured:" date above and update this header if the
       command-substitution table needs revising.

  Required reading priority for /osf-propose:

    1. "## Specs" section, especially:
         - "### Spec Format"
         - "### Why Structure Specs This Way"
         - "### What a Spec Is (and Is Not)"   <-- most important
         - "### Keep It Lightweight: Progressive Rigor"
    2. "## Delta Specs"
    3. "## Changes" and "## Artifacts"
    4. "## Archive" — note: in this repo, archive is performed by
       /osf-apply-finish on the execution branch, not as a standalone
       command.
-->

# Concepts

This guide explains the core ideas behind OpenSpec and how they fit together. For practical usage in this repo, see **`OPENSPEC_FLOW.md`** at the repo root and the **`/osf-propose`** skill that this file supports. For upstream context, see [Getting Started](https://github.com/Fission-AI/OpenSpec/blob/main/docs/getting-started.md) and [Workflows](https://github.com/Fission-AI/OpenSpec/blob/main/docs/workflows.md).

## Philosophy

OpenSpec is built around four principles:

```
fluid not rigid         — no phase gates, work on what makes sense
iterative not waterfall — learn as you build, refine as you go
easy not complex        — lightweight setup, minimal ceremony
brownfield-first        — works with existing codebases, not just greenfield
```

### Why These Principles Matter

**Fluid not rigid.** Traditional spec systems lock you into phases: first you plan, then you implement, then you're done. OpenSpec is more flexible — you can create artifacts in any order that makes sense for your work.

**Iterative not waterfall.** Requirements change. Understanding deepens. What seemed like a good approach at the start might not hold up after you see the codebase. OpenSpec embraces this reality.

**Easy not complex.** Some spec frameworks require extensive setup, rigid formats, or heavyweight processes. OpenSpec stays out of your way. Initialize in seconds, start working immediately, customize only if you need to.

**Brownfield-first.** Most software work isn't building from scratch — it's modifying existing systems. OpenSpec's delta-based approach makes it easy to specify changes to existing behavior, not just describe new systems.

## The Big Picture

OpenSpec organizes your work into two main areas:

```
┌────────────────────────────────────────────────────────────────────┐
│                        openspec/                                   │
│                                                                    │
│   ┌─────────────────────┐      ┌───────────────────────────────┐   │
│   │       specs/        │      │         changes/              │   │
│   │                     │      │                               │   │
│   │  Source of truth    │◄─────│  Proposed modifications       │   │
│   │  How your system    │ merge│  Each change = one folder     │   │
│   │  currently works    │      │  Contains artifacts + deltas  │   │
│   │                     │      │                               │   │
│   └─────────────────────┘      └───────────────────────────────┘   │
│                                                                    │
└────────────────────────────────────────────────────────────────────┘
```

**Specs** are the source of truth — they describe how your system currently behaves.

**Changes** are proposed modifications — they live in separate folders until you're ready to merge them.

This separation is key. You can work on multiple changes in parallel without conflicts. You can review a change before it affects the main specs. And when you archive a change, its deltas merge cleanly into the source of truth.

## Coordination Workspaces (not applicable here)

OpenSpec's coordination-workspace feature is for planning across multiple linked repos. **This project is a single repo**, so the workspace surface does not apply: specs and changes live under `openspec/specs/` and `openspec/changes/` directly, and `openspec workspace …` commands are not used. If multi-repo planning is ever needed, see the upstream [Coordination Workspaces](https://github.com/Fission-AI/OpenSpec/blob/main/docs/concepts.md#coordination-workspaces) section.

## Specs

Specs describe your system's behavior using structured requirements and scenarios.

### Structure

```
openspec/specs/
├── auth/
│   └── spec.md           # Authentication behavior
├── payments/
│   └── spec.md           # Payment processing
├── notifications/
│   └── spec.md           # Notification system
└── ui/
    └── spec.md           # UI behavior and themes
```

Organize specs by domain — logical groupings that make sense for your system. Common patterns:

- **By feature area**: `auth/`, `payments/`, `search/`
- **By component**: `api/`, `frontend/`, `workers/`
- **By bounded context**: `ordering/`, `fulfillment/`, `inventory/`

### Spec Format

A spec contains requirements, and each requirement has scenarios:

```markdown
# Auth Specification

## Purpose
Authentication and session management for the application.

## Requirements

### Requirement: User Authentication
The system SHALL issue a JWT token upon successful login.

#### Scenario: Valid credentials
- GIVEN a user with valid credentials
- WHEN the user submits login form
- THEN a JWT token is returned
- AND the user is redirected to dashboard

#### Scenario: Invalid credentials
- GIVEN invalid credentials
- WHEN the user submits login form
- THEN an error message is displayed
- AND no token is issued

### Requirement: Session Expiration
The system MUST expire sessions after 30 minutes of inactivity.

#### Scenario: Idle timeout
- GIVEN an authenticated session
- WHEN 30 minutes pass without activity
- THEN the session is invalidated
- AND the user must re-authenticate
```

**Key elements:**

| Element | Purpose |
|---------|---------|
| `## Purpose` | High-level description of this spec's domain |
| `### Requirement:` | A specific behavior the system must have |
| `#### Scenario:` | A concrete example of the requirement in action |
| SHALL/MUST/SHOULD | RFC 2119 keywords indicating requirement strength |

### Why Structure Specs This Way

**Requirements are the "what"** — they state what the system should do without specifying implementation.

**Scenarios are the "when"** — they provide concrete examples that can be verified. Good scenarios:
- Are testable (you could write an automated test for them)
- Cover both happy path and edge cases
- Use Given/When/Then or similar structured format

**RFC 2119 keywords** (SHALL, MUST, SHOULD, MAY) communicate intent:
- **MUST/SHALL** — absolute requirement
- **SHOULD** — recommended, but exceptions exist
- **MAY** — optional

### What a Spec Is (and Is Not)

A spec is a **behavior contract**, not an implementation plan.

Good spec content:
- Observable behavior users or downstream systems rely on
- Inputs, outputs, and error conditions
- External constraints (security, privacy, reliability, compatibility)
- Scenarios that can be tested or explicitly validated

Avoid in specs:
- Internal class/function names
- Library or framework choices
- Step-by-step implementation details
- Detailed execution plans (those belong in `design.md` or `tasks.md`)

Quick test:
- If implementation can change without changing externally visible behavior, it likely does not belong in the spec.

### Keep It Lightweight: Progressive Rigor

OpenSpec aims to avoid bureaucracy. Use the lightest level that still makes the change verifiable.

**Lite spec (default):**
- Short behavior-first requirements
- Clear scope and non-goals
- A few concrete acceptance checks

**Full spec (for higher risk):**
- Cross-team or cross-repo changes
- API/contract changes, migrations, security/privacy concerns
- Changes where ambiguity is likely to cause expensive rework

Most changes should stay in Lite mode.

### Human + Agent Collaboration (in this repo)

In this repo the loop is concrete and the agent steps are slash commands rather than a generic "agent." The human provides intent — sometimes through chat, sometimes through **`/osf-explore`** (read-only thinking partner) when intent is fuzzy — and **`/osf-propose`** drafts the artifacts under `openspec/changes/<name>/`:

1. Human provides intent, context, and constraints.
2. **`/osf-propose`** converts this into behavior-first requirements and scenarios under `openspec/changes/<name>/specs/<domain>/spec.md`.
3. **`/osf-propose`** keeps implementation detail in `design.md` and `tasks.md`, never in any `spec.md`.
4. `npx @fission-ai/openspec@latest validate <name> --type change` plus the **`/osf-propose`** spec-quality re-read confirm both structure *and* content quality before implementation begins. (`openspec validate` checks structure; the re-read catches behavior-vs-implementation drift, which the validator does not.)

This keeps specs readable for humans and consistent for agents. Implementation then happens via **`/osf-apply-changes`** → **`/osf-apply-start`** on isolated branches (see the flow diagram near the end of this file).

## Changes

A change is a proposed modification to your system, packaged as a folder with everything needed to understand and implement it.

### Change Structure

```
openspec/changes/add-dark-mode/
├── proposal.md           # Why and what
├── design.md             # How (technical approach)
├── tasks.md              # Implementation checklist
├── .openspec.yaml        # Change metadata (optional)
└── specs/                # Delta specs
    └── ui/
        └── spec.md       # What's changing in ui/spec.md
```

Each change is self-contained. It has:
- **Artifacts** — documents that capture intent, design, and tasks
- **Delta specs** — specifications for what's being added, modified, or removed
- **Metadata** — optional configuration for this specific change

### Why Changes Are Folders

Packaging a change as a folder has several benefits:

1. **Everything together.** Proposal, design, tasks, and specs live in one place. No hunting through different locations.

2. **Parallel work.** Multiple changes can exist simultaneously without conflicting. Work on `add-dark-mode` while `fix-auth-bug` is also in progress.

3. **Clean history.** When archived, changes move to `changes/archive/` with their full context preserved. You can look back and understand not just what changed, but why.

4. **Review-friendly.** A change folder is easy to review — open it, read the proposal, check the design, see the spec deltas.

## Artifacts

Artifacts are the documents within a change that guide the work.

### The Artifact Flow

```
proposal ──────► specs ──────► design ──────► tasks ──────► implement
    │               │             │              │
   why            what           how          steps
 + scope        changes       approach      to take
```

Artifacts build on each other. Each artifact provides context for the next.

### Artifact Types

#### Proposal (`proposal.md`)

The proposal captures **intent**, **scope**, and **approach** at a high level.

```markdown
# Proposal: Add Dark Mode

## Intent
Users have requested a dark mode option to reduce eye strain
during nighttime usage and match system preferences.

## Scope
In scope:
- Theme toggle in settings
- System preference detection
- Persist preference in localStorage

Out of scope:
- Custom color themes (future work)
- Per-page theme overrides

## Approach
Use CSS custom properties for theming with a React context
for state management. Detect system preference on first load,
allow manual override.
```

**When to update the proposal:**
- Scope changes (narrowing or expanding)
- Intent clarifies (better understanding of the problem)
- Approach fundamentally shifts

#### Specs (delta specs in `specs/`)

Delta specs describe **what's changing** relative to the current specs. See [Delta Specs](#delta-specs) below.

#### Design (`design.md`)

The design captures **technical approach** and **architecture decisions**.

````markdown
# Design: Add Dark Mode

## Technical Approach
Theme state managed via React Context to avoid prop drilling.
CSS custom properties enable runtime switching without class toggling.

## Architecture Decisions

### Decision: Context over Redux
Using React Context for theme state because:
- Simple binary state (light/dark)
- No complex state transitions
- Avoids adding Redux dependency

### Decision: CSS Custom Properties
Using CSS variables instead of CSS-in-JS because:
- Works with existing stylesheet
- No runtime overhead
- Browser-native solution

## Data Flow
```
ThemeProvider (context)
       │
       ▼
ThemeToggle ◄──► localStorage
       │
       ▼
CSS Variables (applied to :root)
```

## File Changes
- `src/contexts/ThemeContext.tsx` (new)
- `src/components/ThemeToggle.tsx` (new)
- `src/styles/globals.css` (modified)
````

**When to update the design:**
- Implementation reveals the approach won't work
- Better solution discovered
- Dependencies or constraints change

#### Tasks (`tasks.md`)

Tasks are the **implementation checklist** — concrete steps with checkboxes.

```markdown
# Tasks

## 1. Theme Infrastructure
- [ ] 1.1 Create ThemeContext with light/dark state
- [ ] 1.2 Add CSS custom properties for colors
- [ ] 1.3 Implement localStorage persistence
- [ ] 1.4 Add system preference detection

## 2. UI Components
- [ ] 2.1 Create ThemeToggle component
- [ ] 2.2 Add toggle to settings page
- [ ] 2.3 Update Header to include quick toggle

## 3. Styling
- [ ] 3.1 Define dark theme color palette
- [ ] 3.2 Update components to use CSS variables
- [ ] 3.3 Test contrast ratios for accessibility
```

**Task best practices:**
- Group related tasks under headings
- Use hierarchical numbering (1.1, 1.2, etc.)
- Keep tasks small enough to complete in one session
- Check tasks off as you complete them

## Delta Specs

Delta specs are the key concept that makes OpenSpec work for brownfield development. They describe **what's changing** rather than restating the entire spec.

### The Format

```markdown
# Delta for Auth

## ADDED Requirements

### Requirement: Two-Factor Authentication
The system MUST support TOTP-based two-factor authentication.

#### Scenario: 2FA enrollment
- GIVEN a user without 2FA enabled
- WHEN the user enables 2FA in settings
- THEN a QR code is displayed for authenticator app setup
- AND the user must verify with a code before activation

#### Scenario: 2FA login
- GIVEN a user with 2FA enabled
- WHEN the user submits valid credentials
- THEN an OTP challenge is presented
- AND login completes only after valid OTP

## MODIFIED Requirements

### Requirement: Session Expiration
The system MUST expire sessions after 15 minutes of inactivity.
(Previously: 30 minutes)

#### Scenario: Idle timeout
- GIVEN an authenticated session
- WHEN 15 minutes pass without activity
- THEN the session is invalidated

## REMOVED Requirements

### Requirement: Remember Me
(Deprecated in favor of 2FA. Users should re-authenticate each session.)
```

### Delta Sections

| Section | Meaning | What Happens on Archive |
|---------|---------|------------------------|
| `## ADDED Requirements` | New behavior | Appended to main spec |
| `## MODIFIED Requirements` | Changed behavior | Replaces existing requirement |
| `## REMOVED Requirements` | Deprecated behavior | Deleted from main spec |

### Why Deltas Instead of Full Specs

**Clarity.** A delta shows exactly what's changing. Reading a full spec, you'd have to diff it mentally against the current version.

**Conflict avoidance.** Two changes can touch the same spec file without conflicting, as long as they modify different requirements.

**Review efficiency.** Reviewers see the change, not the unchanged context. Focus on what matters.

**Brownfield fit.** Most work modifies existing behavior. Deltas make modifications first-class, not an afterthought.

## Schemas

A schema defines the artifact types and their dependency graph for a workflow. **This repo uses the upstream `spec-driven` default schema** (proposal → specs → design → tasks → implement) and calls OpenSpec through the `osf-*` skills rather than configuring custom schemas.

The dependency graph is what drives **`/osf-propose`**'s artifact build loop:

```
                    proposal
                   (root node)
                       │
         ┌─────────────┴─────────────┐
         │                           │
         ▼                           ▼
      specs                       design
   (requires:                  (requires:
    proposal)                   proposal)
         │                           │
         └─────────────┬─────────────┘
                       │
                       ▼
                    tasks
                (requires:
                specs, design)
```

**Dependencies are enablers, not gates.** They show what is *possible* to create next, not what is *required*. **`/osf-propose`** walks this graph in `ready` order via `npx @fission-ai/openspec@latest status --change <name> --json` and `npx @fission-ai/openspec@latest instructions <artifact-id> --change <name> --json`, writing each artifact at its `outputPath` until everything in `applyRequires` is `done`.

Custom schemas (`openspec schema init`, `openspec schema fork`) are an upstream feature this repo does not currently use. See [upstream Concepts → Schemas](https://github.com/Fission-AI/OpenSpec/blob/main/docs/concepts.md#schemas) if that ever changes.

## Archive

Archiving completes a change by merging its delta specs into the main specs and preserving the change for history. **In this repo, archive is not a standalone command** — it is performed by **`/osf-apply-finish`** on the execution branch as part of an atomic finish.

### What Happens When You Archive

```
Before archive (on the execution branch):

openspec/
├── specs/
│   └── auth/
│       └── spec.md ◄────────────────┐
└── changes/                         │
    └── add-2fa/                     │
        ├── proposal.md              │
        ├── design.md                │ merge (during /osf-apply-finish)
        ├── tasks.md                 │
        └── specs/                   │
            └── auth/                │
                └── spec.md ─────────┘


After archive (still on the execution branch, before merge into main):

openspec/
├── specs/
│   └── auth/
│       └── spec.md        # Now includes 2FA requirements
└── changes/
    └── archive/
        └── 2026-05-05-add-2fa/    # Preserved for history
            ├── proposal.md
            ├── design.md
            ├── tasks.md
            └── specs/
                └── auth/
                    └── spec.md
```

### The Archive Process (in this repo)

In this repo, archive is a *step inside* **`/osf-apply-finish`**, not a standalone command. The full atomic finish is:

1. **Verify.** Tasks are checked off and `npx @fission-ai/openspec@latest validate <name> --type change` passes.
2. **Archive on the execution branch.** Each delta section (ADDED / MODIFIED / REMOVED) is applied to the corresponding main spec under `openspec/specs/`. The change folder moves to `openspec/changes/archive/` with a date prefix for chronological ordering. All artifacts remain intact in the archive.
3. **Merge the execution branch into `main`.**
4. **Push** to the remote.

This means there is **never a window** where behavior lands on `main` while living specs lag — `openspec/specs/` and `main` advance together. If a worker discovers the approved change should not continue, it spawns **`/osf-apply-abort`** instead, which rolls back unapproved work, returns a debrief, and never archives.

### Why Archive Matters

**Clean state.** Active changes (`openspec/changes/`) shows only work in progress. Completed work moves out of the way to `openspec/changes/archive/`.

**Audit trail.** The archive preserves the full context of every change — not just what changed, but the proposal explaining why, the design explaining how, and the tasks showing the work done.

**Spec evolution.** Specs grow organically as changes are archived. Each archive merges its deltas, building up a comprehensive specification over time. Because the merge into `main` is atomic with the archive, a reader of `main` at any commit sees living specs that match the implementation at that commit.

## How It All Fits Together (in this repo)

This diagram replaces the upstream `/opsx:*` flow with the `/osf-*` flow used here. Note the explore lane upstream of propose, the branch-isolated implement step, and the success/blocked split at the end.

```
                          (optional)
              ┌── /osf-explore ──────────┐
              │   read-only thinking     │
              │   partner; never writes  │
              └─────────────┬────────────┘
                            │
                            ▼
              ┌──────────────────────────┐
              │  /osf-propose            │  shape openspec/changes/<name>/
              │                          │  proposal -> delta specs ->
              │                          │  design -> tasks
              │                          │  validate + spec-quality re-read
              └─────────────┬────────────┘
                            │  (human approves intent)
                            ▼
              ┌──────────────────────────┐
              │  /osf-apply-changes      │  orchestrator: spawn one or more
              │                          │  workers in isolated branches
              │                          │  or worktrees
              └─────────────┬────────────┘
                            ▼
              ┌──────────────────────────┐
              │  /osf-apply-start        │  one worker per change, on its
              │  (Task subagent)         │  own branch; walks tasks.md,
              │                          │  ticks checkboxes, validates
              └─────┬──────────────┬─────┘
                    │              │
                success         blocked
                    │              │
                    ▼              ▼
   ┌─────────────────────────┐  ┌────────────────────────┐
   │  /osf-apply-finish      │  │  /osf-apply-abort      │
   │  (Task subagent)        │  │  (Task subagent)       │
   │                         │  │                        │
   │  1. verify              │  │  - roll back unapproved│
   │  2. archive on the      │  │    work from the       │
   │     branch (deltas      │  │    execution branch    │
   │     merge into          │  │  - park investigation  │
   │     openspec/specs/)    │  │    on a clearly named  │
   │  3. merge execution     │  │    exploratory branch  │
   │     branch into main    │  │    if useful           │
   │  4. push                │  │  - return debrief to   │
   │                         │  │    the human; never    │
   │  (atomic; no window     │  │    edits the change    │
   │   where main and        │  │    folder              │
   │   openspec/specs/       │  │  - check out main      │
   │   disagree)             │  │                        │
   └────────────┬────────────┘  └───────────┬────────────┘
                │                           │
                ▼                           ▼
       main + openspec/specs/      human revises intent via
       advance together; the       /osf-propose, then re-runs
       cycle restarts on the       /osf-apply-changes
       next change
```

**The virtuous cycle (in this repo):**

1. Living specs under `openspec/specs/` describe current behavior.
2. **`/osf-propose`** (optionally fed by **`/osf-explore`**) proposes modifications as deltas under `openspec/changes/<name>/`.
3. **`/osf-apply-changes`** spawns **`/osf-apply-start`** workers that make the changes real on isolated branches.
4. **`/osf-apply-finish`** archives (deltas merge into `openspec/specs/`) and merges into `main` atomically — `main` and living specs always agree.
5. Living specs now describe the new behavior.
6. The next change builds on updated specs.

If **`/osf-apply-abort`** runs instead of **`/osf-apply-finish`**, the cycle pauses at step 3: the human revises intent via **`/osf-propose`** based on the abort debrief, then re-runs **`/osf-apply-changes`**.

## Glossary

| Term | Definition |
|------|------------|
| **Artifact** | A document within a change (proposal, design, tasks, or delta specs) |
| **Archive** | The process of completing a change and merging its deltas into main specs |
| **Change** | A proposed modification to the system, packaged as a folder with artifacts |
| **Delta spec** | A spec that describes changes (ADDED/MODIFIED/REMOVED) relative to current specs |
| **Domain** | A logical grouping for specs (e.g., `auth/`, `payments/`) |
| **Requirement** | A specific behavior the system must have |
| **Scenario** | A concrete example of a requirement, typically in Given/When/Then format |
| **Schema** | A definition of artifact types and their dependencies |
| **Spec** | A specification describing system behavior, containing requirements and scenarios |
| **Source of truth** | The `openspec/specs/` directory, containing the current agreed-upon behavior |

## Next Steps

For repo-local guidance the agent should prefer (these reflect the `osf-*` flow in this repo):

- **`OPENSPEC_FLOW.md` → "What a spec actually is"** at the repo root — the repo-local distillation of this file plus the `osf-*` capability table.
- **`AGENTS.md`** at the repo root — `openspec/` workflow discipline (read-only living specs, archive-only merges, safety boundaries).
- **`.cursor/skills/osf-propose/SKILL.md`** — the skill this reference supports, including the spec-quality checklist and the drift-to-refuse list.

Upstream OpenSpec docs (canonical, may evolve — re-port spec-authoring changes here per this file's header recipe; do **not** consult the upstream slash-command reference as an authority for this repo's flow):

- [Getting Started](https://github.com/Fission-AI/OpenSpec/blob/main/docs/getting-started.md) — practical first steps with stock OpenSpec.
- [Workflows](https://github.com/Fission-AI/OpenSpec/blob/main/docs/workflows.md) — upstream patterns.
- [Commands](https://github.com/Fission-AI/OpenSpec/blob/main/docs/commands.md) — full upstream `/opsx:*` command reference. **In this repo, use the `osf-*` skills/agents instead**; this link is for context only.
- [Customization](https://github.com/Fission-AI/OpenSpec/blob/main/docs/customization.md) — custom schemas, not currently used here.
