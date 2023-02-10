
// NSE Coffee Roaster code for the Arduino Uno
// 2011 NSE Labs
// Version 2 2022 NSE Labs
// This code is in the public domain - feel free to use as you see fit
//
// MAX 6675 library is the Adafruit version available in Manage Libraries in the Arduino IDE
//
// PString library from Mikal Hart http://arduiniana.org/libraries/PString/
// 

#include <max6675.h>
#include <PString.h>
#include <Wire.h>
#include <avr/io.h>
#include <avr/interrupt.h>

// Digital I/O pins we are using
#define ZERO_CROSSING_DETECT_INPUT 2
#define DRUM_MOTOR 3
#define CASE_FAN 4
#define MAX6675_DO_INPUT 5
#define MAX6675_CS_OUTPUT 6
#define MAX6675_CLK_OUTPUT 7
#define FAN_TRIAC_OUTPUT 9
#define HEATER_TRIAC_OUTPUT 10
#define MANUAL_LED_OUTPUT 11
#define COMPUTER_LED_OUTPUT 12
#define ONBOARD_LED_OUTPUT 13

// Analog I/O pins we are using
#define HEATER_CONTROL_POT 0
#define FAN_CONTROL_POT 1
#define I2C_SDA A4
#define I2C_CLK A5

// Fire up the MAX 6675 thermocouple interface library
MAX6675 thermocouple(MAX6675_CLK_OUTPUT, MAX6675_CS_OUTPUT, MAX6675_DO_INPUT);

//********************************************************************
// These are the main parameters we are reading/setting on the roaster
//********************************************************************
double temperature = 0; // Roaster temperature degrees C
int heater = 0;  // Roaster heater output 0-100%
int fan = 0;     // Roaster fan output 0-100%
//********************************************************************


// The control mode can be manual or computer. In manual
// mode the front panel knobs on the roaster are used to 
// control the power to the heater and the fan speed. In 
// computer mode the front panel knobs are disabled and 
// the heater and fan are controlled by serial commands
// from the PC through the USB port
enum mode {
  manual,
  computer
} controlMode = manual;

// Serial port receive state
// See state diagram on the NSE web site
enum state {
   betweenMessages,
   startReceived,
   setReceived,
   digit1,
   digit2,
   digit3,
   readReceived,
   commandReceived
} rcvState = betweenMessages;

// These are the commands we will accept from the PC
enum command {
  unknown,
  setControlMode,
  readControlMode,
  readTemperature,
  setHeater,
  readHeater,
  setFan,
  readFan
} rcvCommand = unknown;

// temporary storage during numeric commands
char firstDigit = 0;
char secondDigit = 0;
char thirdDigit = 0;
  
// Serial response for unknown command
const char *unknownCommand = ":U/";

#define TEMP_SAMPLE_PERIOD  1000 // how long between temperature samples in milliseconds
long lastTempSampleTime = 0;  // last time the temperature was sampled

#define DISPLAY_UPDATE_PERIOD 100 // how long between display updates in milliseconds
long lastDisplayUpdateTime = 0; // last time display was updated

#define TRIGGER_PULSE_COUNT 80  // The triac trigger must stay high for this number of timer1 counts (40 uS)

// Map power output (0-100%)to timer counts for triac firing
unsigned int heaterMap[101] = 
{ 
  0xffff,14734,14219,13851,13555,13301,13077,12875,12688,12515,
  12352,12198,12053,11913,11779,11650,11525,11404,11287,11173, 
  11062,10953,10847,10743,10640,10540,10441,10344,10248,10154,
  10060,9968,9877,9786,9697,9608,9520,9433,9346,9260,
  9174,9089,9004,8920,8835,8751,8668,8584,8501,8417,
  8334,8251,8167,8084,8000,7916,7832,7748,7664,7579,
  7494,7408,7322,7235,7148,7060,6971,6881,6791,6700,
  6608,6514,6420,6324,6226,6128,6027,5925,5821,5714,
  5606,5495,5380,5263,5143,5018,4889,4755,4616,4469,
  4313,4153,3980,3794,3591,3367,3113,2817,2450,1933,
  50
};

