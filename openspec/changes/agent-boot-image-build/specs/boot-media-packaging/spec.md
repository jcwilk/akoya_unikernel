## ADDED Requirements

### Requirement: Canonical flash-ready disk image identity

The project SHALL designate one canonical flash-ready disk image path in the build output area for legacy-BIOS USB deployment, plus a stable secondary reference intended for third-party imaging tools when documented. Agent and human handoffs SHALL use the canonical path as the primary burn target.

#### Scenario: Handoff cites canonical path

- **GIVEN** disk-image packaging completed successfully
- **WHEN** an agent or packaging entry point reports the deploy artifact
- **THEN** the primary reference is the canonical flash-ready disk image in the build output area
- **AND** any secondary imaging-tool reference points at the same underlying image content

### Requirement: Rebuild invalidates prior flash-ready disk image

The disk-image packaging entry point SHALL remove an existing canonical flash-ready disk image (and its documented imaging-tool alias) at the start of a new packaging invocation before writing a replacement, so a partial or in-flight rebuild cannot coexist with a complete prior image at the canonical location.

#### Scenario: Packaging start clears stale image

- **GIVEN** a canonical flash-ready disk image already exists from a prior packaging run
- **WHEN** disk-image packaging is invoked again
- **THEN** the prior canonical image is absent before the new image bytes are written
- **AND** a successful run restores exactly one canonical flash-ready image

#### Scenario: Interrupted packaging leaves no complete canonical image

- **GIVEN** packaging removed a prior canonical image and then failed before completion
- **WHEN** a human or agent inspects the canonical location
- **THEN** no complete flash-ready disk image is present there
- **AND** packaging error output indicates the run did not finish successfully
