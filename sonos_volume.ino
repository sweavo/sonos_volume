/************************************************************************/
/* Sonos Volume control                                                 */
/*                                                                      */
/* This library is free software: you can redistribute it and/or modify */
/* it under the terms of the GNU General Public License as published by */
/* the Free Software Foundation, either version 3 of the License, or    */
/* (at your option) any later version.                                  */
/*                                                                      */
/* This library is distributed in the hope that it will be useful, but  */
/* WITHOUT ANY WARRANTY; without even the implied warranty of           */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU     */
/* General Public License for more details.                             */
/*                                                                      */
/* You should have received a copy of the GNU General Public License    */
/* along with this library. If not, see <http://www.gnu.org/licenses/>. */
/*                                                                      */
/* By Steve Carter sweavo@gmail.com October 2020, derived from          */
/* Sonos UPnP, v1.1 by Thomas Mittet (code@lookout.no) January 2015.    */
/************************************************************************/

////////////////////////////////////////////////////////

// Board-specific
#define PIN_LED 2           // GPIO pin of Onboard LED
#define PIN_RGB_R 15        // RGB Red LED
#define PIN_RGB_G 12        // RGB Green LED
#define PIN_RGB_B 13        // RGB Blue LED

#define PIN_LDR   A0        // Define the analog pin the LDR is connected to

// Application-specific
#define PIN_BUTT_UP 5       // GPIO pin of VOLUME UP button
#define PIN_BUTT_DOWN 0     // GPIO pin of VOLUME DOWN button
#define PIN_BUTT_FUNCTION 4 // GPIO pin of FUNCTION button
#define PIN_JUMPER_DEV 14   // GPIO pin to short to ground to use the development SONOS address/ID
#define PIN_DEVMODE PIN_LED // GPIO pin of Onboard LED
#define DEVMODE_ON LOW
#define DEVMODE_OFF HIGH

#define MS_DEBOUNCE 20
#define MS_LONG_PRESS 2000

////////////////////////////////////////////////////////

#include <ESP8266WiFi.h>
#include <WiFiManager.h>

#include <SonosUPnP.h>
#include <MicroXPath_P.h>

#define SERIAL_DATA_THRESHOLD_MS 500
#define SERIAL_ERROR_TIMEOUT "E: Serial"
#define ETHERNET_ERROR_DHCP "E: DHCP"
#define ETHERNET_ERROR_CONNECT "E: Connect"

void ethConnectError();
WiFiClient client;
SonosUPnP g_sonos = SonosUPnP(client, ethConnectError);


// Den
const IPAddress DEN_IP(192, 168, 1, 49);
const char DEN_ID[] = "949f3e1c6e30";

// Kitchen
const IPAddress KITCHEN_IP(192, 168, 1, 48);
const char KITCHEN_ID[] = "5caafd0b8104";

const IPAddress* sonosIP=&KITCHEN_IP;
const char* sonosID=KITCHEN_ID;

void setup()
{
  // PWM
  analogWriteRange (100);
  
  // GPIO
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, 50);
  pinMode(PIN_RGB_R, OUTPUT);
  digitalWrite(PIN_RGB_R, 50);
  pinMode(PIN_RGB_G, OUTPUT);
  digitalWrite(PIN_RGB_G, 50);
  pinMode(PIN_RGB_B, OUTPUT);
  digitalWrite(PIN_RGB_B, 50);

  pinMode(PIN_BUTT_DOWN, INPUT_PULLUP);
  pinMode(PIN_BUTT_UP, INPUT_PULLUP);
  pinMode(PIN_BUTT_FUNCTION, INPUT_PULLUP);
  pinMode(PIN_JUMPER_DEV, INPUT_PULLUP);
  
  // SERIAL
  Serial.begin(115200);

  // WIFI
  WiFiManager wifiManager;
  wifiManager.autoConnect();

  // SONOS
  if (digitalRead(PIN_JUMPER_DEV))
  {
    digitalWrite(PIN_DEVMODE,DEVMODE_OFF);
  }
  else
  {
    sonosIP = &DEN_IP;
    sonosID = DEN_ID;
    digitalWrite(PIN_DEVMODE,DEVMODE_ON);
  }

  Serial.println("Ready");
}

/* this lerp takes lower, upper, x and returns:
 *
 * 0 if x <= lower
 * 100 if x>= upper
 * linear interpolation of values between.
 */
 int clampedLerp( int lower, int upper, int current )
{
  float gradient = 100.0 / ((float)upper-(float)lower);
  float result = (float)(current - lower ) * gradient;

  if ( result > 100.0 )
  {
    return 100;
  }
  else if ( result < 0.0 )
  {
    return 0.0;
  }
  else
  {
    return result;
  }
  
}

