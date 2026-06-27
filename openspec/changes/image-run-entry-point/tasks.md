## Required for this change

## 1. Run entry point CLI

- [x] 1.1 Refactor the QEMU run script so `--headful` and `--headless` are mandatory, mutually exclusive mode flags (no default)
- [x] 1.2 Add optional `--image PATH` argument; when omitted, auto-select only if exactly one logical runnable image exists in the build output directory
- [x] 1.3 Implement logical-image discovery: group `*.elf` / `*.bin` by stem (`kernel`, `v1`, …); stem dedup so `v1.elf` + `v1.bin` count as one `v1`; prefer `.elf` at run time when both exist; fail fast with stem list when zero or multiple logical images
- [x] 1.4 Split headless vs headful execution paths: headless keeps timeout + bootstrap message assertion; headful uses interactive display without smoke-test gating

## 2. Integration and docs

- [x] 2.1 Update `Makefile` so `test` invokes headless mode and `run` invokes headful mode via the unified script
- [x] 2.2 Update `README.md` with run script usage, mandatory mode flags, stem-grouped auto-selection, and ambiguity error behavior

## 3. Verification

- [x] 3.1 Verify invoking the run script without a mode flag exits non-zero with a clear error
- [x] 3.2 Verify headless `make test` still passes smoke test on a fresh build
- [x] 3.3 Verify headful `make run` launches emulation with visible display (or documented remote-viewer fallback) on the development workstation
- [x] 3.4 Verify empty build output directory fails fast with a build-required message
- [x] 3.5 Verify `kernel.elf` + `kernel.bin` (or equivalent stem pair) auto-selects as one logical image
- [x] 3.6 Verify multiple stems (e.g. `v1` + `v2` variants) fails fast and lists logical identities in the error

## Explicitly deferred

- Renaming `run-qemu.sh` unless apply finds the old name misleading after refactor
- Bare-metal run path or USB boot automation
- Supporting a third default or implicit run mode
- Recursive or subdirectory-based image discovery