unsigned int fanMap[101] = 
{ 
  0xffff,9600,9599,9596,9593,9588,9583,9577,9569,9561,
  9552,9542,9531,9519,9506,9492,9477,9461,9444,9427,
  9408,9388,9368,9346,9324,9301,9277,9252,9226,9199,
  9171,9142,9113,9082,9051,9019,8986,8952,8917,8881,
  8845,8807,8769,8730,8690,8649,8607,8564,8521,8476,
  8431,8384,8337,8289,8240,8190,8139,8087,8034,7980,
  7926,7870,7813,7755,7696,7636,7575,7512,7449,7384,
  7318,7251,7182,7112,7041,6968,6894,6818,6741,6661,
  6580,6497,6412,6325,6236,6144,6050,5953,5854,5751,
  5645,5535,5421,5303,5180,5052,4917,4776,4627,4469,
  4300
};

int ledState = LOW;  // used for toggling the on-board LED
int currentFanOutput = 0; // used to ramp the fan output to its set level
#define FAN_UPDATE_INTERVAL 34 // how many milliseconds between fan output updates of 1% change
long lastFanUpdateTime = 0; // last time the fan output was updated

   
void setup() 
{
  // start serial port at 115200 bps:
  Serial.begin(115200);
  
  // start the I2C display interface and put some text on the screen
  initializeDisplay();
   
  // Initialize drum motor and case fan outputs
  pinMode(DRUM_MOTOR, OUTPUT); 
  digitalWrite(DRUM_MOTOR, LOW); /* drum motor off */
  pinMode(CASE_FAN, OUTPUT); 
  digitalWrite(CASE_FAN, HIGH); /* case fan on */
  
  // Set up the LEDs
  pinMode(ONBOARD_LED_OUTPUT, OUTPUT);
  pinMode(MANUAL_LED_OUTPUT, OUTPUT);
  pinMode(COMPUTER_LED_OUTPUT, OUTPUT);
  setFrontPanelLEDs();

  // Set up the port pin for zero crossing detect
  pinMode(ZERO_CROSSING_DETECT_INPUT, INPUT);
  digitalWrite(ZERO_CROSSING_DETECT_INPUT, HIGH);  // Turn on the pullup resistor  
  
  // Set up the analog input ports (used for front panel knobs)
  pinMode(A0, INPUT);
  pinMode(A1, INPUT);
  digitalWrite(A0, LOW);  // Turn off the pullup resistors
  digitalWrite(A1, LOW);
  
  // Wait for MAX 6675 to stabilize
  delay(500);
  
  // Set up Timer 1
  TIMSK1 = B00000000;  //No interrupts enabled
  TCCR1A = B10100000;  //Both comparator output pins set to zero on comparator match, timer set to Normal mode
  TCCR1B = B00000010;  //No capture filtering, timer set to Normal mode, use clk/8 prescaler 
  OCR1A = 0xffff;  // Set the compare registers to their maximum values
  OCR1B = 0xffff;  // Set the compare registers to their maximum values
  TCNT1 = 0;  //Set the timer count to zero
  TCCR1C = B10000000;  //Force a compare match on comparator A to set the outputs to zero
  TCCR1C = B01000000;  //Force a compare match on comparator B to set the outputs to zero
  TIMSK1 = B00000110;  //Compare A and Compare B interrupts enabled
  pinMode(HEATER_TRIAC_OUTPUT, OUTPUT);  // Set the heater triac trigger pin to output
  pinMode(FAN_TRIAC_OUTPUT, OUTPUT);  // Set the fan triac trigger pin to output
  TCCR1A = B01010000;  //Both comparator output pins toggle on comparator match, timer set to Normal mode

  // Set up the zero crossing interrupt
  attachInterrupt(0, zeroCrossingInterrupt, FALLING);
}

