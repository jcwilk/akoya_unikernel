## ADDED Requirements

### Requirement: Unprivileged run and verification entry points

Documented development-workstation entry points for emulation and boot-media verification SHALL NOT invoke superuser privilege elevation during normal operation.

When workstation LAN attachment requires host capabilities the current user lacks, the entry point SHALL fail fast with actionable guidance for one-time workstation setup performed outside the script, rather than elevating privileges inside the script.

#### Scenario: Run entry point never elevates

- **GIVEN** an unprivileged user invokes a documented run or boot-media verification entry point
- **WHEN** the script executes
- **THEN** it does not invoke superuser privilege elevation or privilege-elevation wrappers
- **AND** emulation or verification proceeds when prerequisites are satisfied for the current user

#### Scenario: LAN setup unavailable without elevation

- **GIVEN** macvtap LAN attachment prerequisites are not satisfied for the unprivileged user
- **WHEN** a documented run entry point that requires LAN attachment is invoked
- **THEN** it exits immediately with non-zero status
- **AND** error output describes one-time workstation setup the operator must perform
- **AND** the script does not attempt superuser privilege elevation as a fallback

#### Scenario: Boot-media verification builds without elevation

- **GIVEN** a boot-media verification entry point is invoked and the packaged artifact is missing
- **WHEN** the verification flow triggers packaging
- **THEN** packaging runs without superuser privilege elevation
