## MODIFIED Requirements

### Requirement: Conservative hardware assumptions

Boot images in this change SHALL assume modest single-core resources comparable to a legacy notebook (on the order of two gigabytes system RAM). Bootstrap images SHALL require only platform features needed for their declared diagnostic behavior: basic boot and console output for the console-only variant, and wired Ethernet plus basic IPv4 connectivity for the network-enabled bootstrap variant.

#### Scenario: Minimal platform footprint for console-only bootstrap

- **GIVEN** the console-only bootstrap variant
- **WHEN** it boots on target-class hardware
- **THEN** it does not require network, storage beyond the boot medium, discrete GPU drivers, or multi-core scheduling to complete its diagnostic behavior

#### Scenario: Network bootstrap uses wired Ethernet only

- **GIVEN** the network-enabled bootstrap variant built for the Akoya profile
- **WHEN** it boots on target-class hardware with a connected RJ-45 link and reachable DHCP
- **THEN** it can complete its network diagnostics using the wired Ethernet interface documented for the Medion Akoya EX
- **AND** it does not require wireless, modem, or Bluetooth hardware to complete its diagnostic behavior
