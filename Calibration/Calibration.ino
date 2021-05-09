/*
 Name:		Calibration.ino
 Created:	4/29/2021 3:04:41 PM
 Author:	aivel
*/

-////// Board library
#include <M5Stack.h>


////// GFX Free Fonts https://rop.nl/truetype2gfx/
#include "FreeSans5pt7b.h"
#include "FreeSans6pt7b.h"
#include "FreeSans7pt7b.h"
#include "FreeSans8pt7b.h"
//#include "FreeSans9pt7b.h"
#include "FreeSans10pt7b.h"
#include "FreeSans15pt7b.h"
#include "FreeSans20pt7b.h"


////// Credentials is a user library containing paswords, IDs and credentials
#include "credentials.h"
const char ssid[] = WIFI_SSID;
const char password[] = WIFI_PASSWD;
const char iot_user[] = IoT_USER;
const char iot_device[] = IoT_DEVICE;
const char iot_credential[] = IoT_CREDENTIAL;


////// Comunication libraries
#include <Wire.h>
#include <WiFi.h>
//const char* ssid = "<SSID>";
//const char* password = "<PASSWORD>";
#include <WiFiUdp.h>
WiFiUDP ntpUDP;
//#define _DEBUG_   // Uncomment for debugging connection to Thinger
#define _DISABLE_TLS_
#include <ThingerESP32.h>
ThingerESP32 thing(iot_user, iot_device, iot_credential);




////// SD fat library used in order to use exFAT file system
////// Adapted to Mg stack acording to https://github.com/ArminPP/sdFAT-M5Stack
#include <SdFat.h>
#define SPI_SPEED SD_SCK_MHZ(25)
#define SD_CONFIG SdSpiConfig(TFCARD_CS_PIN, SHARED_SPI, SPI_SPEED) // TFCARD_CS_PIN is defined in M5Stack Config.h (Pin 4)
/// <summary>
/// IMPORTANT
/// in file SdFat\src\SdFatConfig.h at Line 100 set to cero for NON dedicated SPI:
/// #define ENABLE_DEDICATED_SPI 0
/// </summary>
SdExFat sd;
ExFile LogFile;
ExFile root;
char line[250];
String str = "";
int position = 0;


////// Time libraries
#include <TimeLib.h>
#include <NTPClient.h>
NTPClient timeClient(ntpUDP, "north-america.pool.ntp.org", 0, 1000); // For details, see https://github.com/arduino-libraries/NTPClient
// Time zone library
#include <Timezone.h>
//  Central Time Zone (Mexico City)
TimeChangeRule mxCDT = { "CDT", First, Sun, Apr, 2, -300 };
TimeChangeRule mxCST = { "CST", Last, Sun, Oct, 2, -360 };
Timezone mxCT(mxCDT, mxCST);



////// CO2 Sensor Input
const int CO2In = G35;

///// This is for the calibration process
import processing.serial.*;

byte [] buf = new byte[9];
Serial s = new Serial(this, "COM9", 9600);


//////////////////////////////////////////
////// User Constants and Variables //////
//////////////////////////////////////////

////// Station IDs & Constants
const int StaNum = 4;
String StaType = F("Gas-Environmental Alarm");
String StaName = F("Photosynthesis Lab Alarm");
String Firmware = F("v1.0.0");
//const float VRef = 3.3;
const float CO2cal = 3.125;   // Calibrated coeficient to transform voltage to ppm
const bool debug = false;


////// Log File & Headers
const char* FileName[] = { "2020.txt", "2021.txt", "2022.txt", "2023.txt", "2024.txt", "2025.txt", "2026.txt", "2027.txt", "2028.txt", "2029.txt",
            "2030.txt", "2031.txt", "2032.txt", "2033.txt", "2034.txt", "2035.txt", "2036.txt", "2037.txt", "2038.txt", "2039.txt",
            "2040.txt", "2041.txt", "2042.txt", "2043.txt", "2044.txt", "2045.txt", "2046.txt", "2047.txt", "2048.txt", "2049.txt",
            "2050.txt" };
