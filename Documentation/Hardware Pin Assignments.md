# NSE Coffee Roaster V2
## Hardware Pin Assignments
----
### Arduino Nano
| Digital Pin | Connection | I/O |
| --- | --- | --- |
| 0 | Serial Receive Data | Input |
| 1 | Serial Transmit Data | Output |
| 2 | Zero Crossing Detect (INT0) | Input |
| 3 | Roaster Drum Motor On/Off | Output |
| 4 | Roaster Case Fan On/Off | Output |
| 5 | MAX 6675 D0 | Input |
| 6 | MAX 6675 CS | Output |
| 7 | MAX 6675 CLK | Output |
| 8 | Open | |
| 9 | Heater SSC On/Off | Output |
| 10 | Fan Triac Trigger | Output |
| 11 | Front Panel LED - Manual | Output |
| 12 | Front Panel LED - Computer | Output |
| 13 | On-board LED | Output |

| Analog Pin | Connection | I/O |
| --- | --- | --- |
| A0 | Heater Front Panel Pot | Input |
| A1 | Fan Front Panel Pot | Input |
| A3 | Open | |
| A4 | I2C Display Interface | Output |
| A5 | I2C Display Interface | Output |
----
### Front Panel Interface Ribbon Cable
| Cable Pin | Signal |
| --- | --- |
| 1 | VCC (+5V) |
| 2 | Ground |
| 3 | Display Pin 4 |
| 4 | Display Pin 3 |
| 5 | Heater Pot |
| 6 | Fan Pot |
| 7 | NC |
| 8 | LED - Manual |
| 9 | NC |
| 10 | LED - Computer |