// Interrupt service routine for zero crossing detection
void zeroCrossingInterrupt()
{
  TCNT1 = 0;  //Set the timer count to zero
   
  // Set the counter compare registers based on the heater and fan settings
  OCR1B = heaterMap[heater];
  OCR1A = fanMap[currentFanOutput];
}

// Interrupt service routine for comparator A match
ISR(TIMER1_COMPA_vect)
{
  unsigned int start;
  
  start = TCNT1;  // Read the current timer value
  while((TCNT1 - start) < TRIGGER_PULSE_COUNT); // Wait for the required trigger time for the triac
  
  TCCR1C = B10000000;  // Force a compare match on channel A to toggle the output pin to zero
}

// Interrupt service routine for comparator B match
ISR(TIMER1_COMPB_vect)
{
  unsigned int start;
  
  start = TCNT1;  // Read the current timer value
  while((TCNT1 - start) < TRIGGER_PULSE_COUNT); // Wait for the required trigger time for the triac
  
  TCCR1C = B01000000;  // Force a compare match on channel B to toggle the output pin to zero
}

void loop() 
{ 
  long currentMillis;
  int analogValue;
  

  if(controlMode == manual)
  {
    // Read the front panel control knobs
    analogValue = analogRead(HEATER_CONTROL_POT);
    heater = map(analogValue, 1023,0, 0, 100);

    analogValue = analogRead(FAN_CONTROL_POT);
    fan = map(analogValue, 1023, 0, 0, 100);
  }    
    
  currentMillis = millis();
  
  // read the temperature every TEMP_SAMPLE_PERIOD
  if((currentMillis - lastTempSampleTime) > TEMP_SAMPLE_PERIOD) 
  {
    // save the last time we took a sample and read the temperature
    lastTempSampleTime = currentMillis;   
    temperature = thermocouple.readCelsius();
 
    // toggle the LED to show we are sampling
    if(ledState == LOW)
    {
      ledState = HIGH;      
    }
    else
    {
      ledState = LOW;
    }
    digitalWrite(ONBOARD_LED_OUTPUT, ledState);
    
  }
  
  // deal with any commands from the PC
  if (Serial.available() >  0) handleSerial();  
  
    // update the fan output every FAN_UPDATE_INTERVAL milliseconds
  if((currentMillis - lastFanUpdateTime) > FAN_UPDATE_INTERVAL) 
  {
    lastFanUpdateTime = currentMillis;
    
    // ramp the fan output to its set value by 1% per interval
    if(currentFanOutput < fan)
    {
      currentFanOutput += 1;
    }
    else if (currentFanOutput > fan)
    {
      currentFanOutput -= 1;
    }
  }
  
  if(currentMillis > 3000)
  {
    // Leave the initial message up for 3 seconds, after that put data on the display

    // update the display every DISPLAY_UPDATE_PERIOD
    if((currentMillis - lastDisplayUpdateTime) > DISPLAY_UPDATE_PERIOD)
    {
      lastDisplayUpdateTime = currentMillis;
      updateDisplay();
    }
  }
   
}

