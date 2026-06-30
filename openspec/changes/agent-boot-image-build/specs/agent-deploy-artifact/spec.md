## ADDED Requirements

### Requirement: Deploy media built on implementation batch completion

When an agent completes an approved implementation batch that modifies bootable unikernel artifacts and prepares a human-facing handoff for deployment review, the agent SHALL produce a fresh legacy-BIOS USB disk image containing the main interactive chat unikernel before claiming the batch is ready to flash to removable media.

#### Scenario: Successful completion includes fresh deploy media

- **GIVEN** an agent has finished implementation tasks for an approved change that affects bootable unikernel output
- **WHEN** the agent prepares its completion handoff to the human
- **THEN** a fresh USB disk image packaging run has completed successfully for the current workspace state
- **AND** the handoff does not describe the batch as ready to flash unless that packaging run succeeded

#### Scenario: Packaging failure blocks flash-ready claim

- **GIVEN** USB disk image packaging fails after implementation tasks are otherwise complete
- **WHEN** the agent prepares its completion handoff
- **THEN** the handoff states that deploy media packaging failed
- **AND** the handoff does not claim the batch is ready to flash
- **AND** the human can identify the failing stage from the handoff output

### Requirement: Stale deploy media invalidated before rebuild

When an agent begins a fresh deploy media build for a completed implementation batch, the agent SHALL remove or invalidate the prior flash-ready disk image in the project build output area before the new packaging run starts, so a superseded image cannot be selected while a new build is in progress or after kernel changes without a successful rebuild.

#### Scenario: Prior image removed at packaging start

- **GIVEN** a flash-ready disk image from an earlier build exists in the build output area
- **WHEN** the agent starts packaging deploy media for the current batch
- **THEN** the prior flash-ready disk image is no longer present at its previous location before the new packaging run proceeds
- **AND** only a successfully completed new packaging run restores a flash-ready path

#### Scenario: Failed rebuild leaves no misleading flash path

- **GIVEN** the agent removed a prior flash-ready disk image and the new packaging run fails
- **WHEN** the human inspects the build output area
- **THEN** no complete prior flash-ready disk image remains at the canonical flash location
- **AND** the completion handoff explains that flashing should wait for a successful rebuild

### Requirement: Prominent flash path in human handoff

When deploy media packaging succeeds, the agent's human-facing completion handoff SHALL present the absolute path to the flash-ready disk image in a dedicated, prominently placed section separate from implementation narrative, suitable for copy-paste into an imaging tool.

#### Scenario: Flash path above implementation detail

- **GIVEN** deploy media packaging completed successfully
- **WHEN** the human reads the agent completion handoff
- **THEN** a clearly labeled flash section appears before lengthy implementation summaries
- **AND** the section contains the absolute path to the disk image intended for whole-device imaging

#### Scenario: Machine-parseable deploy outcome

- **GIVEN** deploy media packaging completed
- **WHEN** an automated reader inspects the agent output
- **THEN** a single-line structured summary indicates success and includes the flash-ready artifact reference
- **AND** the structured summary uses the same artifact reference as the prominent human section

### Requirement: Asynchronous packaging without false-ready debrief

Agents MAY run deploy media packaging concurrently with other completion work such as verification notes, provided stale artifacts are invalidated first. The agent SHALL await packaging completion before claiming the batch is ready to flash, and SHALL report in-progress packaging state if the human receives a partial update before packaging finishes.

#### Scenario: Background packaging completes before flash-ready claim

- **GIVEN** the agent started deploy media packaging in a concurrent worker
- **WHEN** the agent states the batch is ready to flash
- **THEN** the concurrent packaging worker has already completed successfully
- **AND** the prominent flash path reflects the newly built artifact

#### Scenario: Partial handoff reports packaging in progress

- **GIVEN** deploy media packaging is still running in a concurrent worker
- **WHEN** the agent sends an interim update before packaging completes
- **THEN** the update states that packaging is in progress
- **AND** the update does not claim the batch is ready to flash
