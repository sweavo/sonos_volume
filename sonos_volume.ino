/************************************************************************/
/* Sonos UPnP, an UPnP based read/write remote control library, v1.1.   */
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
/* Written by Thomas Mittet (code@lookout.no) January 2015.             */
/************************************************************************/
/*

  Before you can control you Sonos speakers you need to enter their IP
  addresses and serial numbers below. You can find this information in the
  Sonos Controller application on your PC, Mac or phone by opening the
  "About My Sonos System" menu. For each device you will find information
  similar to this:

  PLAY:3: Bedroom
  Serial Number: 00-0A-74-7F-33-A7:8
  Version: 5.2 (build 28183040)
  Hardware Version: 1.8.1.2-2
  IP Address: 192.168.0.203

  Given the above, you configure the IP address and serial number (ID):
  IPAddress g_sonosBedroomIP(192, 168, 0, 203);
  const char g_sonosBedroomID[] = "000A747F33A7";

  When you are done with the configuration you can send commands to the
  speakers by entering commands in the Arduino Serial Monitor. The test
  sketch will mainly control the speaker named Living Room. The other
  speakers are only there to be able to test the group speakers function.

  Here's a list of the commands that are implemented in the test sketch:

  pl = Play
  pa = Pause
  st = Stop
  pr = Previous track
  nx = Next track

  fi = Play test file (file path must be changed for this to work)
  ht = Play http stream (you need access to the music service WIMP)
  ra = Play radio (works if your speakers are connected to the internet)
  li = Play line in (only works if the device has an AUX input)

  gr = Group speakers (Living Room, Bathroom and Bedroom)
  ug = Ungroups speakers

  re = Toggle repeat
  sh = Toggle shuffle
  lo = Toggle loudness
  mu = Toggle mute

  52 = Set volume level 52 (range is 00 - 99)
  b5 = Set bass level -5 (range is 0 to -9)
  B5 = Set bass level +5 (range is 0 to +9)
  t3 = Set treble level -3
  T0 = Set treble level normal

*/

#include <SonosUPnP.h>
#include <MicroXPath_P.h>
//#include <MicroXPath.h>

#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <WiFiManager.h>
#include <ESP8266mDNS.h>
MDNSResponder mdns;
#include <ArduinoOTA.h>

#define SERIAL_DATA_THRESHOLD_MS 500
#define SERIAL_ERROR_TIMEOUT "E: Serial"
#define ETHERNET_ERROR_DHCP "E: DHCP"
#define ETHERNET_ERROR_CONNECT "E: Connect"

void handleSerialRead();
void ethConnectError();
//EthernetClient g_ethClient;
WiFiClient client;
SonosUPnP g_sonos = SonosUPnP(client, ethConnectError);

#define BUTTON_PIN 4    // Define pin the button is connected to
#define ON_BOARD_LED 2  // Define pin the on-board LED is connected to
#define RGB_G_PIN 12    // RGB Green LED
#define LDR_PIN A0      // Define the analog pin the LDR is connected to

// Den
IPAddress g_sonosIP(192, 168, 1, 49);
const char g_sonosID[] = "949f3e1c6e30";

// Kitchen
//IPAddress g_sonosIP(192, 168, 1, 48);
//const char g_sonosID[] = "5caafd0b8104";

char uri[100] = "";

#include "FS.h"

File f;

void handleRoot();
void handleNotFound();

void setup()
{
  // GPIO
  pinMode(ON_BOARD_LED, OUTPUT);       // Initialize the LED_BUILTIN pin as an output
  pinMode(BUTTON_PIN, INPUT_PULLUP);  // Initialize button pin with built-in pullup.
  digitalWrite(ON_BOARD_LED, HIGH);    // Ensure LED is off

  // SERIAL
  Serial.begin(115200);

  // WIFI
  WiFiManager wifiManager;
  wifiManager.autoConnect();
//
//  ArduinoOTA.onStart([]() {
//    Serial.println("Start");
//  });
//  ArduinoOTA.onEnd([]() {
//    Serial.println("\nEnd");
//  });
//  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
//    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
//  });
//  ArduinoOTA.onError([](ota_error_t error) {
//    Serial.printf("Error[%u]: ", error);
//    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
//    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
//    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
//    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
//    else if (error == OTA_END_ERROR) Serial.println("End Failed");
//  });
//  ArduinoOTA.begin();
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());


  /* File System stuff */

  SPIFFS.begin();

  Dir dir = SPIFFS.openDir("/");

  while (dir.next()) {
    Serial.println(dir.fileName());
    f = dir.openFile("r");
    Serial.println(f.size());
    //f.close();
  }
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


void handleSerialRead()
{
  // Read 2 bytes from serial buffer
  if (Serial.available() >= 2)
  {
    byte b1 = Serial.read();
    byte b2 = Serial.read();
    if (isCommand("ok", b1, b2 ))
    {
      Serial.write("OK\r\n");
    }
    // Play
    else if (isCommand("pl", b1, b2))
    {
      g_sonos.play(g_sonosIP);
    }
    // Pause
    else if (isCommand("pa", b1, b2))
    {
      g_sonos.pause(g_sonosIP);
    }
    // Stop
    else if (isCommand("st", b1, b2))
    {
      g_sonos.stop(g_sonosIP);
    }
    // Previous
    else if (isCommand("pr", b1, b2))
    {
      g_sonos.skip(g_sonosIP, SONOS_DIRECTION_BACKWARD);
    }
    // Next
    else if (isCommand("nx", b1, b2))
    {
      g_sonos.skip(g_sonosIP, SONOS_DIRECTION_FORWARD);
    }
    // Mute On
    else if (isCommand("mu", b1, b2))
    {
      g_sonos.setMute(g_sonosIP, true);
    }
    // Mute Off
    else if (isCommand("m_", b1, b2))
    {
      g_sonos.setMute(g_sonosIP, false);
    }
    // Volume/Bass/Treble
    else if (b2 >= '0' && b2 <= '9')
    {
      // Volume 0 to 99
      if (b1 >= '0' && b1 <= '9')
      {
        g_sonos.setVolume(g_sonosIP, ((b1 - '0') * 10) + (b2 - '0'));
      }
    }

    else if (isCommand("ti", b1, b2))
    {
      Serial.println("we want the track uri");
      TrackInfo track = g_sonos.getTrackInfo(g_sonosIP, uri, sizeof(uri));
      Serial.println(uri);
    }

  }
}

///////////////////////////////////////////
// Main Loop
void loop()
{
  if (Serial.available() >= 2) handleSerialRead();
  else yield();
  digitalWrite( ON_BOARD_LED, digitalRead(BUTTON_PIN));
}
