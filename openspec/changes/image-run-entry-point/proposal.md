## Why

Running built images today splits across ad-hoc flags and Makefile targets, with implicit defaults that hide whether the user wants interactive display or automated smoke testing. A single, explicit run entry point with mandatory headful/headless choice and predictable artifact selection will make human and agent invocation safer and easier to diagnose.

## What Changes

- Introduce one documented script as the canonical way to run a built boot image under emulation.
- **BREAKING:** Require callers to specify either headful or headless mode; the runner SHALL NOT infer a default display mode.
- Accept an optional path to a specific boot image; when omitted, auto-select only when the build output area contains exactly one runnable image, otherwise fail fast with an explanatory error.
- Align existing Makefile test/run targets and QEMU wrapper behavior with the new entry-point contract.
- Clarify which existing smoke-test behaviors (timeout, message assertion, serial capture) apply only in headless mode.

## Capabilities

### New Capabilities

<!-- None — behavior extends the existing dev-test-runner domain. -->

### Modified Capabilities

- `dev-test-runner`: Replace implicit run/smoke-test invocation with a unified run entry point, mandatory headful/headless mode, and deterministic artifact resolution from the build output area.

## Impact

- `scripts/run-qemu.sh` (or successor run script), `Makefile` targets (`test`, `run`, etc.), and `README.md` usage docs.
- Living `dev-test-runner` spec requirements for the QEMU entry point, artifact selection, and mode-specific smoke-test behavior.
- No change to compilation pipeline, bootstrap kernel behavior, or deployment-target constraints.
