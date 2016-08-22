// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// Use Arduino IDE 1.6.8 or later.

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <sys/time.h>
#include <SPI.h>

// for ESP8266
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <WiFiUdp.h>

#include "azureiothubsample_run.h"
#include "AzureIoTHubClient.h"

<PSW_NEEDED>

static const char ssid[] = "<SSID>";     // the name of your network
static const char psw[] = "<PSW>";
int status = WL_IDLE_STATUS;     // the Wifi radio's status


WiFiClientSecure sslClient; // for ESP8266

AzureIoTHubClient iotHubClient(sslClient);

void initSerial() {
  //Initialize serial and wait for port to open:
   Serial.begin(115200);
   Serial.setDebugOutput(true);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
}

void initWifi() {
  // Set WiFi to station mode and disconnect from an AP if it was previously connected
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  
  byte mac[6];                     // the MAC address of your Wifi shield
  // check for the presence of the shield:
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present");
    // don't continue:
    while (true);
  }

  // attempt to connect to Wifi network:
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    #ifdef PSW_NEEDED
    status = WiFi.begin(ssid, psw);
    #else
    status = WiFi.begin(ssid);
    #endif
    // wait 10 seconds for connection:
    uint8_t timeout = 10;
    while (timeout && (WiFi.status() != WL_CONNECTED)) {
      timeout--;
      delay(1000);
      WiFi.macAddress(mac);
      Serial.print("MAC: ");
      Serial.print(mac[5],HEX);
      Serial.print(":");
      Serial.print(mac[4],HEX);
      Serial.print(":");
      Serial.print(mac[3],HEX);
      Serial.print(":");
      Serial.print(mac[2],HEX);
      Serial.print(":");
      Serial.print(mac[1],HEX);
      Serial.print(":");
      Serial.println(mac[0],HEX);
    }
  }
}

void initTime() {
  // For ESP8266 boards comment out the above portion of the function and un-comment
  // the remainder below.
  
   time_t epochTime;

   configTime(0, 0, "pool.ntp.org", "time.nist.gov");

   while (true) {
       epochTime = time(NULL);

       if (epochTime == 0) {
           Serial.println("Fetching NTP epoch time failed! Waiting 2 seconds to retry.");
           delay(2000);
       } else {
           Serial.print("Fetched NTP epoch time is: ");
           Serial.println(epochTime);
           break;
       }
   }
}

void setup() {
  initSerial();
  Serial.print("Finish initSerial");
  initWifi();
  initTime();
  // you're connected now, so print out the data:
  iotHubClient.begin();
  Serial.print("You're connected to the network");
}

void loop() {
  
    azureiothubsample_run();
}