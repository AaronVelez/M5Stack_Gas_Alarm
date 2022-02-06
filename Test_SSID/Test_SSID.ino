/*
 Name:		Test_SSID.ino
 Created:	2/3/2022 6:24:07 PM
 Author:	aivel
*/


////// Board library
#include <M5Stack.h>


////// Credentials_Gas_Alarm_Photo_Lab.h is a user-created library containing paswords, IDs and credentials
#include "Credentials_Gas_Alarm_Photo_Lab.h"
#ifdef WiFi_SSID_is_HEX
const bool ssidIsHex = true;
const char ssidHEX[] = WIFI_SSID_HEX;
char ssid[64];
#else
const bool ssidIsHex = false;
const char ssid[] = WIFI_SSID;
#endif
const char password[] = WIFI_PASSWD;
const char iot_server[] = IoT_SERVER;
const char iot_user[] = IoT_USER;
const char iot_device[] = IoT_DEVICE;
const char iot_credential[] = IoT_CREDENTIAL;
const char iot_data_bucket[] = IoT_DATA_BUCKET;



////// WiFi
#include <WiFi.h>




// the setup function runs once when you press reset or power the board
void setup() {
    M5.begin();
    M5.Power.begin();
    M5.Lcd.println(F("M5 started"));
    
 
    if (ssidIsHex) {
        String ssidStr = HexString2ASCIIString(ssidHEX); 
        ssidStr.toCharArray(ssid, sizeof(ssid) + 1);
    }
      
    M5.Lcd.print(F("SSID name: "));
    M5.Lcd.println(ssid);

    if (password == "") { WiFi.begin(ssid); }
    else { WiFi.begin(ssid, password); }

    WiFi.setAutoReconnect(false);
    M5.Lcd.print(F("Connecting to internet..."));
    // Test for 10 seconds if there is WiFi connection;
    // if not, continue to loop in order to monitor gas levels
    for (int i = 0; i <= 10; i++) {
        if (WiFi.status() != WL_CONNECTED) {
            M5.Lcd.print(".");
            delay(1000);
        }
        else {
            M5.Lcd.println(F("\nConnected to internet!"));
            break;
        }
        if (i == 10) {
            M5.Lcd.println(F("\nNo internet connection"));
            WiFi.disconnect();  // if no internet, disconnect. This prevents the board to be busy only trying to connect.
        }
    }

}

// the loop function runs over and over again until power down or reset
void loop() {
  
}



void printMacAddress() {
    // the MAC address of your WiFi shield
    byte mac[6];

    // print your MAC address:
    WiFi.macAddress(mac);
    M5.Lcd.print("MAC: ");
    printMacAddress(mac);
}

void listNetworks() {
    // scan for nearby networks:
    M5.Lcd.println("** Scan Networks **");
    int numSsid = WiFi.scanNetworks();
    if (numSsid == -1)
    {
        M5.Lcd.println("Couldn't get a wifi connection");
        while (true);
    }

    // print the list of networks seen:
    M5.Lcd.print("number of available networks:");
    M5.Lcd.println(numSsid);

    // print the network number and name for each network found:
    for (int thisNet = 0; thisNet < numSsid; thisNet++) {
        M5.Lcd.print(thisNet);
        M5.Lcd.print(") ");
        String SSID = WiFi.SSID(thisNet);
        M5.Lcd.println(SSID);
        char SSIDchar[25];
        SSID.toCharArray(SSIDchar, 26);
        M5.Lcd.print(F("Aray: "));
        M5.Lcd.println(SSIDchar);
        byte SSIDbyte[25];
        SSID.getBytes(SSIDbyte, 26);
        M5.Lcd.print(F("Bytes: "));
        for (int i = 0; i < 25; i++) {
            M5.Lcd.print(SSIDbyte[i], HEX);
        }
        M5.Lcd.println();
        M5.Lcd.print("\tSignal: ");
        M5.Lcd.print(WiFi.RSSI(thisNet));
        M5.Lcd.println(" dBm");
        Serial.println(SSID);
        for (int i = 0; i < 25; i++) {
            Serial.print(SSIDbyte[i], HEX);
        }
        Serial.println();
    }
}



void printMacAddress(byte mac[]) {
    for (int i = 5; i >= 0; i--) {
        if (mac[i] < 16) {
            Serial.print("0");
        }
        M5.Lcd.print(mac[i], HEX);
        if (i > 0) {
            Serial.print(":");
        }
    }
    M5.Lcd.println();
}





String HexString2ASCIIString(String hexstring) {
    String temp = "", sub = "", result;
    char buf[3];
    for (int i = 0; i < hexstring.length(); i += 2) {
        sub = hexstring.substring(i, i + 2);
        sub.toCharArray(buf, 3);
        char b = (char)strtol(buf, 0, 16);
        if (b == '\0')
            break;
        temp += b;
    }
    return temp;
}


