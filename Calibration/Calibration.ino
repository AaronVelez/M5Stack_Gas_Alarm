/*
 Name:		Calibration.ino
 Created:	2/11/2022 09:26 AM
 Authors:	Aarón Vélez
*/




//////////////////////////////////////////////////////////////////
////// Libraries and its associated constants and variables //////
//////////////////////////////////////////////////////////////////

////// Board library
#include <M5StickCPlus.h>




////// Credentials_Gas_Alarm_Photo_Lab.h is a user-created library containing paswords, IDs and credentials
#include "Credentials_Calibration_Gas_Alarm_Photo_Lab.h"
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


////// Comunication libraries
#include <Wire.h>
#include <SPI.h>
#include <WiFi.h>


////// Iot Thinger
#define THINGER_SERVER iot_server   // Delete this line if using a free thinger account 
#define _DEBUG_   // Uncomment for debugging connection to Thinger
#define _DISABLE_TLS_     // Uncoment if needed (port 25202 closed, for example)
#include <ThingerESP32.h>
ThingerESP32 thing(iot_user, iot_device, iot_credential);
/// <summary>
/// IMPORTANT
/// in file \thinger.io\src\ThingerClient.h at Line 355, the function handle_connection() was modified
/// to prevent the thinger handler to agresively try to reconnect to WiFi in case of a lost connection
/// This allows the alarn to keep monitoring gas levels even if there is no network connection
/// </summary>




//////////////////////////////////////////
////// User Constants and Variables //////
//////////////////////////////////////////

////// Station IDs & Constants
const int Station_Number = 4;
String Board = F("M5StickC Plus");
String Processor = F("ESP32-Pico");
String IoT_Hardware = F("ESP32-Pico WiFi");
String IoT_Software = F("Thinger ESP32");
String RTC_Hardware = F("BM8563");
String IoT_Asset_Type = F("Calibration");
String IoT_Group = F("Photosynthesis_Lab");
String IoT_Station_Name = F("Calibrator Gas Alarm Photo Lab");
String Firmware = F("v1.0.0");



//const float VRef = 3.3;
//bool debug = false;



////// CO2 Sensor
// PINs
const int CO2In = G36;
const int CO2Cal = G26;
const int CO2Range = 5000;

// State Variables
bool IsCal = false;             // Shift register to store previous Calibration status
unsigned long CalChg = 0;       // Variable to record calibration status change time in mills
const int CalTime = 15;         // Calibration duration in seconds

// Variables
const int n = 1000; // measure n times the ADC input for averaging

float CO2mVolt = -1;    // Carbon dioxide value read each second
float CO2mVoltSum = 0;
float CO2ppm = -1;







//////////////////////////////////////////////////////////////////////////
// the setup function runs once when you press reset or power the board //
//////////////////////////////////////////////////////////////////////////
void setup() {
    ////// Initialize and setup M5Stack
    M5.begin();
    M5.Lcd.println(F("M5 started"));

#ifdef WiFi_SSID_is_HEX
    String ssidStr = HexString2ASCIIString(ssidHEX);
    ssidStr.toCharArray(ssid, sizeof(ssid) + 1);
#endif

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


    ////// Configure IoT
    M5.Lcd.println(F("Configuring IoT..."));
    if (password == "") { thing.add_wifi(ssid); }
    else { thing.add_wifi(ssid, password); }
    // Define input resources
    // Calibrate command
    thing["Calibrate"] << [](pson& in) {
        if ((bool)in) {
            digitalWrite(CO2Cal, LOW);
            CalChg = millis();
            IsCal = true;
        }
    };


    // Define output resources
    thing["Is calibrating?"] >> [](pson& out) { out = IsCal; };
    thing["RT_Carbon_Dioxide_mVolt"] >> [](pson& out) { out = CO2mVolt; };
    thing["RT_Carbon_Dioxide_ppm"] >> [](pson& out) { out = CO2ppm; };
    
    
    ////// Configure ADC for reading CO2 sensor
    M5.Lcd.println(F("Setting ADC for CO2 sensor..."));

    pinMode(CO2In, INPUT);
    gpio_pulldown_dis(GPIO_NUM_25);
    gpio_pullup_dis(GPIO_NUM_25);

    analogSetClockDiv(255);     // Default is 1
    analogReadResolution(12);   // ADC read resolution
    analogSetWidth(12);         // ADC sampling resolution
    analogSetAttenuation(ADC_11db);  // ADC attenuation, with 11 dB the range is 150 to 2450 mV
    // Adjust CO2 sensor reading if resolution or attenuation is changed



    ////// Configure Calibrate PIN
    pinMode(CO2Cal, OUTPUT);
    digitalWrite(CO2Cal, HIGH);



    // Setup finish message
    M5.Lcd.println(F("Setup done!"));
    M5.Lcd.println(F("\nStarting in "));
    for (int i = 0; i <= 5; i++) {
        M5.Lcd.println(5 - i);
        if (i != 5) { delay(1000); }
        else { delay(250); }
    }
    M5.Lcd.fillScreen(BLACK);
}



//////////////////////////////////////////////////////////////////////////
// The loop function runs over and over again until power down or reset //
//////////////////////////////////////////////////////////////////////////
void loop() {
    ////// State 1. Keep the Iot engine runing
    thing.handle();


    ////// State 2. Read sensor 
    CO2mVoltSum = 0;
    for (int i = 0; i < n; i++) {
        CO2mVoltSum += analogReadMilliVolts(CO2In);
    }
    CO2mVolt = CO2mVoltSum / n;
    CO2ppm = (CO2mVolt - 400) * CO2Range / (2000 - 400);


    ////// State 3. Update Screen
    // Reset screen
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.setCursor(0, 0);
    // CO2 mVolt
    M5.Lcd.setTextSize(2.5);
    M5.Lcd.println();
    M5.Lcd.println(" CO2 mV:");
    M5.Lcd.println();
    M5.Lcd.setTextSize(3.5);
    M5.Lcd.print(" ");
    M5.Lcd.println(CO2mVolt, 1);
    M5.Lcd.println();
    // CO2 ppm
    M5.Lcd.setTextSize(2.5);
    M5.Lcd.println(" CO2 ppm:");
    M5.Lcd.println();
    M5.Lcd.setTextSize(3.5);
    M5.Lcd.print(" ");
    M5.Lcd.println(CO2ppm, 1);
    M5.Lcd.println();
    // Calibration status
    M5.Lcd.setTextSize(2.5);
    M5.Lcd.println(" Is Cal?");
    M5.Lcd.println();
    M5.Lcd.setTextSize(3.5);
    M5.Lcd.print(" ");
    M5.Lcd.println(IsCal);


    ////// State 4. Calibrate
    if ( ( (millis() - CalChg) / 1000) > CalTime ) {
        digitalWrite(CO2Cal, HIGH);
        IsCal = false;
    }




    //
    delay(1000);
  
}






// Function for WiFi SSID with non-ASCII characters
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