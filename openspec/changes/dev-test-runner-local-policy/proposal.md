## Why

Recent workstation setup and runner simplification (native emulator, no container fallbacks, headful hold-open vs headless auto-exit) are implemented in scripts and README but not yet reflected in living specs. Codifying these decisions prevents drift back toward optional Docker/VNC paths and makes the dev-test contract explicit for agents and future changes.

## What Changes

- Replace the `dev-test-runner` Purpose stub with a concise capability description.
- Tighten headful mode to **interactive display on the development workstation** (drop remote-viewer obligation).
- Add **guest shutdown behavior**: headless defaults to emulator exit after bootstrap; headful defaults to holding the session open after the guest halts; explicit caller overrides allowed.
- Add **local workstation emulator** requirement: 32-bit x86 system emulation via host-installed tools, fail-fast when missing, no containerized emulator fallback.
- Add **native workstation toolchain** requirement on `unikernel-build-pipeline`: cross-compile using host-available toolchain (including vendored repo tools), no containerized build fallback.
- Reconcile README and scripts with any spec wording gaps found during apply (implementation largely already matches).

## Capabilities

### New Capabilities

<!-- None -->

### Modified Capabilities

- `dev-test-runner`: Purpose text, headful display scope, guest shutdown defaults/overrides, local emulator prerequisite.
- `unikernel-build-pipeline`: Purpose text, native toolchain without container fallback.

## Impact

- Living specs under `openspec/specs/dev-test-runner/` and `openspec/specs/unikernel-build-pipeline/`.
- Verification-only apply work for scripts/README already aligned; no expected kernel or target-profile changes.
