## MODIFIED Requirements

### Requirement: Orderly completion after network diagnostics

The bootstrap image SHALL finish its boot-time work after the interactive chat session ends and then enter an orderly halt or idle loop without starting additional services.

#### Scenario: No follow-on traffic after session ends

- **GIVEN** network diagnostics have been printed and the operator ended the interactive chat session
- **WHEN** boot completes
- **THEN** the image does not send further application traffic beyond what diagnostics and the chat session required
- **AND** behavior remains suitable for repeated smoke testing when sessions end explicitly
