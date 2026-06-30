## MODIFIED Requirements

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

## ADDED Requirements

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
