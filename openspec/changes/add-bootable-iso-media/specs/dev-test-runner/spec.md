## ADDED Requirements

### Requirement: Packaged ISO verification under emulation

The project SHALL provide a documented development-workstation entry point that boots the packaged ISO under 32-bit x86 emulation and verifies that the guest reaches the same observable bootstrap success as the direct-kernel smoke path.

#### Scenario: Headless ISO boot smoke test

- **GIVEN** a successfully produced boot ISO on the development workstation
- **AND** workstation emulation and LAN attachment prerequisites are satisfied
- **WHEN** an agent or human invokes the documented ISO verification entry point in headless mode
- **THEN** the emulator boots from the ISO as optical media rather than loading a flat kernel image directly
- **AND** captured console output contains the bootstrap diagnostic message
- **AND** the process exits with success when bootstrap and configured smoke assertions pass

#### Scenario: ISO verification without existing ISO

- **GIVEN** no boot ISO artifact is present in the project build output area
- **WHEN** the ISO verification entry point is invoked
- **THEN** it produces the ISO via the documented packaging entry point before starting emulation
- **OR** it exits with non-zero status if packaging fails

#### Scenario: ISO boot failure is actionable

- **GIVEN** a corrupt or non-booting ISO artifact
- **WHEN** the ISO verification entry point runs in headless mode
- **THEN** the runner exits with non-zero status within the configured timeout
- **AND** failure output indicates that expected bootstrap console output was not observed after booting from the ISO

### Requirement: ISO verification preserves LAN-attached emulation

When verifying a packaged ISO, the development test runner SHALL attach the guest wired interface to the workstation LAN using the same mandatory LAN attachment behavior as direct-kernel runs.

#### Scenario: ISO boot on workstation LAN

- **GIVEN** ISO verification runs in headless mode with LAN prerequisites satisfied
- **WHEN** emulation starts from the packaged ISO
- **THEN** the guest wired interface is on the workstation LAN
- **AND** network diagnostic output remains observable in captured console output when the LAN provides DHCP
