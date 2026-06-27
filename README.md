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

Development workstation (Linux assumed for v1 scripts):

- **Cross toolchain:** `i686-elf-gcc` and `i686-elf-binutils` (or set `AKOYA_CROSS_PREFIX` for an equivalent prefix). On Debian/Ubuntu:

  ```bash
  sudo apt install gcc-i686-linux-gnu binutils-i686-linux-gnu
  export AKOYA_CROSS_PREFIX=i686-linux-gnu-
  ```

  For freestanding bare-metal builds, an `i686-elf-*` toolchain is preferred (see [OSDev GCC Cross-Compiler](https://wiki.osdev.org/GCC_Cross-Compiler)).

- **QEMU:** `qemu-system-i386` for smoke tests (`qemu-system-x86` package on Debian/Ubuntu).

- **Optional:** Docker — when the cross compiler or QEMU is not installed locally, `scripts/build.sh` and `scripts/run-qemu.sh` can fall back to documented container images (`AKOYA_BUILD_DOCKER_IMAGE`, `AKOYA_QEMU_DOCKER_IMAGE`).

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

# Interactive display window
bash scripts/run-qemu.sh --headful

# Headful via VNC (useful in Docker or on remote workstations without native X11)
bash scripts/run-qemu.sh --headful --vnc
# Connect with a VNC client to vnc://127.0.0.1:5900

# Run a specific image
bash scripts/run-qemu.sh --headless --image build/kernel.elf
```

**Auto-selection:** When `--image` is omitted, the script scans the build output directory (`build/`) for runnable `*.elf` and `*.bin` files and groups them by **stem** (filename without suffix). Co-emitted format variants of the same build count as one logical image — for example `kernel.elf` and `kernel.bin` are a single `kernel` image, and the runner prefers `.elf` when both exist.

| Build output contents | Auto-select behavior |
|-----------------------|----------------------|
| `kernel.elf` + `kernel.bin` only | Selects `kernel` (uses `.elf`) |
| No `*.elf` or `*.bin` | Fails: no runnable image; run `make build` |
| `v1` + `v2` stems (e.g. `v1.elf`, `v2.elf`) | Fails: lists ambiguous logical identities; pass `--image` |

`make test` invokes `--headless`; `make run` invokes `--headful`.

### Build environment variables

| Variable | Default | Purpose |
|----------|---------|---------|
| `AKOYA_BUILD_MEM_LIMIT_MB` | `4096` | Virtual-memory ceiling for compilation |
| `AKOYA_CROSS_PREFIX` | `i686-elf-` | Toolchain command prefix |
| `AKOYA_BUILD_USE_DOCKER` | `1` | Use Docker when local cross GCC is missing |

Successful builds print an `AKOYA_BUILD_RESULT=...` summary line and write `build/build.log`.

### QEMU test environment variables

| Variable | Default | Purpose |
|----------|---------|---------|
| `AKOYA_QEMU_TIMEOUT_SEC` | `30` | Max seconds before smoke test fails |
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
