/************************************************************************/
/* Sonos Volume control
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

#define PIN_BUTT_UP 4    // Define pin the button is connected to
#define PIN_BUTT_DN 0    // Define pin the button is connected to

#define PIN_LED 2  // Define pin the on-board LED is connected to

#define PIN_RGB_R 15    // RGB Red LED?
#define PIN_RGB_G 12    // RGB Green LED
#define PIN_RGB_B 13    // RGB Blue LED?

#define LDR_PIN A0      // Define the analog pin the LDR is connected to

// Den
//IPAddress g_sonosIP(192, 168, 1, 49);
//const char g_sonosID[] = "949f3e1c6e30";

// Kitchen
IPAddress g_sonosIP(192, 168, 1, 48);
const char g_sonosID[] = "5caafd0b8104";

void setup()
{
  // GPIO
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, HIGH);
  pinMode(PIN_RGB_R, OUTPUT);
  digitalWrite(PIN_RGB_R, LOW);
  pinMode(PIN_RGB_G, OUTPUT);
  digitalWrite(PIN_RGB_G, LOW);
  pinMode(PIN_RGB_B, OUTPUT);
  digitalWrite(PIN_RGB_B, LOW);

  pinMode(PIN_BUTT_DN, INPUT_PULLUP);
  pinMode(PIN_BUTT_UP, INPUT_PULLUP);

  // SERIAL
  Serial.begin(115200);

  // WIFI
  WiFiManager wifiManager;
  wifiManager.autoConnect();
  
  Serial.println("Ready");
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
  g_sonosIsPlaying = (g_sonos.getState(g_sonosIP)==SONOS_STATE_PLAYING);
  g_sonosVolume = g_sonos.getVolume(g_sonosIP);
}

void refreshUI()
{
  digitalWrite( PIN_RGB_R, g_sonosIsPlaying);
}

void task250ms()
{
  static bool latchPlayPauseToggle=false;

  syncToSonos();
  if ( !digitalRead( PIN_BUTT_DN ) && !digitalRead( PIN_BUTT_UP ) )
  {
    if (!latchPlayPauseToggle) {
      latchPlayPauseToggle=true;

      digitalWrite( PIN_RGB_G, HIGH );
      digitalWrite( PIN_RGB_B, HIGH ); 

      if (g_sonosIsPlaying)
      {
        g_sonos.pause( g_sonosIP );
      }
      else
      {
        g_sonos.play( g_sonosIP );
      }
    }
    else
    {
      digitalWrite( PIN_RGB_G, LOW );
      digitalWrite( PIN_RGB_B, LOW );
    }
  }
  else if ( !digitalRead( PIN_BUTT_DN ) )
  {
    latchPlayPauseToggle=false;
    digitalWrite( PIN_RGB_G, HIGH );
    digitalWrite( PIN_RGB_B, LOW );
    g_sonosVolume-=5;
    g_sonos.setVolume( g_sonosIP, g_sonosVolume );
  }
  else if ( !digitalRead( PIN_BUTT_UP ) )
  {
    latchPlayPauseToggle=false;
    digitalWrite( PIN_RGB_G, LOW );
    digitalWrite( PIN_RGB_B, HIGH);
    g_sonosVolume+=5;
    g_sonos.setVolume( g_sonosIP, g_sonosVolume );
  }
  else
  {
    latchPlayPauseToggle=false;
    digitalWrite( PIN_RGB_G, LOW );
    digitalWrite( PIN_RGB_B, LOW );
  }

  refreshUI();
}

///////////////////////////////////////////
// Main Loop
void loop()
{
  static int lastMillis=0;
  int theseMillis = millis() / 250;
  if ( theseMillis != lastMillis )
  {
    lastMillis=theseMillis;
    task250ms();
  }
  yield();
}
