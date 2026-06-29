## ADDED Requirements

### Requirement: Packaged ISO verification under emulation

The project SHALL provide a documented development-workstation entry point that boots the packaged ISO under 32-bit x86 emulation and verifies that the guest booted successfully and completed wired-network bring-up. Pass for this entry point SHALL require the bootstrap diagnostic message and a successful outbound connectivity probe to the build-configured probe target. It SHALL NOT require chat-completion exchange or inference-endpoint availability beyond what the connectivity probe itself needs.

#### Scenario: Headless ISO boot smoke test

- **GIVEN** a successfully produced boot ISO on the development workstation
- **AND** workstation emulation and LAN attachment prerequisites are satisfied
- **AND** the workstation LAN provides DHCP and allows the guest connectivity probe to succeed
- **WHEN** an agent or human invokes the documented ISO verification entry point in headless mode
- **THEN** the emulator boots from the ISO as optical media rather than loading a flat kernel image directly
- **AND** captured console output contains the bootstrap diagnostic message
- **AND** captured console output shows a successful outbound connectivity probe result for the build-configured probe target
- **AND** the process exits with success without running multi-turn chat regression

#### Scenario: ISO verification does not require inference endpoint

- **GIVEN** the configured chat or inference endpoint is unreachable from the development workstation
- **AND** the workstation LAN still allows the guest connectivity probe to succeed
- **WHEN** the ISO verification entry point runs in headless mode
- **THEN** verification may still pass when bootstrap and connectivity-probe assertions succeed
- **AND** failure is not reported solely because chat-completion infrastructure is unavailable

#### Scenario: ISO verification without existing ISO

- **GIVEN** no boot ISO artifact is present in the project build output area
- **WHEN** the ISO verification entry point is invoked
- **THEN** it produces the ISO via the documented packaging entry point before starting emulation
- **OR** it exits with non-zero status if packaging fails

#### Scenario: ISO boot failure is actionable

- **GIVEN** a corrupt or non-booting ISO artifact, or a booted guest that never reports connectivity-probe success
- **WHEN** the ISO verification entry point runs in headless mode
- **THEN** the runner exits with non-zero status within the configured timeout
- **AND** failure output indicates which expected bootstrap or connectivity-probe output was not observed after booting from the ISO

### Requirement: ISO verification preserves LAN-attached emulation

When verifying a packaged ISO, the development test runner SHALL attach the guest wired interface to the workstation LAN using the same mandatory LAN attachment behavior as direct-kernel runs.

#### Scenario: ISO boot on workstation LAN

- **GIVEN** ISO verification runs in headless mode with LAN prerequisites satisfied
- **WHEN** emulation starts from the packaged ISO
- **THEN** the guest wired interface is on the workstation LAN
- **AND** network diagnostic output remains observable in captured console output when the LAN provides DHCP
