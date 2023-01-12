
# NSE Coffee Roaster V2
## Serial protocol state machine (Arduino)

```mermaid
  stateDiagram-V2
      Start --> BetweenMessages
      BetweenMessages --> StartReceived : Semicolon
      BetweenMessages --> UnknownCommand : Not Semicolon
      UnknownCommand --> BetweenMessages
      StartReceived --> SetReceived : >
      StartReceived --> ReadReceived : ?
      StartReceived --> UnknownCommand : Not > or ?
      SetReceived --> CommandReceived : M or C
      SetReceived --> Digit1 : H or F
      SetReceived --> UnknownCommand : Not M or C or H or F
      Digit1 --> Digit2
      Digit2 --> Digit3
      Digit3 --> CommandReceived
      CommandReceived --> SendResponse : /
      CommandReceived --> UnknownCommand : Not /
      SendResponse --> BetweenMessages
      ReadReceived --> CommandReceived : C or H or T or F
      ReadReceived --> UnknownCommand : Not C or H or T or F    
```