void setRGBForVolume( int vol ) {
  analogWrite( PIN_RGB_G, clampedLerp( 0, 40, vol ) );
  analogWrite( PIN_RGB_R, clampedLerp( 25,65, vol ) );
  analogWrite( PIN_RGB_B, clampedLerp( 50, 100, vol) );

}
void ethConnectError()
{
  Serial.println(ETHERNET_ERROR_CONNECT);
  Serial.println("mir geht es nicht gut");
}

bool isCommand(const char *command, byte b1, byte b2)
{
  return *command == b1 && *++command == b2;
}

bool g_sonosIsPlaying = false;
int g_sonosVolume=0;

void syncToSonos()
{
  g_sonosIsPlaying = (g_sonos.getState((*sonosIP))==SONOS_STATE_PLAYING);
  g_sonosVolume = g_sonos.getVolume((*sonosIP));
}

void refreshUI()
{
  setRGBForVolume( g_sonosIsPlaying ? g_sonosVolume : 0);
}

void task250ms()
{
  syncToSonos();

  if ( !digitalRead( PIN_BUTT_DOWN ) )
  {
    if ( g_sonosVolume > 5 )
    {
      g_sonosVolume-=5;

    }else
    {
      g_sonosVolume=0;

    }
    g_sonos.setVolume( (*sonosIP), g_sonosVolume );
    setRGBForVolume( g_sonosVolume );
  }
  else if ( !digitalRead( PIN_BUTT_UP ) )
  {
    if ( g_sonosVolume < 95 )
    {
      g_sonosVolume+=5;

    }else
    {
      g_sonosVolume=100;
    }
    g_sonos.setVolume( (*sonosIP), g_sonosVolume );
    setRGBForVolume( g_sonosVolume );
  }
  else
  {
    analogWrite( PIN_RGB_B, 0 );
    analogWrite( PIN_RGB_R, 0 );
    analogWrite( PIN_RGB_G, 0 );
    refreshUI();
  }

}

void handleShortPress() 
{
  digitalWrite( PIN_RGB_B, HIGH ); 
  if (g_sonosIsPlaying)
  {
    g_sonos.pause( (*sonosIP) );
  }
  else
  {
    //g_sonos.playLineIn( *sonosIP, KITCHEN_ID);
    g_sonos.play( (*sonosIP) );
  }
}

void handleLongPress()
{
  digitalWrite( PIN_RGB_B, HIGH );    
  g_sonos.playLineIn( *sonosIP, KITCHEN_ID);
}

void handleButtonDown()
{
  digitalWrite( PIN_RGB_G, HIGH );
}

void handleButtonUp()
{
  digitalWrite( PIN_RGB_G, LOW );
  digitalWrite( PIN_RGB_B, LOW );
}

void handleIdle()
{

}
///////////////////////////////////////////
// Main Loop
void loop()
{
  static bool latchPress=false;
  static bool latchLongPress=false;
  
  static unsigned int lastMillis=0;
  static unsigned int functionMillis=0;

  static bool functionButtonReading=false;
  static bool functionButtonPressed=false;
  static unsigned int debounceMillis=0;

   unsigned int theseMillis = millis();

  functionButtonReading = !digitalRead( PIN_BUTT_FUNCTION );
  if ( functionButtonReading != functionButtonPressed )
  {
    functionButtonPressed = functionButtonReading;
    debounceMillis = theseMillis;
  }

  if ( (theseMillis - debounceMillis ) > MS_DEBOUNCE )
  {
    if ( functionButtonReading ) // i.e., pressed.
    {
      if (!latchPress) // ... and this is the edge
      {
        latchPress=true;
        functionMillis=theseMillis;
        handleButtonDown();
      }

      if (!latchLongPress && (( theseMillis - functionMillis ) > MS_LONG_PRESS) ) // Long press
      {
        latchLongPress=true;
        handleLongPress();
      }
    }

    else // function button not pressed
    {
      if ( latchPress ) // ... and this is an edge
      {
        if (!latchLongPress) // It was a short press
        {
          handleShortPress();
        }
        
        latchPress=false;
        latchLongPress=false;

        handleButtonUp();      
      }
      
      handleIdle();  
    }
  }

  if ( theseMillis - lastMillis >= 250 )
  {
    lastMillis=theseMillis;
    task250ms();
  }

}