const String Headers = F("UNIX_t\tyear\tmonth\tday\thour\tminute\tsecond\t\
O2%\tCO2ppm\t\
AirTemp\tAirRH\t\
SentIoT");
const int HeaderN = 11;	// Number of items in header (columns); it is cero indexed
String LogString = "";


////// Time variables
unsigned long time_ms = 0;
time_t unix_t;	    // RT UNIX time stamp
time_t SD_unix_t;    // Recorded UNIX time stamp
int s = -1;		    // Seconds
int m = -1;		    // Minutes
int h = -1;		    // Hours
int dy = -1;	    // Day
int mo = -1;	    // Month
int yr = -1;	    // Year
int yrIoT = -1;     // Year of IoT data in payload


////// State machine Shift Registers
int LastSec = -1;           // Last second that gas sensor values where measured
int LastSum = -1;			// Last minute that variables were added to the sum for later averaging
int SumNum = 0;				// Number of times a variable value has beed added to the sum for later averaging
int LastLog = -1;			// Last minute that variables were loged to the SD card
bool PayloadRdy = false;	// Payload ready to send to LoRa

bool O2low = false;
bool O2high = false;
bool CO2high = false;
unsigned long BeepStr = 0;
const int Note = 2000;      // Alarm beep note frequency in Hz
const int BeepLgth = 250;   // Alarm beep length in ms
time_t t_email = 0;             // Last time an Alarm email was sent (in UNIX time format) 
const int email_frq = 10;       // Alert e-mail frequency in minutes

time_t t_DataBucket = 0;             // Last time Data was sent to bucket (in UNIX time format) 
const int DataBucket_frq = 120;       // Data bucket update frequency in seconds


////// Measured instantaneous variables
float O2Value = -1;		// Oxygene value read each second
float CO2raw = -1;    // Carbon dioxide value read each second
float CO2Volt = -1;    // Carbon dioxide value read each second
float CO2ppm = -1;


////// Variables to store sum for eventual averaging
float O2ValueSum = 0;
float CO2ppmSum = 0;

////// Values to be logged. They will be the average over the last 5 minutes
float CO2ppmAvg = 0;


