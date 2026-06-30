## MODIFIED Requirements

### Requirement: Deterministic bootstrap scope

The bootstrap image SHALL perform no application logic beyond initialization, console setup, emitting diagnostic output (including network diagnostics and interactive chat session activity) under the guest event runtime, and an orderly halt or idle loop after the operator ends the chat session.

#### Scenario: No hidden side effects

- **GIVEN** the bootstrap image is invoked for pipeline verification
- **WHEN** boot completes
- **THEN** no persistent services beyond boot-time diagnostics and the interactive chat session remain active
- **AND** no persistent mutations are made to attached storage
- **AND** behavior remains suitable for repeated smoke testing when the session is ended explicitly

#### Scenario: Network scope bounded to chat session

- **GIVEN** a correctly built bootstrap image
- **WHEN** the interactive chat session is active
- **THEN** network activity is limited to address acquisition, address reporting, a single connectivity probe against the configured chat/inference host, and HTTP chat-completion exchanges for submitted user lines
- **AND** no listening servers or unrelated retry loops continue outside active inference for a submitted line

#### Scenario: Event runtime is primary control flow

- **GIVEN** a correctly built bootstrap image after early console initialization
- **WHEN** network diagnostics or the interactive chat session are running
- **THEN** the guest event runtime is the primary application control flow coordinating that work
- **AND** the image does not run a separate parallel application loop for the same work
