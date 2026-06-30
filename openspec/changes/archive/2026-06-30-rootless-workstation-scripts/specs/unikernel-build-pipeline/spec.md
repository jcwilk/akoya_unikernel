## ADDED Requirements

### Requirement: Unprivileged build and packaging invocation

The documented build entry point and all documented boot-media packaging entry points SHALL run to completion without superuser privileges when invoked by an unprivileged user with satisfied toolchain and packaging prerequisites.

#### Scenario: Build entry point without elevation

- **GIVEN** an unprivileged user with a satisfied cross-compilation toolchain
- **WHEN** the documented build entry point is invoked
- **THEN** compilation and linking complete without superuser privilege elevation

#### Scenario: Packaging after build without elevation

- **GIVEN** an unprivileged user who has successfully built the main unikernel
- **WHEN** a documented boot-media packaging entry point is invoked
- **THEN** packaging completes without superuser privilege elevation
