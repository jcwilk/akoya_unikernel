# unikernel-build-pipeline Specification

## Purpose
Cross-compilation of boot images for the Akoya deployment target on the development workstation, with memory-bounded builds and agent-parseable outcomes.
## Requirements
### Requirement: Single agent-invokable build entry point

The project SHALL expose one documented build entry point that produces all artifacts needed to boot the bootstrap unikernel on the deployment target, without requiring ad-hoc manual compiler invocations.

#### Scenario: Successful build from entry point

- **GIVEN** the repository is checked out on the development workstation with prerequisites satisfied
- **WHEN** an agent or human invokes the documented build entry point
- **THEN** the build completes with a success indication
- **AND** the output includes the location of the bootable image artifact
- **AND** the output includes the location of a build log suitable for diagnosis

#### Scenario: Build failure is actionable

- **GIVEN** a compilation or link error occurs
- **WHEN** the build entry point is invoked
- **THEN** the process exits with a non-zero status
- **AND** the build log contains enough context to identify the failing stage

### Requirement: Workstation builds always target deployment ISA

The build pipeline SHALL always compile and link for the deployment target's 32-bit x86 instruction set, regardless of the development workstation's native architecture.

#### Scenario: Cross-architecture workstation

- **GIVEN** the development workstation uses a different native word size than the deployment target
- **WHEN** the build entry point runs
- **THEN** the produced boot image is encoded for 32-bit x86 execution on the deployment target
- **AND** no build step treats a native workstation binary as the deliverable boot image

### Requirement: Compilation memory ceiling

The build pipeline SHALL enforce a configurable upper bound on virtual memory used by the compilation process, defaulting to approximately four gigabytes.

#### Scenario: Build stays within limit

- **GIVEN** the default memory ceiling is in effect
- **WHEN** a normal bootstrap build runs on the development workstation
- **THEN** peak compilation memory remains at or below the configured ceiling
- **AND** the build completes successfully

#### Scenario: Build exceeds limit

- **GIVEN** the memory ceiling is set below what compilation requires
- **WHEN** the build entry point runs
- **THEN** the build terminates without producing a bootable image
- **AND** the failure is reported as a memory-limit violation distinct from compile errors

### Requirement: Structured build outcome for agents

The build entry point SHALL emit a machine-parseable summary of the build outcome on completion.

#### Scenario: Successful build summary

- **GIVEN** a successful build
- **WHEN** the build entry point finishes
- **THEN** the summary includes a success indicator
- **AND** the summary includes the boot image artifact reference
- **AND** the summary includes the build log reference

### Requirement: Native workstation toolchain

The build entry point SHALL compile and link using a cross-compilation toolchain available on the development workstation (system installation or repository-vendored tools on the host PATH).

The build entry point SHALL fail fast with an actionable error when the required toolchain is not available.

The build entry point SHALL NOT rely on containerized toolchain fallback.

#### Scenario: Toolchain present on workstation

- **GIVEN** the required cross-compilation tools are available on the development workstation
- **WHEN** the build entry point is invoked
- **THEN** compilation and linking proceed without spawning a containerized toolchain environment

#### Scenario: Toolchain absent

- **GIVEN** the required cross-compilation tools are not available on the development workstation
- **WHEN** the build entry point is invoked
- **THEN** it exits immediately with a non-zero status
- **AND** the error indicates what toolchain must be made available

### Requirement: Unprivileged build and packaging invocation

The documented build entry point and all documented boot-media packaging entry points SHALL run to completion without superuser privileges when invoked by an unprivileged user with satisfied toolchain and packaging prerequisites.

#### Scenario: Build entry point without elevation

- **GIVEN** an unprivileged user with a satisfied cross-compilation toolchain
- **WHEN** the documented build entry point is invoked
- **THEN** compilation and linking complete without superuser privilege elevation

#### Scenario: Packaging after build without elevation

- **GIVEN** an unprivileged user who has successfully built the main unikernel
- **WHEN** a documented boot-media packaging entry point is invoked
- **THEN** packaging completes without superuser privilege elevation

