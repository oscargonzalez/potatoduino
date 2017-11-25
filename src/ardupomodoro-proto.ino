/* 
 *  Based in the Serial 7-Segment Display Example Code from SparkFun Electronics by: Jim Lindblom
 *  Code: Oscar Gonzalez - BricoGeek.com
*/
#include <Wire.h> // Include the Arduino SPI library

#define DISPLAY_ADDRESS 0x71
#define BUTTON_PIN      10
#define BUZZER_PIN      6
#define VBATPIN         A9

char tempString[10];  // Will be used with sprintf to create strings
int minutes = 25;
int seconds = 0;

bool pomodoroStarted  = false;
bool pomodoroPause    = false;
int  pomodoroCount    = 1;

unsigned long startTime=0;

// This custom function works somewhat like a serial.print.
//  You can send it an array of chars (string) and it'll print
//  the first 4 characters in the array.
void s7sSendStringI2C(String toSend)
{
  Wire.beginTransmission(DISPLAY_ADDRESS);
  for (int i=0; i<4; i++)
  {
    Wire.write(toSend[i]);
  }
  Wire.endTransmission();
}

// Send the clear display command (0x76)
//  This will clear the display and reset the cursor
void clearDisplayI2C()
{
  Wire.beginTransmission(DISPLAY_ADDRESS);
  Wire.write(0x76);  // Clear display command
  Wire.endTransmission();
}

// Set the displays brightness. Should receive byte with the value
//  to set the brightness to
//  dimmest------------->brightest
//     0--------127--------255
void setBrightnessI2C(byte value)
{
  Wire.beginTransmission(DISPLAY_ADDRESS);
  Wire.write(0x7A);  // Set brightness command byte
  Wire.write(value);  // brightness data byte
  Wire.endTransmission();
}

// Turn on any, none, or all of the decimals.
//  The six lowest bits in the decimals parameter sets a decimal 
//  (or colon, or apostrophe) on or off. A 1 indicates on, 0 off.
//  [MSB] (X)(X)(Apos)(Colon)(Digit 4)(Digit 3)(Digit2)(Digit1)
void setDecimalsI2C(byte decimals)
{
  Wire.beginTransmission(DISPLAY_ADDRESS);
  Wire.write(0x77);
  Wire.write(decimals);
  Wire.endTransmission();
}


void updateTimer()
{

  if (pomodoroStarted == false) { return; }

  Serial.println("updateTimer");
   
  if (seconds == 0) { 
    if (minutes == 0)
    {
      // END COUNTER!
      if (!pomodoroPause)
      {
        tone(BUZZER_PIN, 2500, 50);
        delay(50);
        tone(BUZZER_PIN, 1500, 50);
        delay(50);
        pomodoroPause = true;
        minutes = 5;
        seconds = 0;

        if (pomodoroCount == 4)
        {
          // FIN de 4 pomodoros!
          pomodoroPause = false;
          pomodoroStarted = false;
          minutes = 25;
          seconds = 0;
          pomodoroCount=1;

          tone(BUZZER_PIN, 500, 200); delay(200);
          tone(BUZZER_PIN, 1500, 200); delay(200);
          tone(BUZZER_PIN, 1800, 100); delay(100);
          tone(BUZZER_PIN, 2500, 300); delay(300);
          tone(BUZZER_PIN, 2500, 500); delay(500);

          updateDisplay();
        }
      }
      else
      {
        // END pomodoro pause
        tone(BUZZER_PIN, 1500, 50);
        delay(50);
        tone(BUZZER_PIN, 2500, 50);        
        delay(50);
        pomodoroPause = false;
        minutes = 25;
        seconds = 0;        
        pomodoroCount++;
      }      
    }
    else
    {
      seconds = 59;
      minutes--;              
    }
  }
  else {
    seconds--;   
  }
}

float checkBat()
{
  float measuredvbat = analogRead(VBATPIN);
  measuredvbat *= 2;    // we divided by 2, so multiply back
  measuredvbat *= 3.3;  // Multiply by 3.3V, our reference voltage
  measuredvbat /= 1024; // convert to voltage
  Serial.print("VBat: " ); Serial.println(measuredvbat);  

  if (measuredvbat <= 3.7)
  {
    s7sSendStringI2C("LO  ");
    tone(BUZZER_PIN, 200, 50);delay(80);
    tone(BUZZER_PIN, 200, 50);delay(80);
    tone(BUZZER_PIN, 200, 50);delay(80);
    delay(500);    
  }
  return measuredvbat;
}

void setup()
{  
  Serial.begin(9600);
  delay(3000);
  Serial.println("Setup");
  Wire.begin();  // Initialize hardware I2C pins
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);  

  float vbat = checkBat();
  
  // Clear the display, and then turn on all segments and decimals
  clearDisplayI2C();  // Clears display, resets cursor
  setBrightnessI2C(255);  // High brightness
  setDecimalsI2C(0b00010000);  // Turn on all decimals, colon, apos

  sprintf(tempString, "%02d%02d", minutes, seconds);
  s7sSendStringI2C(tempString);

  setPomodoroPoint(1);
  
}

void setPomodoroPoint(int num)
{
  if (num == 1) { setDecimalsI2C(0b00011000); }
  if (num == 2) { setDecimalsI2C(0b00011100); }
  if (num == 3) { setDecimalsI2C(0b00011110); }
  if (num == 4) { setDecimalsI2C(0b00011111); }
}

void updateDisplay()
{
  // Magical sprintf creates a string for us to send to the s7s.
  //  The %4d option creates a 4-digit integer.
  sprintf(tempString, "%02d%02d", minutes, seconds);

  setDecimalsI2C(0b00011000);
  
  // This will output the tempString to the S7S
  s7sSendStringI2C(tempString);
  setPomodoroPoint(pomodoroCount);            
}

void loop()
{
  
      if(digitalRead(BUTTON_PIN) == LOW)
      {    
        Serial.println("Click");        
        if (pomodoroStarted == false) { 
          startTime = millis();
          pomodoroStarted = true; 
          tone(BUZZER_PIN,1900, 50);   
          updateTimer();                        
        }                
        else
        {
          // Cancelado          
          tone(BUZZER_PIN,1200, 150);   
          delay(100);
          tone(BUZZER_PIN,500, 200);   
          delay(250);
          pomodoroStarted = false; 
          pomodoroCount=1;
          minutes = 25;
          seconds = 0;          
          updateDisplay();
        }
        delay(250); // crapy debounce
      }
    
      if (pomodoroStarted == true)
      {
        Serial.println("update");
        if ( (millis()-startTime) > 1000)
        {

          checkBat();
          
          startTime = millis();
          updateTimer();      
    
          Serial.print(minutes);
          Serial.print(":");
          Serial.println(seconds);
      
          updateDisplay();
        }
      }
      delay(10);
    }   



