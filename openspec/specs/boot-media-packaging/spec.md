# boot-media-packaging Specification

## Purpose
TBD - created by archiving change add-bootable-iso-media. Update Purpose after archive.
## Requirements
### Requirement: Single packaging entry point for bootable ISO

The project SHALL expose one documented packaging entry point that produces a BIOS/Legacy-bootable ISO containing the main interactive chat unikernel, without requiring manual bootloader configuration by the operator at packaging time.

#### Scenario: Successful packaging after build

- **GIVEN** the repository is checked out on the development workstation with packaging prerequisites satisfied
- **AND** a successful main unikernel build exists or can be produced by the packaging entry point
- **WHEN** an agent or human invokes the documented packaging entry point
- **THEN** the process completes with a success indication
- **AND** the output includes a stable reference to the produced ISO artifact suitable for imaging to removable or internal boot media

#### Scenario: Packaging without built kernel

- **GIVEN** no bootable main unikernel artifact is present
- **WHEN** the packaging entry point is invoked
- **THEN** it produces the required kernel build before creating the ISO
- **OR** it exits with a non-zero status and actionable error output explaining that the kernel build failed

#### Scenario: Missing packaging prerequisites

- **GIVEN** required host tooling for ISO creation is not available on the development workstation
- **WHEN** the packaging entry point is invoked
- **THEN** it exits immediately with a non-zero status
- **AND** the error output explains what must be installed or configured on the workstation

### Requirement: ISO matches deployment boot protocol

The produced ISO SHALL boot the main unikernel using the deployment target's Multiboot1 boot protocol under BIOS/Legacy firmware.

#### Scenario: Legacy BIOS boot chain

- **GIVEN** a successfully produced ISO
- **WHEN** firmware loads the ISO using BIOS/Legacy boot (including boot-from-USB after the ISO is written to removable media)
- **THEN** control transfers to a bootloader that loads the main unikernel via Multiboot1
- **AND** the unikernel begins execution without requiring UEFI firmware

### Requirement: ISO suitable for removable and internal boot media

The produced ISO SHALL be usable as a whole-device image for writing to removable USB flash storage and for writing to internal boot storage on target-class hardware.

#### Scenario: Removable flash deployment

- **GIVEN** an operator writes the produced ISO to a USB flash device using a standard whole-device imaging process
- **WHEN** the Akoya-class notebook boots with BIOS/Legacy firmware and selects the USB device
- **THEN** the same bootloader and main unikernel payload start as when the ISO is booted from virtual optical media

#### Scenario: Internal boot storage deployment

- **GIVEN** an operator writes the produced ISO to internal boot storage using a standard imaging process
- **WHEN** the Akoya-class notebook boots with BIOS/Legacy firmware and selects that storage device
- **THEN** the same bootloader and main unikernel payload start as when booting from removable flash

### Requirement: Structured packaging outcome for agents

The packaging entry point SHALL emit a machine-parseable summary of the packaging outcome on completion.

#### Scenario: Successful packaging summary

- **GIVEN** a successful packaging run
- **WHEN** the packaging entry point finishes
- **THEN** the summary includes a success indicator
- **AND** the summary includes the ISO artifact reference

#### Scenario: Packaging failure summary

- **GIVEN** a packaging failure at any stage
- **WHEN** the packaging entry point finishes
- **THEN** the process exits with a non-zero status
- **AND** the summary or error output identifies the failing stage in human-readable form

### Requirement: Production kernel as default payload

The packaging entry point SHALL include the main interactive chat unikernel as the default boot payload. It SHALL NOT substitute the transport-test image unless explicitly selected by a documented override intended for development diagnostics.

#### Scenario: Default ISO payload

- **GIVEN** packaging is invoked without a diagnostic override
- **WHEN** the ISO is produced
- **THEN** the default boot entry loads the main interactive chat unikernel build
- **AND** transport-test is not the default boot target

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