//////////////////////////////////////////////////////////////////////////
// the setup function runs once when you press reset or power the board //
//////////////////////////////////////////////////////////////////////////
void setup() {
    ////// Initialize and setup M5Stack
    M5.begin();
    M5.Power.begin();
    M5.Lcd.println(F("M5 started"));
    Serial.println(F("M5 started"));
    WiFi.begin(ssid, password);
    WiFi.setAutoReconnect(true);
    M5.Lcd.print(F("Connecting to internet"));
    while (WiFi.status() != WL_CONNECTED) {
        M5.Lcd.println(WiFi.status());
        delay(1000);
        M5.Lcd.print(".");
    }
    M5.Lcd.println();
    M5.Lcd.println(F("Setting Speaker"));
    M5.Speaker.setBeep(900, 100);
    M5.Speaker.setVolume(255);
    M5.Speaker.update();
    M5.Lcd.println();


    ////// Configure ADC for reading CO2 sensor
    analogSetClockDiv(128);     // Default is 1, higher value should result in lower sampling speed (better performance)
    analogReadResolution(12);   // ADC read resolution
    analogSetWidth(12);         // ADC sampling resolution
    analogSetAttenuation(ADC_11db);  // ADC attenuation, with 11 dB the range is 150 to 2450 mV
    // Adjust CO2 sensor reading if resolution or attenuation is changed


   
//////////////////////////////////////////////////////////////////////////
// The loop function runs over and over again until power down or reset //
//////////////////////////////////////////////////////////////////////////
void loop() {
    if (debug) {
        M5.Lcd.print(F("Loop start"));
    }
    ////// State 0. Keep the Iot engine runing
    thing.handle();


    ////// State 1. Get current time
    if (true) {
        if (WiFi.isConnected()) { // get UTC unix timestamp from internet time via NTC
            timeClient.update();    // It does not force-update NTC time (see NTPClient declaration for actual udpate interval)
            unix_t = timeClient.getEpochTime();
            unix_t = mxCT.toLocal(unix_t); // Conver to local time
            setTime(unix_t);   // set system time to given unix timestamp
            time_ms = millis();
            if (debug) { M5.Lcd.println(time_ms); }
        }
        else { // If no internet connection, estimate unixtime from last update and enlapsed miliseconds
            if (debug) { M5.Lcd.println(F("No internet")); }
            if (millis() - time_ms > 1000) {
                unix_t = unix_t + round(((millis() - time_ms) / 1000));
                setTime(unix_t);   // set system time to given unix timestamp
                time_ms = millis();
            }
        }
        s = second(unix_t);
        m = minute(unix_t);
        h = hour(unix_t);
        dy = day(unix_t);
        mo = month(unix_t);
        yr = year(unix_t);
        if (debug) { M5.Lcd.println((String)"Time: " + h + ":" + m + ":" + s); }
    }


    ////// State 2. Test if it is time to read gas sensor values (each second)
    if (s != LastSec) { // logical test still need to be added      
        CO2raw = analogRead(CO2In);
        CO2Volt = (CO2raw * 2450) / 4096;   // With attenuation set to 11dB, 12 bit resolution maps to 2450 mV (see ADC setup)
        CO2ppm = (CO2Volt - 400) * CO2cal;
        if (debug) {
            M5.Lcd.println();
            M5.Lcd.println((String)"CO2: " + CO2ppm + " ppm");
            M5.Lcd.println();
        }
        LastSec = s;
    }


    ////// State 3. Test gas limits
    // Always test
    if (true) {
        // High carbon dioxide
        if (CO2ppm > 2000) { CO2high = true; }
        else { CO2high = false; }
    }


    ////// State 4 trigger alarm
    if (O2low || O2high || CO2high) {
        for (int i = 0; i <= 20; i++) {
            BeepStr = millis();
            int j = 0;
            while (millis() - BeepStr < BeepLgth) {
                if (j == 0) {
                    M5.Speaker.tone(Note, BeepLgth);
                    M5.update();
                }
                j += 1;
                thing.handle();     // Keep IoT engine running
            }
            while (millis() - BeepStr < (BeepLgth * 2)) {
                M5.Speaker.mute();
                thing.handle();     // Keep IoT engine running
            }
        }
        // Alert email (via iot thinger endpoint)
        if (O2low || O2high) {
            if ((unix_t - t_email) > (email_frq * 60)) {  // prevent sending too many email with the same message
                pson EmailData;
                EmailData["oxygen"] = O2Value;
                EmailData["year"] = yr;
                EmailData["month"] = mo;
                EmailData["day"] = dy;
                EmailData["hour"] = h;
                EmailData["minute"] = m;
                EmailData["second"] = s;
                if (O2low) {
                    thing.call_endpoint("Low_oxygen_email", EmailData);
                }
                if (O2high) {
                    thing.call_endpoint("High_oxygen_email", EmailData);
                }
                t_email = unix_t;
            }
        }
    }


    ////// State 5. Test if it is time to read Temp and RH values
    ////// AND record sensor values for 5-minute averages (each minute)
    ////// AND update screen
    if (m != LastSum) {
        Temp = sht3x.getTemperatureC();
        RH = sht3x.getHumidityRH();
        if (debug) {
            M5.Lcd.println();
            M5.Lcd.println((String)"Temp: " + Temp + " °C");
            M5.Lcd.println((String)"RH: " + RH + " %");
            M5.Lcd.println();
        }
        // Add new values to sum
        O2ValueSum += O2Value;
        CO2ppmSum += CO2ppm;
        TempSum += Temp;
        RHSum += RH;

        // Update Shift registers
        LastSum = m;
        SumNum += 1;
    }


    ////// State 6. Test if it is time to compute  averages and record in SD card (each 5 minutes)
    if (((m % 5) == 0) && (m != LastLog)) {
        // Calculate averages
        O2ValueAvg = O2ValueSum / SumNum;
        CO2ppmAvg = CO2ppmSum / SumNum;
        TempAvg = TempSum / SumNum;
        RHAvg = RHSum / SumNum;


        // Open Year LogFile (create if not available)
        if (!sd.exists(FileName[yr - 2020])) {
            LogFile.open((FileName[yr - 2020]), O_RDWR | O_CREAT); // Create file

            // Add Metadata
            LogFile.println("Start position of last line send to IoT:\t1");
            // Tabs added to prevent line ending with 0. Line ending with 0 indicates that line needs to be sent to IoT.
            LogFile.println(F("\t\t\t"));
            LogFile.println(F("Metadata:"));
            LogFile.println((String)"Station Number\t" + StaNum + "\t\t\t");
            LogFile.println((String)"Station Name\t" + StaName + "\t\t\t");
            LogFile.println((String)"Station Type\t" + StaType + "\t\t\t");
            LogFile.println((String)"Firmware\t" + Firmware + "\t\t\t");
            LogFile.println((String)"CO2 sensor calibration\t" + CO2cal + "\t\t\t");
            LogFile.println(F("\t\t\t"));
            LogFile.println(F("\t\t\t"));
            LogFile.println(F("\t\t\t"));
            LogFile.println(F("\t\t\t"));
            LogFile.println(F("\t\t\t"));
            LogFile.println(F("\t\t\t"));
            LogFile.println(F("\t\t\t"));

            LogFile.println(Headers); // Add Headers
        }
        else {
            LogFile.open(FileName[yr - 2020], O_RDWR); // Open file
            LogFile.seekEnd(); // Set position to end of file
        }


        // Log to SD card
        LogString = (String)unix_t + "\t" + yr + "\t" + mo + "\t" + dy + "\t" + h + "\t" + m + "\t" + s + "\t" +
            String(O2ValueAvg, 4) + "\t" + String(CO2ppmAvg, 4) + "\t" +
            String(TempAvg, 4) + "\t" + String(RHAvg, 4) + "\t" +
            "0";
        LogFile.println(LogString); // Prints Log string to SD card file "LogFile.txt"
        LogFile.close(); // Close SD card file to save changes


        // Reset Shift Registers
        LastLog = m;
        O2ValueSum = 0;
        CO2ppmSum = 0;
        TempSum = 0;
        RHSum = 0;

        SumNum = 0;
    }


    ////// State 7. Update Screen
    if (true) {
        // Top left, Carbon dioxide Calibration
        M5.Lcd.fillRect(0, 320, 320, 0, M5.Lcd.color565(128, 128, 128));
        M5.Lcd.setTextColor(M5.Lcd.color565(255, 255, 255));
        M5.Lcd.setFreeFont(&FreeSans7pt7b);
        M5.Lcd.setTextDatum(TL_DATUM);
        M5.Lcd.drawString(F("Air CO2 (ppm):"), 10 +160, 10 + 100);
        M5.Lcd.setFreeFont(&FreeSans20pt7b);
        M5.Lcd.setTextDatum(MC_DATUM);
        M5.Lcd.drawString(String(CO2ppm), 80 + 160, 50 + 100);
    
       
    }
    ///State 8. Press boton
    if buttonA.wasPressed():
lcd.print('Button A was Pressed\n');
    }
 
  ///State 9. Calibration begins
 if (true) {
  // calibrate zero point
s.write(0xff);  // header
s.write(0x01);  // sensor no
s.write(0x87);  // command
s.write(0x00);
s.write(0x00);
s.write(0x00);
s.write(0x00);
s.write(0x00);
s.write(0x78);  // check value
  
  // read return value (9byte)
print("read value : ");
for (int i = 0; i < 9; ++i) {
  buf[i] = (byte)s.read();
  print(hex(buf[i]));
  print(", ");
}
println();

// read gas soncentration
s.write(0xff);  // header
s.write(0x01);  // sensor no
s.write(0x86);  // command
s.write(0x00);
s.write(0x00);
s.write(0x00);
s.write(0x00);
s.write(0x00);
s.write(0x79);  // check value
delay(500);

// read return value (9byte)
print("read value : ");
for (int i = 0; i < 9; ++i) {
  buf[i] = (byte)s.read();
  print(hex(buf[i]));
  print(", ");
}
println();

int val = Byte.toUnsignedInt(buf[2]) * 256 + Byte.toUnsignedInt(buf[3]);
println("co2=" + val);
 }

}