void handleSerial()
{
      // This routine is called each time a serial character is available
      // It runs the serial receive state machine documented in the project documentation

      char inByte; // incoming serial byte
      int number;  // decoded number for numeric commands       
      
      inByte = Serial.read();
      switch(rcvState) 
      {
        case betweenMessages:
          if(inByte == ':')
          {
            rcvState = startReceived;
          } 
          else
          {
            clearState();
          }
          break;
        case startReceived:
          if(inByte == '>')
          {
            rcvState = setReceived;
          }
          else if(inByte == '?')
          {
            rcvState = readReceived;
          }
          else
          {
            Serial.print(unknownCommand);
            clearState();
          } 
          break;
        case readReceived:
          switch(inByte)
          {
            case 'C':  // read command mode
              rcvCommand = readControlMode;
              rcvState = commandReceived;
              break;
            case 'T':  // read temperature
              rcvCommand = readTemperature;
              rcvState = commandReceived;
              break;
            case 'H':   // read heater
              rcvCommand = readHeater;
              rcvState = commandReceived;
              break;
            case 'F':  // read fan 
              rcvCommand = readFan;
              rcvState = commandReceived;
              break;
            default:
              Serial.print(unknownCommand);
              clearState();
              break;
          }
          break;
        case setReceived:
          switch(inByte)
          {
            case 'C': // set computer control mode
              rcvCommand = setControlMode;
              controlMode = computer;
              setFrontPanelLEDs();
              rcvState = commandReceived;
              break;
            case 'M': // set manual mode
              rcvCommand = setControlMode;
              controlMode = manual;
              setFrontPanelLEDs();
              rcvState = commandReceived;
              break;
            case 'H':
              if(controlMode == manual)
              {
                // Ignore set heater command in manual mode
                clearState();
              }
              else
              {
                rcvCommand = setHeater;
                rcvState = digit1;
              }
              break;
            case 'F':
              if(controlMode == manual)
              {
                // Ignore set fan command in manual mode
                clearState();
              }
              else
              {
                rcvCommand = setFan;
                rcvState = digit1;
              }
              break;
            default:
              Serial.print(unknownCommand);
              clearState();
              break;
          }
          break;
        case digit1:
          if(isanum(inByte))
          {
            firstDigit = inByte;
            rcvState = digit2;
          }
          else
          {
              Serial.print(unknownCommand);
              clearState();
          }
          break;
        case digit2:
          if(isanum(inByte))
          {
            secondDigit = inByte;
            rcvState = digit3;
          }
          else
          {
              Serial.print(unknownCommand);
              clearState();
          }
          break;
        case digit3:
          if(isanum(inByte))
          {
            thirdDigit = inByte;
            rcvState = commandReceived;
            
            firstDigit = firstDigit - '0';  // convert from ASCII to num
            secondDigit = secondDigit - '0';
            thirdDigit = thirdDigit - '0';
            
            number = ((int)firstDigit*100)+((int)secondDigit*10)+(int)thirdDigit;
            if(number > 100) number = 100;
            if(number < 0) number = 0;
            
            switch(rcvCommand)
            {
              case setHeater:
                heater = number;
                break;
              case setFan:
                fan = number;
                break;
            }
          }
          else
          {
              Serial.print(unknownCommand);
              clearState();
          }
          break;
        case commandReceived:
          if(inByte == '/')
          {
            sendResponse();
          }
          else
          {
            Serial.print(unknownCommand);
          }
          clearState();
          break;
        default:
          Serial.print(unknownCommand);
          clearState();
          break;                
      }           
}


char isanum(char c)
{
  // returns 1 if the character is a number, zero otherwise
  switch(c)
  {
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
      return (1);
      break;
    default:
      return (0);
      break;
  }
}

void clearState()
{
  rcvState = betweenMessages;
  rcvCommand = unknown;
}

void sendResponse()
{
  Serial.print(":"); // Send start character
  switch(rcvCommand)
  {
    case setControlMode:
    case readControlMode:
      if(controlMode == computer)
      {
        Serial.print("C");
      }
      else
      {
        Serial.print("M");
      }
      break;
    case readTemperature:
      Serial.print("T");
      if(temperature < 100) Serial.print("0"); // take care of leading zeros
      if(temperature < 10) Serial.print("0");
      if(temperature < 0)
      {
        Serial.print(".00");  // This shouldn't happen but just in case...
      }
      else
      {
        Serial.print(temperature,2);
      }
      break;
    case setHeater:
    case readHeater:
      Serial.print("H");
      if(heater < 100) Serial.print("0"); // take care of leading zeros
      if(heater < 10) Serial.print("0");    
      Serial.print((double)heater,0);
      break;  
    case setFan:
    case readFan:
      Serial.print("F");
      if(fan < 100) Serial.print("0"); // take care of leading zeros
      if(fan < 10) Serial.print("0");    
      Serial.print((double)fan,0);
      break;  
    default:
      Serial.print("U");  // Unknown command
      break;
  }
  Serial.print("/");  // Send end character
}

