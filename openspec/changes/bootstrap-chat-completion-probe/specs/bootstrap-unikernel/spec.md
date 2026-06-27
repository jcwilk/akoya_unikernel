## MODIFIED Requirements

### Requirement: Deterministic bootstrap scope

The bootstrap image SHALL perform no application logic beyond initialization, console setup, emitting diagnostic output (including network diagnostics and chat-completion diagnostics), and an orderly halt or idle loop.

#### Scenario: No hidden side effects

- **GIVEN** the bootstrap image is invoked for pipeline verification
- **WHEN** boot completes
- **THEN** no persistent services beyond boot-time diagnostics remain active
- **AND** no persistent mutations are made to attached storage
- **AND** behavior is suitable for repeated smoke testing

#### Scenario: Network scope bounded to diagnostics

- **GIVEN** a correctly built bootstrap image
- **WHEN** boot completes
- **THEN** network activity is limited to address acquisition, address reporting, a single connectivity probe, and a single chat-completion HTTP exchange
- **AND** no listening servers or background retry loops continue after diagnostics finish
