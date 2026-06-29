## ADDED Requirements

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
