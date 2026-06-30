## ADDED Requirements

### Requirement: Millisecond delays reflect configured duration

Guest millisecond timing used for bounded waits (including timed idle gaps, DHCP timeouts, and link polling) SHALL derive from a runtime-calibrated time base appropriate to the executing CPU, not from a fixed constant assumed at compile time.

#### Scenario: Calibrated delay on deployment hardware

- **GIVEN** a bootstrap or regression image running on deployment-class bare metal
- **WHEN** the guest requests a multi-second bounded delay
- **THEN** elapsed wall time is within ten percent of the configured duration under normal single-core load during bootstrap

#### Scenario: Calibrated delay under faithful CPU emulation

- **GIVEN** the development test runner emulates the deployment CPU class from the deployment profile
- **WHEN** the same bounded delay runs under emulation
- **THEN** elapsed wall time is within ten percent of the configured duration
- **AND** timed-gap regression gaps are not systematically shortened or lengthened by an order of magnitude relative to the configured value

### Requirement: Time base initialized before network delays

The guest SHALL initialize its millisecond time base during early boot before any network bootstrap or regression logic relies on `time_delay_ms` or equivalent millisecond APIs.

#### Scenario: Network bootstrap uses calibrated time

- **GIVEN** a boot image that performs DHCP and connectivity probing
- **WHEN** network bootstrap begins
- **THEN** millisecond timing has already been calibrated for the running CPU
- **AND** DHCP and probe timeouts use the calibrated base
