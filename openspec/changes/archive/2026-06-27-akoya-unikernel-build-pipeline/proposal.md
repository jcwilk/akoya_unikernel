## Why

The Medion Akoya EX notebook is a 32-bit x86 bare-metal target without hardware virtualization, so conventional VM-based iteration is unavailable. We need a reproducible, agent-friendly build pipeline on the development workstation that compiles unikernels for that target under tight memory constraints, plus a minimal first image and a QEMU path to validate builds before deploying to hardware.

## What Changes

- Introduce an agentic unikernel compilation pipeline that always builds for the Akoya target architecture on the development workstation and keeps peak compilation memory near or below 4 GB.
- Codify the Akoya EX as the deployment target profile: 32-bit x86 (i686-class), bare metal, no reliance on virtualization extensions.
- Deliver a conservative bootstrap unikernel whose only job is to prove the pipeline and runtime path (e.g., emit a short, human-readable message on boot).
- Provide a development-workstation runner that launches built images under QEMU for cross-architecture smoke testing before bare-metal deployment.
- Establish repository layout, documentation, and task structure so agents can drive builds, diagnose failures, and iterate without ad-hoc commands.

## Capabilities

### New Capabilities

- `unikernel-build-pipeline`: Agent-driven compilation workflow, memory ceiling during builds, and workstation builds that always target the Akoya ISA.
- `deployment-target`: Hardware and runtime constraints for bare-metal deployment on the Medion Akoya EX (32-bit x86, no VT-x dependency).
- `bootstrap-unikernel`: Observable behavior of the initial diagnostic unikernel image produced by the pipeline.
- `dev-test-runner`: Workstation-side execution of built images under emulation for pre-hardware validation.

### Modified Capabilities

<!-- No existing living specs. -->

## Impact

- New source tree for unikernel sources, build orchestration, and QEMU runner scripts.
- Development workstation toolchain dependencies (cross-compilation, emulation, memory-limiting wrappers).
- Project documentation (`README.md`, environment notes) updated to describe build, test, and bare-metal deploy flows.
- No existing APIs or services are affected; this is greenfield infrastructure for the `akoya_unikernel` repository.
