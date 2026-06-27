## ADDED Requirements

### Requirement: Native workstation toolchain

The build entry point SHALL compile and link using a cross-compilation toolchain available on the development workstation (system installation or repository-vendored tools on the host PATH).

The build entry point SHALL fail fast with an actionable error when the required toolchain is not available.

The build entry point SHALL NOT rely on containerized toolchain fallback.

#### Scenario: Toolchain present on workstation

- **GIVEN** the required cross-compilation tools are available on the development workstation
- **WHEN** the build entry point is invoked
- **THEN** compilation and linking proceed without spawning a containerized toolchain environment

#### Scenario: Toolchain absent

- **GIVEN** the required cross-compilation tools are not available on the development workstation
- **WHEN** the build entry point is invoked
- **THEN** it exits immediately with a non-zero status
- **AND** the error indicates what toolchain must be made available
