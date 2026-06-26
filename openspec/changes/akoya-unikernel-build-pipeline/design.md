## Context

The `akoya_unikernel` repository is greenfield: OpenSpec Flow is installed but no living specs or application code exist yet. The deployment target is a Medion Akoya EX notebook with an Intel Pentium M 735 (32-bit x86, i686-class, SSE2, single core, no VT-x), 2 GB RAM, and legacy PATA storage. The development workstation compiles images for that target and must stay within roughly 4 GB peak RAM during compilation.

Because the target lacks hardware virtualization, runtime validation on the workstation relies on QEMU user-mode/system emulation, while production runs on bare metal.

## Goals / Non-Goals

**Goals:**

- Provide a single, documented entry point agents and humans can invoke to build, test, and locate artifacts.
- Cross-compile on the workstation for 32-bit x86 regardless of the workstation's native word size.
- Enforce a configurable compilation memory ceiling (default ~4 GB) with a clear failure mode when exceeded.
- Ship a bootstrap image that prints a fixed diagnostic message over a simple console path (serial and/or VGA text).
- Ship a QEMU runner that exercises the same artifact the bare-metal path would boot.
- Emit structured build output (exit status, log location, artifact paths) suitable for agent loops.

**Non-Goals:**

- Full Unikraft/OSv-style application unikernel features in the first iteration (network stack, storage drivers beyond boot, wireless, graphics modesetting).
- Automated bare-metal flashing or PXE deploy in v1 (document manual steps only).
- Optimizing image size or boot time beyond "reliably diagnosable."
- Supporting x86-64 or multi-core SMP on the target.
- CI/CD integration or remote build farms.

## Decisions

### 1. Bootstrap runtime: minimal multiboot kernel first

**Choice:** Start with a tiny freestanding i686 multiboot1 kernel (GCC cross toolchain + linker script + GRUB-compatible boot path) rather than adopting a full unikernel framework immediately.

**Rationale:** Pentium M + no VT-x + 2 GB target RAM favors the smallest diagnosable path. A multiboot hello kernel isolates toolchain, linker, boot protocol, and console output before layering a heavier unikernel SDK. Build RAM stays well under 4 GB.

**Alternatives considered:**

| Option | Why not now |
|--------|-------------|
| Unikraft | Powerful but heavier toolchain/download footprint; i686 support and agent debuggability are weaker for a first smoke test. |
| IncludeOS / Nanos | Less aligned with bare-metal i686 notebook constraints out of the box; steeper agent iteration cost. |

**Follow-on:** A later change may introduce a framework once the pipeline and bare-metal boot path are proven.

### 2. Target ISA flags: i686 tuned for Pentium M class

**Choice:** Compile with an i686 (32-bit) toolchain and CPU tuning appropriate for Pentium M / Dothan (no x86-64, no assumption of VT-x, SSE2 as upper SIMD baseline).

**Rationale:** Matches the Akoya CPU and avoids emitting 64-bit instructions the target cannot execute.

### 3. Workstation always cross-builds for target

**Choice:** The pipeline never emits a "native workstation" production artifact. Even if the workstation is 64-bit, builds use a cross compiler and link for i686.

**Rationale:** Prevents accidental deployment of incompatible binaries; keeps agent behavior deterministic.

### 4. Memory ceiling via process limits

**Choice:** Wrap compilation in an OS-level virtual-memory limit (e.g., `ulimit -v` on Linux, or equivalent cgroup/memory wrapper) defaulting to ~4 GB, overridable via environment variable.

**Rationale:** Simple, agent-observable failure (non-zero exit, log mentions limit) without custom allocators.

### 5. Agentic entry point: `Makefile` + thin shell driver

**Choice:** Top-level `make build`, `make test` (QEMU), `make clean` backed by a small `scripts/build.sh` and `scripts/run-qemu.sh` that print a JSON or key=value summary line on success.

**Rationale:** Make is ubiquitous and easy for agents to parse; scripts hold platform-specific details.

### 6. Console output for bootstrap

**Choice:** Write the diagnostic string to serial (COM1) and mirror to VGA text mode if available; QEMU runner attaches serial to stdio.

**Rationale:** Serial is easiest to capture in QEMU and on bare metal with a null-modem or USB-serial adapter; VGA provides fallback visibility on the laptop panel.

### 7. QEMU runner profile

**Choice:** `qemu-system-i386` with multiboot loader, 512 MB–1 GB emulated RAM, `-serial stdio`, `-display none` (or optional VGA window), single CPU, no KVM requirement (KVM may be unavailable or wrong arch on dev host).

**Rationale:** Matches target word size without VT-x; works on diverse dev machines.

### 8. Repository layout

```
├── Makefile                 # agent-facing targets
├── scripts/
│   ├── build.sh             # cross-build + memory limit
│   └── run-qemu.sh          # smoke test
├── target/
│   └── akoya.profile        # machine-readable target constants (arch, CPU notes)
├── kernel/
│   ├── boot/                # multiboot header, entry
│   ├── console/             # serial + vga text
│   └── main.c               # bootstrap message
├── linker.ld
└── build/                   # gitignored artifacts
    ├── kernel.bin
    └── build.log
```

## Risks / Trade-offs

| Risk | Mitigation |
|------|------------|
| Cross toolchain not installed on workstation | Document prerequisite packages; build script fails fast with install hints. |
| 4 GB limit too tight if framework added later | Limit is env-configurable; design defers heavy frameworks. |
| QEMU behavior diverges from bare metal | Bootstrap keeps hardware touchpoints minimal; tasks include manual bare-metal checklist. |
| PATA boot requires external boot media setup | Document GRUB/multiboot USB creation; out of automated scope for v1. |
| Agent parses build output incorrectly | Emit stable `AKOYA_BUILD_RESULT=...` summary line and documented exit codes. |

## Migration Plan

Greenfield — no migration. Rollback is "delete new tree" or revert the apply branch.

**Apply sequence:**

1. Add toolchain docs and `target/akoya.profile`.
2. Implement kernel + linker + build wrapper with memory limit.
3. Implement QEMU runner.
4. Verify `make build && make test` on workstation.
5. Document bare-metal boot steps (USB/GRUB) without automating.

## Open Questions

- Exact development workstation OS/distro (assume Linux for v1 scripts).
- Whether bare-metal first boot will use GRUB on USB or network boot — default to USB multiboot image in tasks.
- Preferred diagnostic string content (default: project name + "bootstrap ok" + build id/timestamp).
