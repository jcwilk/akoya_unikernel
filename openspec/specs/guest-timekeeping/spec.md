# guest-timekeeping Specification

## Purpose
TBD - created by archiving change akoya-harness-hardware-parity. Update Purpose after archive.
## Requirements
### Requirement: Millisecond delays reflect configured duration

Guest millisecond timing used for bounded waits—including timed idle gaps, DHCP timeouts, transport timeouts, and scheduled event-runtime deadlines—SHALL derive from a runtime-calibrated time base appropriate to the executing CPU, not from a fixed constant assumed at compile time. Runtime deadlines SHALL be delivered through the guest event runtime rather than busy-wait loops that monopolize the CPU until expiry.

#### Scenario: Calibrated delay on deployment hardware

- **GIVEN** a bootstrap or regression image running on deployment-class bare metal
- **WHEN** the guest schedules a multi-second bounded delay through the event runtime
- **THEN** elapsed wall time is within ten percent of the configured duration under normal single-core load during bootstrap

#### Scenario: Calibrated delay under faithful CPU emulation

- **GIVEN** the development test runner emulates the deployment CPU class from the deployment profile
- **WHEN** the same bounded delay runs under emulation through the event runtime
- **THEN** elapsed wall time is within ten percent of the configured duration
- **AND** timed-gap regression gaps are not systematically shortened or lengthened by an order of magnitude relative to the configured value

#### Scenario: Idle gap does not busy-wait

- **GIVEN** a configured inter-turn or inter-phase millisecond wait is active
- **WHEN** no other interface work is pending
- **THEN** the guest idles until the deadline rather than spinning at full CPU utilization for the entire gap duration
- **AND** the elapsed gap still meets the configured duration within the ten percent tolerance

### Requirement: Time base initialized before network delays

The guest SHALL initialize its millisecond time base during early boot before any network bootstrap or regression logic relies on `time_delay_ms` or equivalent millisecond APIs.

#### Scenario: Network bootstrap uses calibrated time

- **GIVEN** a boot image that performs DHCP and connectivity probing
- **WHEN** network bootstrap begins
- **THEN** millisecond timing has already been calibrated for the running CPU
- **AND** DHCP and probe timeouts use the calibrated base

