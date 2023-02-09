| | | | | |
|-|-|-|-|-|
|Coffee Roaster Serial Protocol| | | | |
| | | | | |
|Commands are sent to the Arduino. Responses come from the Arduino.| | | | |
|A response is sent after every command.| | | | |
|A command to set the heater or fan value is ignored and no response is sent if the command mode is Manual| | | | |
| | | | | |
| | |Serial  Characters|Description| |
|Command|Set control mode|:|Start character| |
| | |>|Set command| |
| | |C or M|Computer or Manual| |
| | |/|End character| |
| | | | | |
|Response|Control mode|:|Start character| |
| | |C or M|Computer or Manual| |
| | |/|End character| |
| | | | | |
|Response|Unknown command|:|Start character|For any unrecognized command|
| | |U|Unknown| |
| | |/|End character| |
| | | | | |
|Command|Read control mode|:|Start character| |
| | |?|Read command| |
| | |C|Control mode| |
| | |/|End character| |
| | | | | |
|Response|Control mode|See above| | |
| | | | | |
|Command|Read temperature|:|Start character| |
| | |?|Read command| |
| | |T|Temperature| |
| | |/|End character| |
| | | | | |
|Response|Temperature|:|Start character| |
| | |T|Temperature| |
| | |0-9|Digit 1|Temperature in degrees C|
| | |0-9|Digit 2| |
| | |0-9|Digit 3| |
| | |.|Decimal point| |
| | |0-9|Digit 4| |
| | |0-9|Digit 5| |
| | |/|End character| |
| | | | | |
|Command|Set Heater|:|Start character| |
| | |>|Set command| |
| | |H|Heater| |
| | |0-9|Digit 1|0-100%|
| | |0-9|Digit 2| |
| | |0-9|Digit 3| |
| | |/|End character| |
| | | | | |
|Response|Heater Value|:|Start character| |
| | |H|Heater| |
| | |0-9|Digit 1|0-100%|
| | |0-9|Digit 2| |
| | |0-9|Digit 3| |
| | |/|End character| |
| | | | | |
|Command|Read Heater|:|Start character| |
| | |?|Read command| |
| | |H|Heater| |
| | |/|End character| |
| | | | | |
|Response|Heater Value|See above| | |
| | | | | |
|Command|Set Fan|:|Start character| |
| | |>|Set command| |
| | |F|Fan| |
| | |0-9|Digit 1|0-100%|
| | |0-9|Digit 2| |
| | |0-9|Digit 3| |
| | |/|End character| |
| | | | | |
|Response|Fan Value|:|Start character| |
| | |F|Fan| |
| | |0-9|Digit 1|0-100%|
| | |0-9|Digit 2| |
| | |0-9|Digit 3| |
| | |/|End character| |
| | | | | |
|Command|Read Fan|:|Start character| |
| | |?|Read command| |
| | |F|Fan| |
| | |/|End character| |
| | | | | |
|Response|Fan Value|See above| | |
