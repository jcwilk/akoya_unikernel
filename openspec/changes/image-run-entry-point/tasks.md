## Required for this change

## 1. Run entry point CLI

- [ ] 1.1 Refactor the QEMU run script so `--headful` and `--headless` are mandatory, mutually exclusive mode flags (no default)
- [ ] 1.2 Add optional `--image PATH` argument; when omitted, auto-select only if exactly one runnable image exists in the build output directory
- [ ] 1.3 Implement runnable-image discovery (`.elf`/`.bin` in build output, dedupe elf+bin pairs by stem) with fail-fast errors for zero or multiple candidates
- [ ] 1.4 Split headless vs headful execution paths: headless keeps timeout + bootstrap message assertion; headful uses interactive display without smoke-test gating

## 2. Integration and docs

- [ ] 2.1 Update `Makefile` so `test` invokes headless mode and `run` invokes headful mode via the unified script
- [ ] 2.2 Remove or fold ad-hoc targets/flags that bypass mandatory mode selection
- [ ] 2.3 Update `README.md` with run script usage, mode requirement, and artifact selection rules

## 3. Verification

- [ ] 3.1 Verify invoking the run script without a mode flag exits non-zero with a clear error
- [ ] 3.2 Verify headless `make test` still passes smoke test on a fresh build
- [ ] 3.3 Verify headful `make run` launches emulation with visible display (or documented VNC fallback) on the development workstation
- [ ] 3.4 Verify ambiguous and empty build output directories produce the specified fail-fast errors

## Explicitly deferred

- Renaming `run-qemu.sh` unless apply finds the old name misleading after refactor
- Bare-metal run path or USB boot automation
- Supporting a third default or implicit run mode
