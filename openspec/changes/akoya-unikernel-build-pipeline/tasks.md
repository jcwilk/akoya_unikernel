## Required for this change

## 1. Target profile and documentation

- [ ] 1.1 Add `target/akoya.profile` with deployment constants (32-bit x86, Pentium M class, no VT-x, ~2 GB RAM assumptions)
- [ ] 1.2 Update `README.md` with prerequisites (cross toolchain, QEMU), build/test commands, and bare-metal boot overview

## 2. Bootstrap kernel

- [ ] 2.1 Create freestanding i686 multiboot1 kernel skeleton (`kernel/`, `linker.ld`) with entry and boot header
- [ ] 2.2 Implement minimal console output (serial COM1 + VGA text) in `kernel/console/`
- [ ] 2.3 Implement bootstrap `main` that prints fixed diagnostic message plus build identity (timestamp or git describe)
- [ ] 2.4 Add `.gitignore` entries for `build/` artifacts and logs

## 3. Build pipeline

- [ ] 3.1 Add `scripts/build.sh` using i686-elf (or equivalent) cross GCC with Pentium M–appropriate flags
- [ ] 3.2 Wrap compilation with virtual-memory limit defaulting to ~4 GB (`AKOYA_BUILD_MEM_LIMIT_MB` override)
- [ ] 3.3 Emit `AKOYA_BUILD_RESULT=...` summary line and write `build/build.log` on every run
- [ ] 3.4 Add top-level `Makefile` targets: `build`, `clean`, `test` delegating to scripts

## 4. QEMU dev runner

- [ ] 4.1 Add `scripts/run-qemu.sh` using `qemu-system-i386`, multiboot load, serial on stdio, no KVM requirement
- [ ] 4.2 Assert bootstrap diagnostic message in captured output; enforce timeout on boot hang
- [ ] 4.3 Wire `make test` to build-if-needed then run QEMU smoke test with non-zero exit on failure

## 5. Workstation verification

- [ ] 5.1 Run `make build` on the development workstation and confirm peak compile memory stays within configured ceiling
- [ ] 5.2 Run `make test` on the development workstation and capture passing QEMU smoke-test evidence in apply notes

## Explicitly deferred

- Automated USB/GRUB image creation and bare-metal flash tooling (manual steps documented in README only)
- Unikraft or other full unikernel framework integration
- Network, storage, audio, and wireless drivers for the Akoya platform
- CI pipeline and remote builders
- Bare-metal acceptance test on physical Akoya hardware (human-operated follow-up after apply)
