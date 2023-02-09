
# NSE Coffee Roaster V2
## Serial protocol state machine (Arduino)

```mermaid
  stateDiagram-V2
      [*] --> BetweenMessages
      BetweenMessages --> StartReceived : Semicolon
      BetweenMessages --> UnknownCommand : Not Semicolon
      UnknownCommand --> BetweenMessages
      StartReceived --> SetReceived : >
      StartReceived --> ReadReceived : ?
      StartReceived --> UnknownCommand : Not > or ?
      SetReceived --> CommandReceived : Valid set command other than H or F
      SetReceived --> Digit1 : H or F
      SetReceived --> UnknownCommand : Not valid set command
      Digit1 --> Digit2
      Digit2 --> Digit3
      Digit3 --> CommandReceived
      CommandReceived --> SendResponse : /
      CommandReceived --> UnknownCommand : Not /
      SendResponse --> BetweenMessages
      ReadReceived --> CommandReceived : Valid read command
      ReadReceived --> UnknownCommand : Not valid read command