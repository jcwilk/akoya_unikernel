## MODIFIED Requirements

### Requirement: Conservative hardware assumptions

Boot images in this change SHALL assume modest single-core resources comparable to a legacy notebook (on the order of two gigabytes system RAM). Bootstrap images SHALL require wired Ethernet and basic IPv4 connectivity via DHCP to complete their declared diagnostic behavior on target-class hardware. Interactive chat on bare metal SHALL use the notebook integrated keyboard as the operator input device.

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
