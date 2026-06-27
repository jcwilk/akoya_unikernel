# akoya_unikernel

Bare-metal unikernel project targeting the **Medion Akoya EX** notebook (32-bit x86, Pentium M class, no VT-x). The repository provides an agent-friendly build pipeline, a bootstrap diagnostic kernel, and a QEMU smoke-test path for pre-hardware validation.

## Target hardware

| Property | Value |
|----------|-------|
| CPU | Intel Pentium M 735 (i686, single core) |
| RAM | ~2 GB |
| Virtualization | Not required (no VT-x dependency) |
| Boot | Multiboot1 via GRUB on USB (manual setup) |

Machine-readable constants live in `target/akoya.profile`.

## Prerequisites

Development workstation (Linux):

- **Cross toolchain:** `i686-elf-gcc` and `i686-elf-binutils` on `PATH`, or the vendored toolchain under `tools/bin/` (prepended automatically by `scripts/build.sh`). To install system-wide, see [OSDev GCC Cross-Compiler](https://wiki.osdev.org/GCC_Cross-Compiler).

- **QEMU:** `qemu-system-i386` for emulation (Debian/Ubuntu package `qemu-system-x86`):

  ```bash
  sudo apt install qemu-system-x86
  ```

- **Node.js 20.19+** for OpenSpec Flow (`npx @fission-ai/openspec@latest …`).

## Build and test

```bash
make build    # cross-compile bootstrap kernel → build/kernel.elf and build/kernel.bin
make test     # build (if needed) + headless QEMU smoke test
make run      # build (if needed) + headful interactive QEMU session
make clean    # remove build/ artifacts
```

### Run entry point (`scripts/run-qemu.sh`)

The canonical way to run a built boot image under emulation. **Display mode is mandatory** — specify exactly one of `--headful` or `--headless`; the script exits with an error if neither or both are given.

```bash
# Headless smoke test (timeout + bootstrap message assertion)
bash scripts/run-qemu.sh --headless

# Interactive display window (stays open after guest halts so you can read VGA output)
bash scripts/run-qemu.sh --headful

# Headful but exit as soon as the guest finishes (quick flash)
bash scripts/run-qemu.sh --headful --exit-on-guest-done

# Run a specific image
bash scripts/run-qemu.sh --headless --image build/kernel.elf
```

**Auto-selection:** When `--image` is omitted, the script scans the build output directory (`build/`) for runnable `*.elf` and `*.bin` files and groups them by **stem** (filename without suffix). Co-emitted format variants of the same build count as one logical image — for example `kernel.elf` and `kernel.bin` are a single `kernel` image, and the runner prefers `.elf` when both exist.

| Build output contents | Auto-select behavior |
|-----------------------|----------------------|
| `kernel.elf` + `kernel.bin` only | Selects `kernel` (uses `.elf`) |
| No `*.elf` or `*.bin` | Fails: no runnable image; run `make build` |
| `v1` + `v2` stems (e.g. `v1.elf`, `v2.elf`) | Fails: lists ambiguous logical identities; pass `--image` |

`make test` invokes `--headless` (exits when the guest finishes). `make run` invokes `--headful` (holds the window open after the guest halts).

**Guest shutdown behavior:** By default, headless runs attach QEMU's debug-exit device so the emulator closes when the bootstrap kernel finishes — ideal for `make test`. Headful runs omit that device so the guest halts in place and the window stays open until you close it. Override with `--exit-on-guest-done` or `--hold` on either mode.

### Build environment variables

| Variable | Default | Purpose |
|----------|---------|---------|
| `AKOYA_BUILD_MEM_LIMIT_MB` | `4096` | Virtual-memory ceiling for compilation |
| `AKOYA_CROSS_PREFIX` | `i686-elf-` | Toolchain command prefix |

Successful builds print an `AKOYA_BUILD_RESULT=...` summary line and write `build/build.log`.

### QEMU test environment variables

| Variable | Default | Purpose |
|----------|---------|---------|
| `AKOYA_QEMU_TIMEOUT_SEC` | `30` | Max seconds before smoke test fails (headless, exit-on-guest-done) |
| `AKOYA_BOOTSTRAP_MESSAGE` | `akoya_unikernel bootstrap ok` | Expected console output |

## Bare-metal boot (manual)

Automated USB/GRUB image creation is deferred. To boot on the Akoya:

1. Build the kernel: `make build` → `build/kernel.bin`.
2. Prepare a bootable USB with GRUB (BIOS/Legacy mode).
3. Add a `multiboot` entry pointing at `kernel.bin` on the USB.
4. Boot the Akoya from USB; connect a USB-serial adapter to COM1 or use the laptop panel for VGA text output.
5. Confirm the bootstrap message and `build_id=` line appear on serial and/or VGA.

## OpenSpec Flow

This repo uses **OpenSpec Flow (OSF)** for structured propose → apply → finish workflows.

| Path | Role |
|------|------|
| **`OPENSPEC_FLOW.md`** | Workflow overview and bundle version. |
| **`AGENTS.md`** | Agent rules and `openspec/` discipline. |
| **`openspec/`** | Living specs and active changes. |

Use **`/osf-explore`**, **`/osf-propose`**, and **`/osf-apply-changes`** with Cursor Task subagents.
