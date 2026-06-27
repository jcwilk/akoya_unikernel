## MODIFIED Requirements

### Requirement: Deterministic bootstrap scope

The bootstrap image SHALL perform no application logic beyond initialization, console setup, emitting diagnostic output (including network diagnostics and interactive chat session activity), and an orderly halt or idle loop after the operator ends the chat session.

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
- **AND** no listening servers or background retry loops continue outside active inference for a submitted line

## ADDED Requirements

### Requirement: Conversation-focused console handoff

After IPv4 configuration succeeds, the bootstrap image SHALL transition the console to a conversation-focused presentation by clearing prior boot output before reachability verification and presenting subsequent chat interaction with minimal, dialogue-oriented formatting.

#### Scenario: Chat session begins on a cleared display

- **GIVEN** IPv4 configuration succeeded
- **WHEN** the interactive chat session is about to accept operator input
- **THEN** prior boot and network trace lines are not visible on the console display
- **AND** the operator sees reachability confirmation followed by the minimal chat input prompt
