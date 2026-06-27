## MODIFIED Requirements

### Requirement: QEMU smoke-test entry point

The project SHALL provide a single documented development-workstation entry point that runs a built boot image under emulation. The caller SHALL specify either headful or headless execution mode on every invocation; the entry point SHALL NOT assume a default mode when none is given.

#### Scenario: Headless run after build

- **GIVEN** a successful bootstrap build on the development workstation
- **WHEN** the documented run entry point is invoked in headless mode
- **THEN** the boot image starts under emulation without an interactive display
- **AND** the process exits with success when the bootstrap diagnostic message is observed

#### Scenario: Headful run after build

- **GIVEN** a successful bootstrap build on the development workstation
- **WHEN** the documented run entry point is invoked in headful mode
- **THEN** the boot image starts under emulation with an interactive or remotely viewable display
- **AND** console output remains available for observation

#### Scenario: Missing display mode

- **GIVEN** a caller invokes the run entry point without specifying headful or headless mode
- **WHEN** the entry point parses arguments
- **THEN** it exits immediately with a non-zero status
- **AND** the error explains that a display mode is required

#### Scenario: Conflicting display modes

- **GIVEN** a caller specifies both headful and headless mode
- **WHEN** the entry point parses arguments
- **THEN** it exits immediately with a non-zero status
- **AND** the error explains that exactly one display mode must be chosen

### Requirement: Capturable console output

In headless mode, the run entry point SHALL attach emulated serial console output so the bootstrap diagnostic message is visible in the runner's standard output or log. In headful mode, console output SHALL remain observable through the display and/or serial attachment without requiring automated capture for pass/fail.

#### Scenario: Headless diagnostic message assertion

- **GIVEN** a correctly built bootstrap image
- **WHEN** the run entry point completes successfully in headless mode
- **THEN** the captured output contains the bootstrap diagnostic message

#### Scenario: Headful observation

- **GIVEN** a correctly built bootstrap image
- **WHEN** the run entry point is invoked in headful mode
- **THEN** an observer can read the bootstrap diagnostic message from the interactive session without relying on automated pass/fail gating

### Requirement: Bounded test runtime

In headless mode, the run entry point SHALL terminate within a bounded time when the image fails to produce the diagnostic message, rather than hanging indefinitely. Headful mode SHALL NOT apply the same automated timeout gate.

#### Scenario: Headless boot hang detection

- **GIVEN** a corrupt or non-booting image artifact
- **WHEN** the run entry point runs in headless mode
- **THEN** the runner exits with non-zero status within the configured timeout
- **AND** the failure mode indicates that expected console output was not observed

#### Scenario: Headful session control

- **GIVEN** the run entry point is invoked in headful mode
- **WHEN** the emulated machine is running
- **THEN** the session continues until the user or operator ends it
- **AND** no automated smoke-test timeout forces termination solely for pass/fail scoring

## ADDED Requirements

### Requirement: Explicit boot image selection

The run entry point SHALL accept an optional path identifying which built boot image to run. When the path is omitted, the entry point SHALL resolve the image from the project's build output area.

#### Scenario: Caller supplies image path

- **GIVEN** a valid boot image path is provided
- **WHEN** the run entry point is invoked in either display mode
- **THEN** that image is the one started under emulation

#### Scenario: Exactly one runnable image in output area

- **GIVEN** no image path is provided
- **AND** the build output area contains exactly one runnable boot image
- **WHEN** the run entry point is invoked
- **THEN** that sole runnable image is selected automatically

#### Scenario: No runnable images

- **GIVEN** no image path is provided
- **AND** the build output area contains zero runnable boot images
- **WHEN** the run entry point is invoked
- **THEN** it exits immediately with a non-zero status
- **AND** the error explains that no runnable image was found and a build is required

#### Scenario: Multiple runnable images

- **GIVEN** no image path is provided
- **AND** the build output area contains more than one runnable boot image
- **WHEN** the run entry point is invoked
- **THEN** it exits immediately with a non-zero status
- **AND** the error explains that the image path must be specified
- **AND** the error enumerates the ambiguous candidates so the caller can choose