void setFrontPanelLEDs()
{
  switch(controlMode)
  {
    case computer:
      digitalWrite(MANUAL_LED_OUTPUT, LOW);
      digitalWrite(COMPUTER_LED_OUTPUT, HIGH);
      break;
    case manual:
      digitalWrite(MANUAL_LED_OUTPUT, HIGH);
      digitalWrite(COMPUTER_LED_OUTPUT, LOW);
      break;
  }
}

//*****************************************************************
// LCD display code
//*****************************************************************

// The default I2C slave address for the display is 50 hex
// The Wire library uses 7-bit addresses so we need to right-shift
// by one bit to get the correct address
#define DISPLAY_I2C_ADDRESS  0x50>>1

// The command prefix for the display - send this byte
// followed by the command
#define DISPLAY_COMMAND_PREFIX 0xFE

// Command codes for the display
#define DISPLAY_ON 0x41
#define DISPLAY_CURSOR_POS 0x45
#define DISPLAY_CURSOR_HOME 0x46
#define DISPLAY_CLEAR_SCREEN 0x51
#define DISPLAY_SET_BRIGHTNESS 0x53

// Display brightness can be set from 1-8
#define DISPLAY_BRIGHTNESS 8

void initializeDisplay()
{
  // start the I2C display interface
  pinMode(I2C_SDA, OUTPUT);
  pinMode(I2C_CLK, OUTPUT);
  Wire.begin();
  
  // Put something on the display
  Wire.beginTransmission(DISPLAY_I2C_ADDRESS);
  Wire.write(DISPLAY_COMMAND_PREFIX);
  Wire.write(DISPLAY_ON);
  Wire.write(DISPLAY_COMMAND_PREFIX);
  Wire.write(DISPLAY_SET_BRIGHTNESS);
  Wire.write(DISPLAY_BRIGHTNESS);
  Wire.write(DISPLAY_COMMAND_PREFIX);
  Wire.write(DISPLAY_CLEAR_SCREEN);
  Wire.endTransmission();
  Wire.beginTransmission(DISPLAY_I2C_ADDRESS);
  Wire.write("    NSE Labs");
  Wire.write(DISPLAY_COMMAND_PREFIX);
  Wire.write(DISPLAY_CURSOR_POS);
  Wire.write(0x40); // Beginning of second line
  Wire.write(" Tissot Coffee");
  Wire.endTransmission();
}

void updateDisplay()
{
  double tempF; // temperature in degrees
  char firstLine[17]; //16 chars per line plus null terminator
  char secondLine[17];
  PString line1(firstLine, sizeof(firstLine));
  PString line2(secondLine, sizeof(secondLine));
  
  tempF = temperature * 9.0/5.0 + 32;
  
  line1.print("HTR ");
  line1.print(heater);
  line1.print("% ");
  if(heater<100) line1.print(" ");
  if(heater<10) line1.print(" ");
  if(tempF<100.0) line1.print(" ");
  line1.print(tempF,1);
  line1.print("\xDF"); // degree symbol
  line1.print("F");

  
  line2.print("FAN ");
  line2.print(fan);
  line2.print("% ");
  if(fan<100) line2.print(" ");
  if(fan<10) line2.print(" ");
  if(controlMode == computer)
  {
    line2.print("   Auto");
  }
  {
    line2.print(" Manual");
  }
  

  Wire.beginTransmission(DISPLAY_I2C_ADDRESS);
  Wire.write(DISPLAY_COMMAND_PREFIX);
  Wire.write(DISPLAY_CURSOR_HOME);
  Wire.write(firstLine);
  Wire.endTransmission();
 
  Wire.beginTransmission(DISPLAY_I2C_ADDRESS);
  Wire.write(DISPLAY_COMMAND_PREFIX);
  Wire.write(DISPLAY_CURSOR_POS);
  Wire.write(0x40); // Beginning of second line
  Wire.write(secondLine);
  Wire.endTransmission();
  
}
