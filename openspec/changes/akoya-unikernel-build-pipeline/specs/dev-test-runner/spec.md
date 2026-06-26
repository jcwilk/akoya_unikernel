## ADDED Requirements

### Requirement: QEMU smoke-test entry point

The project SHALL provide a documented development-workstation entry point that runs the latest built bootstrap image under QEMU for pre-hardware validation.

#### Scenario: Run after build

- **GIVEN** a successful bootstrap build on the development workstation
- **WHEN** the documented QEMU test entry point is invoked
- **THEN** the bootstrap image boots under emulation
- **AND** the process exits with success when the diagnostic message is observed

#### Scenario: Missing artifact

- **GIVEN** no boot image artifact exists
- **WHEN** the QEMU test entry point is invoked
- **THEN** the runner fails fast with a clear indication that a build is required first

### Requirement: Emulated target matches deployment word size

The QEMU runner SHALL emulate 32-bit x86 execution consistent with the deployment target, without requiring host hardware virtualization.

#### Scenario: Emulation without KVM dependency

- **GIVEN** the development host lacks usable hardware acceleration for the emulated target
- **WHEN** the QEMU test entry point runs
- **THEN** the smoke test still executes using software emulation
- **AND** the test remains suitable for cross-architecture workstations

### Requirement: Capturable console output

The QEMU runner SHALL attach emulated serial console output so the bootstrap diagnostic message is visible in the test runner's standard output or log.

#### Scenario: Diagnostic message assertion

- **GIVEN** a correctly built bootstrap image
- **WHEN** the QEMU test entry point completes successfully
- **THEN** the captured output contains the bootstrap diagnostic message

### Requirement: Bounded test runtime

The QEMU smoke test SHALL terminate within a bounded time when the image fails to produce the diagnostic message, rather than hanging indefinitely.

#### Scenario: Boot hang detection

- **GIVEN** a corrupt or non-booting image artifact
- **WHEN** the QEMU test entry point runs
- **THEN** the runner exits with non-zero status within the configured timeout
- **AND** the failure mode indicates that expected console output was not observed
