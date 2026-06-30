# deployment-target Specification

## Purpose
TBD - created by archiving change akoya-unikernel-build-pipeline. Update Purpose after archive.
## Requirements
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

Boot images SHALL assume modest single-core resources comparable to a legacy notebook on bare metal (on the order of two gigabytes system RAM). Pre-hardware emulation on the development workstation MAY use a smaller allocated guest memory footprint when the guest working set does not require deployment-scale RAM. Bootstrap images SHALL require wired Ethernet and basic IPv4 connectivity via DHCP to complete their declared diagnostic behavior on target-class hardware. Interactive chat on bare metal SHALL use the notebook integrated keyboard as the operator input device.

#### Scenario: Network bootstrap uses wired Ethernet only

- **GIVEN** a bootstrap image built for the Akoya profile
- **WHEN** it boots on target-class hardware with a connected RJ-45 link and reachable DHCP
- **THEN** it completes its network diagnostics using the wired Ethernet interface documented for the Medion Akoya EX
- **AND** it does not require wireless, modem, or Bluetooth hardware to complete its diagnostic behavior

#### Scenario: Required platform features for bootstrap

- **GIVEN** a bootstrap image built for the Akoya profile
- **WHEN** it boots on target-class hardware
- **THEN** it requires basic boot, console output, wired Ethernet with DHCP reachability, and integrated keyboard input to complete its full interactive chat behavior
- **AND** it does not require discrete GPU drivers or multi-core scheduling beyond what the network and chat session need

#### Scenario: Integrated keyboard for interactive chat

- **GIVEN** a bootstrap image built for the Akoya profile
- **WHEN** an operator uses interactive chat on bare metal
- **THEN** input is accepted from the notebook integrated keyboard
- **AND** external USB keyboards or serial terminals are not required for typing

#### Scenario: Emulation memory footprint may be smaller than deployment RAM

- **GIVEN** pre-hardware validation runs on the development workstation with reduced guest memory allocation
- **WHEN** smoke and regression images execute under emulation
- **THEN** pass or fail reflects bootstrap and chat behavior at the reduced allocation
- **AND** the deployment profile continues to document full deployment RAM for bare-metal expectations

### Requirement: Deployment target hardware documentation

The Medion Akoya EX deployment target SHALL be described in the project's authoritative hardware inventory governed by the deployment-hardware-inventory capability.

#### Scenario: Deployment facts traceable to inventory

- **GIVEN** the hardware inventory capability is in effect
- **WHEN** a contributor needs hardware facts about the deployment target beyond bare-metal behavioral constraints
- **THEN** they can find Medion Akoya EX hardware documentation in the authoritative inventory
- **AND** that documentation follows the inventory's Akoya EX–exclusive source confirmation rules

### Requirement: BIOS/Legacy boot from prepared removable media

Deployment boot media produced by the project SHALL support BIOS/Legacy firmware boot from removable USB flash storage without requiring the operator to manually install or configure a bootloader at deploy time.

#### Scenario: Plug-and-boot from prepared USB

- **GIVEN** an operator prepared boot media using the project's documented ISO imaging process
- **WHEN** the Medion Akoya EX boots with BIOS/Legacy firmware and the USB device selected in the boot menu
- **THEN** the main unikernel starts without additional on-device bootloader setup steps

### Requirement: BIOS/Legacy boot from prepared internal storage

Deployment boot media produced by the project SHALL support BIOS/Legacy firmware boot from internal boot storage when the ISO has been written there using the documented imaging process.

#### Scenario: Boot from imaged internal drive

- **GIVEN** an operator wrote the project ISO to internal boot storage on the target notebook
- **WHEN** the notebook boots with BIOS/Legacy firmware and selects that internal device
- **THEN** the main unikernel starts using the same boot chain as removable flash deployment

### Requirement: Deployment profile supplies CPU constants for tooling

The machine-readable deployment profile SHALL declare the deployment CPU class. Build and emulation tooling SHALL consume that declaration for CPU targeting without requiring duplicate hardcoded CPU names in multiple places.

#### Scenario: Profile CPU used by emulation

- **GIVEN** the deployment profile declares a Pentium M class CPU
- **WHEN** the development test runner starts emulation
- **THEN** the emulated CPU matches the declared class or the closest faithful model supported by the host emulator
- **AND** emulation does not default to an unrelated CPU family when the profile is present

#### Scenario: Profile remains authoritative for bare-metal RAM

- **GIVEN** the deployment profile declares deployment RAM capacity
- **WHEN** a contributor reads deployment constraints
- **THEN** bare-metal RAM expectations come from the profile and authoritative inventory
- **AND** reduced emulation RAM does not change the documented deployment RAM value

