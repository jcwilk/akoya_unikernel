## ADDED Requirements

### Requirement: Akoya bare-metal deployment profile

The system SHALL treat the Medion Akoya EX notebook class hardware as the sole deployment target for produced boot images in this change.

#### Scenario: Target hardware class

- **GIVEN** a boot image produced by the build pipeline
- **WHEN** the image is deployed according to project documentation
- **THEN** the image is intended to execute on 32-bit x86 bare metal consistent with Pentium M–class notebooks (single core, no hardware virtualization dependency)

### Requirement: No virtualization requirement on target

Produced boot images SHALL NOT require hardware virtualization extensions on the deployment target.

#### Scenario: Target without VT-x

- **GIVEN** the deployment CPU does not expose hardware virtualization
- **WHEN** the bootstrap image boots on bare metal
- **THEN** boot and runtime do not depend on a hypervisor or hardware virtualization feature

### Requirement: Instruction set compatibility

Boot images SHALL be compatible with i686-class 32-bit x86 execution and SHALL NOT require 64-bit x86 instructions.

#### Scenario: 32-bit-only CPU

- **GIVEN** the deployment CPU executes only 32-bit x86 instructions
- **WHEN** the bootstrap image runs on bare metal
- **THEN** execution does not require x86-64 mode or 64-bit-only instructions

### Requirement: Conservative hardware assumptions

Boot images in this change SHALL assume modest single-core resources comparable to a legacy notebook (on the order of two gigabytes system RAM) and SHALL avoid requiring modern optional platform features beyond basic boot and console output.

#### Scenario: Minimal platform footprint

- **GIVEN** the bootstrap image is built for the Akoya profile
- **WHEN** it boots on target-class hardware
- **THEN** it does not require network, storage beyond the boot medium, discrete GPU drivers, or multi-core scheduling to complete its diagnostic behavior
