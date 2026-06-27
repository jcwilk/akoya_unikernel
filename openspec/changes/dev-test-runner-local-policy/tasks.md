## Required for this change

## 1. Living spec reconciliation

- [ ] 1.1 Replace `dev-test-runner` Purpose stub with text from `design.md` at archive
- [ ] 1.2 Replace `unikernel-build-pipeline` Purpose stub with text from `design.md` at archive

## 2. Implementation alignment (verify existing branch work)

- [x] 2.1 Confirm `scripts/run-qemu.sh` matches guest-shutdown defaults and `--hold` / `--exit-on-guest-done` overrides
- [x] 2.2 Confirm `scripts/run-qemu.sh` requires host emulator and has no container fallback
- [x] 2.3 Confirm `scripts/build.sh` requires host toolchain and has no container fallback
- [x] 2.4 Confirm `README.md` documents local prerequisites, shutdown-behavior flags, and absence of Docker/VNC paths

## 3. Verification

- [x] 3.1 `make test` passes (headless auto-exit smoke test)
- [x] 3.2 `make run` with default headful holds emulator open after guest halts (manual or scripted observation)
- [x] 3.3 Invoking run script without host `qemu-system-i386` on PATH fails fast with install guidance (simulate by temporarily overriding PATH in apply notes)
- [x] 3.4 Invoking build without cross toolchain on PATH fails fast (simulate or document vendored `tools/bin` satisfies spec)

## Explicitly deferred

- CI or multi-machine dev environments
- Remote/VNC headful viewers
- Reintroducing any containerized build or emulator fallback
