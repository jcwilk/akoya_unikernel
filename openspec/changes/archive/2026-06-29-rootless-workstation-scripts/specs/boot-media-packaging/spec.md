## ADDED Requirements

### Requirement: Unprivileged packaging entry points

All documented boot-media packaging entry points SHALL complete successfully when invoked by an unprivileged user account, provided required host tools are installed and a successful kernel build exists or can be produced without elevation.

The packaging flow SHALL NOT invoke superuser privilege elevation, loop-device mounts requiring elevated privileges, or block-device operations that require root on the development workstation.

#### Scenario: USB disk image packaging without elevation

- **GIVEN** an unprivileged user on the development workstation with packaging prerequisites satisfied
- **WHEN** the documented USB/HDD disk-image packaging entry point is invoked
- **THEN** the process completes with a success indication
- **AND** the produced disk image is suitable for legacy-BIOS removable media imaging tools
- **AND** no superuser privilege elevation occurred during the run

#### Scenario: ISO packaging remains unprivileged

- **GIVEN** an unprivileged user on the development workstation with ISO packaging prerequisites satisfied
- **WHEN** the documented ISO packaging entry point is invoked
- **THEN** the process completes without superuser privilege elevation

#### Scenario: Missing prerequisites without elevation

- **GIVEN** a required host tool is absent
- **WHEN** any packaging entry point is invoked by an unprivileged user
- **THEN** the process exits with non-zero status and actionable error output
- **AND** the error does not instruct the user to re-run the same script under superuser privileges

### Requirement: Legacy-BIOS USB disk image packaging

The project SHALL expose a documented packaging entry point that produces a raw disk image for legacy-BIOS USB or internal HDD imaging (including third-party imagers such as Etcher), containing the main interactive chat unikernel with Multiboot1 boot via GRUB on an MBR-partitioned layout.

#### Scenario: Successful disk image for removable media

- **GIVEN** packaging prerequisites satisfied and a successful main unikernel build
- **WHEN** the disk-image packaging entry point completes
- **THEN** the output includes a stable reference to a raw disk image artifact
- **AND** the artifact is intended for whole-device imaging to removable USB flash on target-class legacy BIOS hardware

#### Scenario: Production kernel as disk-image payload

- **GIVEN** disk-image packaging is invoked without a diagnostic override
- **WHEN** the image is produced
- **THEN** the default boot entry loads the main interactive chat unikernel
- **AND** transport-test is not the default boot target
