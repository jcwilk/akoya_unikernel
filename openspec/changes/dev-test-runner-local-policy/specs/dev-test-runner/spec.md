## MODIFIED Requirements

### Requirement: QEMU smoke-test entry point

The project SHALL provide a single documented development-workstation entry point that runs a built boot image under emulation. The caller SHALL specify either headful or headless execution mode on every invocation; the entry point SHALL NOT assume a default mode when none is given.

#### Scenario: Headless run after build

- **GIVEN** a successful bootstrap build on the development workstation
- **WHEN** the documented run entry point is invoked in headless mode
- **THEN** the boot image starts under emulation without an interactive display
- **AND** the process exits with success when the bootstrap diagnostic message is observed

#### Scenario: Headful run after build

- **GIVEN** a successful bootstrap build on the development workstation
- **WHEN** the documented run entry point is invoked in headful mode
- **THEN** the boot image starts under emulation with an interactive display on the development workstation
- **AND** console output remains available for observation

#### Scenario: Missing display mode

- **GIVEN** a caller invokes the run entry point without specifying headful or headless mode
- **WHEN** the entry point parses arguments
- **THEN** it exits immediately with a non-zero status
- **AND** the error explains that a display mode is required

#### Scenario: Conflicting display modes

- **GIVEN** a caller specifies both headful and headless mode
- **WHEN** the entry point parses arguments
- **THEN** it exits immediately with a non-zero status
- **AND** the error explains that exactly one display mode must be chosen

## ADDED Requirements

### Requirement: Guest shutdown behavior

The run entry point SHALL support configurable behavior for whether the emulator exits when the guest finishes its bootstrap work.

By default, headless invocations SHALL cause the emulator to exit after the guest completes bootstrap diagnostics. By default, headful invocations SHALL keep the emulator running after the guest halts so the operator can read display output and close the session manually.

The caller SHALL be able to override the default for either display mode with an explicit shutdown-behavior choice.

#### Scenario: Headless default auto-exit

- **GIVEN** a successful bootstrap build
- **WHEN** the run entry point is invoked in headless mode without a shutdown-behavior override
- **THEN** the emulator exits after the bootstrap diagnostic message is observed
- **AND** the invocation can be used as an automated smoke test

#### Scenario: Headful default hold-open

- **GIVEN** a successful bootstrap build
- **WHEN** the run entry point is invoked in headful mode without a shutdown-behavior override
- **THEN** the emulator remains open after the guest halts
- **AND** the operator can read the display output before ending the session

#### Scenario: Override to hold on headless

- **GIVEN** a caller requests hold-open shutdown behavior on a headless invocation
- **WHEN** the run entry point starts emulation
- **THEN** the emulator does not automatically exit solely because the guest halted

#### Scenario: Override to auto-exit on headful

- **GIVEN** a caller requests auto-exit shutdown behavior on a headful invocation
- **WHEN** the guest completes bootstrap diagnostics
- **THEN** the emulator may exit without waiting for manual session termination

### Requirement: Local workstation emulator

The run entry point SHALL invoke a 32-bit x86 system emulator installed on the development workstation. It SHALL fail fast with an actionable error when that emulator is not available.

The run entry point SHALL NOT rely on containerized emulator fallback.

#### Scenario: Emulator present

- **GIVEN** a host-installed 32-bit x86 system emulator is available
- **WHEN** the run entry point is invoked with prerequisites otherwise satisfied
- **THEN** emulation starts using the host installation

#### Scenario: Emulator absent

- **GIVEN** no suitable host-installed emulator is available
- **WHEN** the run entry point is invoked
- **THEN** it exits immediately with a non-zero status
- **AND** the error indicates that a host emulator must be installed
