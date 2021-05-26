/*
 Name:		Arduino_code.ino
 Created:	2/18/2021 12:12:37 PM
 Authors:	Daniela Cornejo y Aarón Vélez
*/




//////////////////////////////////////////////////////////////////
////// Libraries and its associated constants and variables //////
//////////////////////////////////////////////////////////////////

////// Board library
#include <M5Stack.h>


////// GFX Free Fonts https://rop.nl/truetype2gfx/
#include "FreeSans5pt7b.h"
#include "FreeSans6pt7b.h"
#include "FreeSans7pt7b.h"
#include "FreeSans8pt7b.h"
//#include "FreeSans9pt7b.h" already defined, it can be used
#include "FreeSans10pt7b.h"
//#include "FreeSans12pt7b.h" already defined, it can be used
#include "FreeSans15pt7b.h"
#include "FreeSans20pt7b.h"


////// Credentials_Gas_Alarm_Photo_Lab.h is a user-created library containing paswords, IDs and credentials
#include "Credentials_Gas_Alarm_Photo_Lab.h"
const char ssid[] = WIFI_SSID;
const char password[] = WIFI_PASSWD;
const char iot_server[] = IoT_SERVER;
const char iot_user[] = IoT_USER;
const char iot_device[] = IoT_DEVICE;
const char iot_credential[] = IoT_CREDENTIAL;
const char iot_data_bucket[] = IoT_DATA_BUCKET;


////// Comunication libraries
#include <Wire.h>
#include <WiFi.h>
#include <WiFiUdp.h>
WiFiUDP ntpUDP;
#define THINGER_SERVER iot_server   // Delete this line if using a free thinger account 
#define _DEBUG_   // Uncomment for debugging connection to Thinger
#define _DISABLE_TLS_     // Uncoment if needed (port 25202 closed, for example)
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
/*
SdExFat sd;
ExFile LogFile;
ExFile root;
*/
SdFat32 sd;
File32 LogFile;
File32 root;

char line[250];
String str = "";
int position = 0;


////// Time libraries
#include <TimeLib.h>
#include <NTPClient.h>
NTPClient timeClient(ntpUDP, "north-america.pool.ntp.org", 0, 60000); // For details, see https://github.com/arduino-libraries/NTPClient
// Time zone library
#include <Timezone.h>
//  Central Time Zone (Mexico City)
TimeChangeRule mxCDT = { "CDT", First, Sun, Apr, 2, -300 };
TimeChangeRule mxCST = { "CST", Last, Sun, Oct, 2, -360 };
Timezone mxCT(mxCDT, mxCST);


////// Library for Oxygen Sensor
#include <DFRobot_OxygenSensor.h>
#define COLLECT_NUMBER    10             // collect number, the collection range is 1-100.
#define Oxygen_IICAddress ADDRESS_3
/*   iic slave Address, The default is ADDRESS_3.
       ADDRESS_0               0x70      // iic device address.
       ADDRESS_1               0x71
       ADDRESS_2               0x72
       ADDRESS_3               0x73
*/
DFRobot_OxygenSensor Oxygen; // Decalre the class for the Oxygen sensor 


////// Library for SHT31 Temperature and Humidity Sensor
#include <DFRobot_SHT3x.h>
DFRobot_SHT3x sht3x(&Wire,/*address=*/0x45,/*RST=*/4);


////// CO2 Sensor Input
const int CO2In = G35;
const int n = 500; // measure n times the ADC input for averaging
float sum = 0; // shift register to hold ADC data



//////////////////////////////////////////
////// User Constants and Variables //////
//////////////////////////////////////////

////// Station IDs & Constants
const int StaNum = 4;
String StaType = F("Gas-Environmental Alarm");
String StaName = F("Gas Alarm Photosynthesis Lab");
String Firmware = F("v1.0.0");
//const float VRef = 3.3;
float CO2cal = 1.25;   // Calibrated coeficient to transform voltage to ppm.
bool debug = false;


////// Log File & Headers
const char* FileName[] = { "2020.txt", "2021.txt", "2022.txt", "2023.txt", "2024.txt", "2025.txt", "2026.txt", "2027.txt", "2028.txt", "2029.txt",
            "2030.txt", "2031.txt", "2032.txt", "2033.txt", "2034.txt", "2035.txt", "2036.txt", "2037.txt", "2038.txt", "2039.txt",
            "2040.txt", "2041.txt", "2042.txt", "2043.txt", "2044.txt", "2045.txt", "2046.txt", "2047.txt", "2048.txt", "2049.txt",
            "2050.txt" };
const String Headers = F("UNIX_t\tyear\tmonth\tday\thour\tminute\tsecond\t\
O2%\tCO2ppm\t\
AirTemp\tAirRH\t\
CO2cal\t\
SentIoT");
const int HeaderN = 12;	// Number of items in header (columns), Also used as a cero-indexed header index
String LogString = "";


////// Time variables
time_t unix_t;	    // RT UNIX time stamp
time_t SD_unix_t;    // Recorded UNIX time stamp
int s = -1;		    // Seconds
int m = -1;		    // Minutes
int h = -1;		    // Hours
int dy = -1;	    // Day
int mo = -1;	    // Month
int yr = -1;	    // Year
// Time for IoT payload
int mIoT = -1;
int hIoT = -1;
int dyIoT = -1;
int moIoT = -1;
int yrIoT = -1;


////// State machine Shift Registers
int LastSec = -1;           // Last second that gas sensor values where measured
int LastLcd = -1;           // Last time the screen was updated
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
const int DataBucket_frq = 150;       // Data bucket update frequency in seconds (must be more than 60)


////// Measured instantaneous variables
float O2Value = -1;		// Oxygene value read each second
float CO2mVolt = -1;    // Carbon dioxide value read each second
float CO2ppm = -1;
float Temp = -1;        // Air temperature read each minute
float RH = -1;          // Air RH value read each minute


////// Variables to store sum for eventual averaging
float O2ValueSum = 0;
float CO2ppmSum = 0;
float TempSum = 0;
float RHSum = 0;


////// Values to be logged. They will be the average over the last 5 minutes
float O2ValueAvg = 0;
float CO2ppmAvg = 0;
float TempAvg = 0;
float RHAvg = 0;


//////////////////////////////////////////////////////////////////////////
// the setup function runs once when you press reset or power the board //
//////////////////////////////////////////////////////////////////////////
void setup() {
    ////// Initialize and setup M5Stack
    M5.begin();
    M5.Power.begin();
    M5.Lcd.println(F("M5 started"));
    WiFi.begin(ssid, password);
    WiFi.setAutoReconnect(true);
    M5.Lcd.print(F("Connecting to internet..."));
    // Test for half a minute if there is WiFi connection;
    // if not, continue to loop in order to monitor gas levels
    for (int i = 0; i <= 30; i++) {
        if (WiFi.status() != WL_CONNECTED) {
            M5.Lcd.print(".");
            delay(1000);
        }
        else {
            M5.Lcd.println(F("Connected to internet!"));
            break;
        }
        if (i == 30) { M5.Lcd.println(F("No internet connection")); }
    }
    M5.Lcd.println(F("Setting Speaker..."));
    //M5.Speaker.setBeep(900, 100);
    //M5.Speaker.setVolume(255);
    //M5.Speaker.update();


    ////// Configure IoT
    M5.Lcd.println(F("Configuring IoT..."));
    thing.add_wifi(ssid, password);
    // Define input resources
    thing["CO2_calibration"] << [](pson& in) {  // Factor is multiplied by 100 in IoT dashboard
        if (in.is_empty()) { in = CO2cal * 100; }
        else { CO2cal = in; CO2cal = CO2cal / 100; }
    };
    thing["Debug"] << [](pson& in) {
        if (in.is_empty()) { in = debug; }
        else { debug = in; }
    };

    // Define output resources
    thing["RT_Oxygene"] >> [](pson& out) { out = O2Value; };
    thing["RT_Carbon_Dioxide"] >> [](pson& out) { out = CO2ppm; };
    thing["RT_Temp"] >> [](pson& out) { out = Temp; };
    thing["RT_RH"] >> [](pson& out) { out = RH; };
    thing["Alarm_O2low"] >> [](pson& out) { out = O2low; };
    thing["Alarm_O2high"] >> [](pson& out) { out = O2high; };
    thing["Alarm_CO2high"] >> [](pson& out) { out = CO2high; };

    thing["Avg_Data"] >> [](pson& out) {
        out["Time_Stamp"] = SD_unix_t;
        out["Oxygene"] = O2ValueAvg;
        out["Carbon_Dioxide"] = CO2ppmAvg;
        out["Temperature"] = TempAvg;
        out["Relative_Humidity"] = RHAvg;
    };


    ////// Initialize SD card
    M5.Lcd.println(F("Setting SD card..."));
    sd.begin(SD_CONFIG);
    // Reserve RAM memory for large and dynamic String object
    // used in SD file write/read
    // (prevents heap RAM framgentation)
    LogString.reserve(HeaderN * 7);
    str.reserve(HeaderN * 7);


    //////// Start NTP client engine and update system time
    M5.Lcd.println(F("Starting NTP client engine..."));
    timeClient.begin();
    M5.Lcd.println(F("Trying to update NTP time..."));
    // Test for half a minute if NTP time can be updated;
    // if not, continue to loop in order to monitor gas levels
    for (int i = 0; i <= 30; i++) {
        if (!timeClient.forceUpdate()) {
            M5.Lcd.print(".");
            delay(1000);
        }
        else {
            M5.Lcd.println(F("NTP time updated!"));
            break;
        }
        if (i == 30) { M5.Lcd.println(F("No NTP update")); }
    }
    unix_t = timeClient.getEpochTime();
    unix_t = mxCT.toLocal(unix_t); // Conver to local time
    setTime(unix_t);   // set system time to given unix timestamp
    

    /////// Start oxygene sensor
    M5.Lcd.println(F("Starting Oxygen sensor..."));
    Oxygen.begin(Oxygen_IICAddress);


    ////// Configure ADC for reading CO2 sensor
    M5.Lcd.println(F("Setting ADC for CO2 sensor..."));
    analogSetClockDiv(255);     // Default is 1
    analogReadResolution(12);   // ADC read resolution
    analogSetWidth(12);         // ADC sampling resolution
    analogSetAttenuation(ADC_11db);  // ADC attenuation, with 11 dB the range is 150 to 2450 mV
    // Adjust CO2 sensor reading if resolution or attenuation is changed


    // Start SHT31 Temp and RH sensor
    M5.Lcd.println(F("Starting Temp/RH sensor..."));
    if (sht3x.begin() != 0) {
        M5.Lcd.println(F("Failed to initialize the chip, please confirm the wire connection"));
        delay(1000);
    }
    M5.Lcd.print(F("Chip serial number: "));
    M5.Lcd.println(sht3x.readSerialNumber());
    if (!sht3x.softReset()) {
        M5.Lcd.print(F("Failed to initialize the chip...."));
    }


    // Setup finish message
    M5.Lcd.println(F("Setup done!"));
    M5.Lcd.println(F("\nStarting in "));
    for (int i = 0; i <= 5; i++) {
        M5.Lcd.println(5 - i);
        if (i != 5) { delay(1000); }
        else { delay(250); }
    }
    M5.Lcd.clear(BLACK);
}



//////////////////////////////////////////////////////////////////////////
// The loop function runs over and over again until power down or reset //
//////////////////////////////////////////////////////////////////////////
void loop() {
    if (debug) {
        M5.Lcd.clear(BLACK);
        M5.Lcd.setTextDatum(TL_DATUM);
        M5.Lcd.setCursor(0, 10);
        M5.Lcd.setFreeFont(&FreeSans6pt7b);
        M5.Lcd.print(F("Loop start at: "));
        M5.Lcd.println(millis());
    }
    ////// State 1. Keep the Iot engine runing
    thing.handle();


    ////// State 2. Get current time from system time
    if (true) {
        s = second();
        m = minute();
        h = hour();
        dy = day();
        mo = month();
        yr = year();
        if (debug) { M5.Lcd.println((String)"Time: " + h + ":" + m + ":" + s); }
    }


    ////// State 3. Update system time from NTP server every minute
    if (timeClient.update()) { // It does not force-update NTC time (see NTPClient declaration for actual udpate interval)
        unix_t = timeClient.getEpochTime();
        unix_t = mxCT.toLocal(unix_t); // Conver to local time
        setTime(unix_t);   // set system time to given unix timestamp
        if (debug) { M5.Lcd.println(F("NTP client update success!")); }
    }
    else {
        if (debug) { M5.Lcd.println(F("NTP update not succesfull")); }
    }




    ////// State 4. Test if it is time to read gas sensor values (each second)
    if (s != LastSec) {
        O2Value = Oxygen.ReadOxygenData(COLLECT_NUMBER); //DFRobot_OxygenSensor Oxygen code
        sum = 0;
        for (int i = 0; i < n; i++) {
            sum += analogReadMilliVolts(CO2In);
        }
        CO2mVolt = sum / n;
        if (CO2mVolt < 400) { // Sensor in preheting if voltage is lower than 400 mV
            CO2ppm = 0;   // Set value to cero to identify faulty datapoints
        }
        else {
            CO2ppm = (CO2mVolt - 400) * CO2cal;
        }
        if (debug) {
            M5.Lcd.println((String)"Oxygene: " + O2Value + " %vol");
            M5.Lcd.println((String)"CO2: " + CO2ppm + " ppm");
        }
        LastSec = s;
    }


    ////// State 5. Test if it is time to read Temp and RH values
    ////// AND record sensor values for 5-minute averages (each minute)
    if (m != LastSum) {
        Temp = sht3x.getTemperatureC();
        RH = sht3x.getHumidityRH();
        if (debug) {
            M5.Lcd.println((String)"Temp: " + Temp + " °C");
            M5.Lcd.println((String)"RH: " + RH + " %");
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
            LogFile.println(F("Start position of last line send to IoT:\t1"));
            // Tabs added to prevent line ending with 0. Line ending with 0 indicates that line needs to be sent to IoT.
            LogFile.println(F("\t\t\t"));
            LogFile.println(F("Metadata:"));
            LogFile.println((String)"Station Number\t" + StaNum + "\t\t\t");
            LogFile.println((String)"Station Name\t" + StaName + "\t\t\t");
            LogFile.println((String)"Station Type\t" + StaType + "\t\t\t");
            LogFile.println((String)"Firmware\t" + Firmware + "\t\t\t");
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
            String(CO2cal, 4) + "\t" +
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


    ////// State 7. Test if there is data available to be sent to IoT cloud
    if (!PayloadRdy &&
        unix_t - t_DataBucket > DataBucket_frq - 15) {
        root.open("/");	// Open root directory
        root.rewind();	// Rewind root directory
        LogFile.openNext(&root, O_RDWR);
        while (LogFile.openNext(&root, O_RDWR)) {
            LogFile.rewind();
            LogFile.fgets(line, sizeof(line));     // Get first line
            str = String(line);
            if (debug) {
                M5.Lcd.print(F("File first line: "));
                M5.Lcd.println(str.substring(0, str.indexOf("\r")));
            }
            str = str.substring(str.indexOf("\t"), str.indexOf("\r"));
            if (str == "Done") {	// Skips file if year data is all sent to IoT
                LogFile.close();
                continue;
            }
            position = str.toInt();	// Sets file position to start of last line sent to IoT
            LogFile.seekSet(position);	// Set position to last line sent to LoRa
            // Read each line until a line not sent to IoT is found
            while (LogFile.available()) {
                position = LogFile.curPosition();  // START position of current line
                int len = LogFile.fgets(line, sizeof(line));
                if (line[len - 2] == '0') {
                    str = String(line); // str is the payload, next state test if there is internet connection to send payload to IoT
                    if (debug) {
                        M5.Lcd.println("Loteria, data to send to IoT found!");
                        M5.Lcd.print(F("Data: "));
                        M5.Lcd.println(str.substring(0, str.indexOf("\r")));
                    }
                    PayloadRdy = true;
                    break;
                }
            }
        }
        root.close();
        LogFile.close();
    }


    ////// State 8. Test if there is Internet and a Payload to sent SD data to IoT
    if (true) {
        if (debug) {
            M5.Lcd.print(F("Thing connected, payload ready and enought time has enlapsed: "));
            M5.Lcd.println(thing.is_connected() &&
                PayloadRdy &&
                unix_t - t_DataBucket > DataBucket_frq);
        }
        if (thing.is_connected() &&
            PayloadRdy &&
            unix_t - t_DataBucket > DataBucket_frq) {
            t_DataBucket = unix_t; // Record Data Bucket update TRY; even if it is not succesfful
            // extract data from payload string (str)
            for (int i = 0; i < HeaderN; i++) {
                String buffer = str.substring(0, str.indexOf('\t'));
                if (i != 6) { 	// Do not read seconds info
                    if (i == 0) {   // UNIX Time
                        SD_unix_t = buffer.toInt();
                    }
                    else if (i == 1) {
                        yrIoT = buffer.toInt();
                    }
                    else if (i == 2) {
                        moIoT = buffer.toInt();
                    }
                    else if (i == 3) {
                        dyIoT = buffer.toInt();
                    }
                    else if (i == 4) {
                        hIoT = buffer.toInt();
                    }
                    else if (i == 5) {
                        mIoT = buffer.toInt();
                    }
                    else if (i == 7) {  // O2
                        O2ValueAvg = buffer.toFloat();
                    }
                    else if (i == 8) {  // CO2
                        CO2ppmAvg = buffer.toFloat();
                    }
                    else if (i == 9) {  // Temp
                        TempAvg = buffer.toFloat();
                    }
                    else if (i == 10) { // RH
                        RHAvg = buffer.toFloat();
                    }


                }
                str = str.substring(str.indexOf('\t') + 1);
            }
            // send data to IoT. If succsessful, rewrite line in log File
            if (thing.write_bucket(iot_data_bucket, "Avg_Data", true) == 1) {
                if (debug) { M5.Lcd.println(F("Loteria, data on Cloud!!!")); }
                // Update line sent to IoT status
                if (debug) {
                    M5.Lcd.print(F("IoT year: "));
                    M5.Lcd.println(yrIoT);
                    M5.Lcd.print(F("File name: "));
                    M5.Lcd.println(FileName[yrIoT - 2020]);
                }
                LogFile.open(FileName[yrIoT - 2020], O_RDWR); // Open file containing the data just sent to IoT
                str = String(line);                     // Recover complete payload from original line
                str.setCharAt(str.length() - 2, '1');   // Replace 0 with 1, last characters are always "\r\n"
                LogFile.seekSet(position);              // Set position to start of line to be rewritten
                LogFile.println(str.substring(0, str.length() - 1));    // Remove last character ('\n') to prevent an empty line below rewritten line
                // Test if this line is last line of year Log File, if so, write "Done" at the end of first line
                if (moIoT == 12 && dyIoT == 31 && hIoT == 23 && mIoT == 55) {
                    LogFile.rewind();
                    LogFile.fgets(line, sizeof(line));     // Get first line
                    str = String(line);
                    str = str.substring(0, str.indexOf("\t"));
                    str = (String)str + "\t" + "Done";
                    LogFile.rewind();
                    LogFile.println(str.substring(0, str.length() - 1));    // Remove last character ('\n') to prevent an empty line below rewritten line
                    LogFile.close();
                }
                else {
                    // Update start position of last line sent to IoT
                    LogFile.rewind();
                    LogFile.fgets(line, sizeof(line));     // Get first line
                    str = String(line);
                    str = str.substring(0, str.indexOf("\t"));
                    str = (String)str + "\t" + position;
                    LogFile.rewind();
                    LogFile.println(str);
                    LogFile.close();
                }
                PayloadRdy = false;
            }
        }
    }


    ////// State 9. Update Screen
    if (!debug && (s != LastLcd)) {
        // Top left, Temp
        M5.Lcd.fillRect(0, 0, 160, 100, M5.Lcd.color565(255, 0, 0));
        M5.Lcd.setTextColor(M5.Lcd.color565(255, 255, 255));
        M5.Lcd.setFreeFont(&FreeSans7pt7b);
        M5.Lcd.setTextDatum(TL_DATUM);
        M5.Lcd.drawString(F("Air Temp (C):"), 10, 10);
        M5.Lcd.setFreeFont(&FreeSans20pt7b);
        M5.Lcd.setTextDatum(MC_DATUM);
        M5.Lcd.drawString(String(Temp), 80, 50);
        
        // Top right, Relative humidity
        M5.Lcd.fillRect(160, 0, 160, 100, M5.Lcd.color565(0, 0, 255));
        M5.Lcd.setTextColor(M5.Lcd.color565(255, 255, 255));
        M5.Lcd.setFreeFont(&FreeSans7pt7b);
        M5.Lcd.setTextDatum(TL_DATUM);
        M5.Lcd.drawString(F("Air RH (%):"), 10 + 160, 10);
        M5.Lcd.setFreeFont(&FreeSans20pt7b);
        M5.Lcd.setTextDatum(MC_DATUM);
        M5.Lcd.drawString(String(RH), 80 + 160, 50);

        // Bottom left, Oxygen
        M5.Lcd.fillRect(0, 100, 160, 100, M5.Lcd.color565(0, 255, 0));
        M5.Lcd.setTextColor(M5.Lcd.color565(255, 255, 255));
        M5.Lcd.setFreeFont(&FreeSans7pt7b);
        M5.Lcd.setTextDatum(TL_DATUM);
        M5.Lcd.drawString(F("Air Oxygen (%):"), 10, 10 + 100);
        M5.Lcd.setFreeFont(&FreeSans20pt7b);
        M5.Lcd.setTextDatum(MC_DATUM);
        M5.Lcd.drawString(String(O2Value), 80, 50 + 100);


        // Bottom right, Carbon dioxide
        M5.Lcd.fillRect(160, 100, 160, 100, M5.Lcd.color565(128, 128, 128));
        M5.Lcd.setTextColor(M5.Lcd.color565(255, 255, 255));
        M5.Lcd.setFreeFont(&FreeSans7pt7b);
        M5.Lcd.setTextDatum(TL_DATUM);
        M5.Lcd.drawString(F("Air CO2 (ppm):"), 10 +160, 10 + 100);
        M5.Lcd.setFreeFont(&FreeSans20pt7b);
        M5.Lcd.setTextDatum(MC_DATUM);
        M5.Lcd.drawString(String(CO2ppm), 80 + 160, 50 + 100);

        // Date and time
        M5.Lcd.fillRect(0, 200, 320, 240, M5.Lcd.color565(0, 0, 0));
        M5.Lcd.setTextColor(M5.Lcd.color565(255, 255, 255));
        M5.Lcd.setFreeFont(&FreeSans12pt7b);
        M5.Lcd.setTextDatum(ML_DATUM);
        M5.Lcd.drawString((String) dy + "/" + mo + "/" + yr + "  " + h + ":" + m + ":" + s, 10, 220);
        

        // IoT connection
        if (thing.is_connected()) {
            M5.Lcd.setFreeFont(&FreeSans12pt7b);
            M5.Lcd.setTextDatum(MR_DATUM);
            M5.Lcd.drawString(F("IoT"), 320 - 10, 220);
        }

        LastLcd = s;
    }


    ////// State 10. Test gas limits
    // Always test
    if (true) {
        if (O2Value < 19) { O2low = true; }
        else { O2low = false; }
        // High oxygene
        if (O2Value > 23) { O2high = true; }
        else { O2high = false; }
        // High carbon dioxide
        if (CO2ppm > 2000) { CO2high = true; }
        else { CO2high = false; }
    }


    ////// State 11 trigger alarm
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
                    thing.call_endpoint("Low_oxygen_PhotoLab_email", EmailData);
                }
                if (O2high) {
                    thing.call_endpoint("High_oxygen_PhotoLab_email", EmailData);
                }
                t_email = unix_t;
            }
        }
    }



    if (debug) {
        M5.Lcd.print(F("Loop end at: "));
        M5.Lcd.println(millis());
        M5.Lcd.println(F("Press button A to continue"));
        while (true) {
            thing.handle(); // Keep IoT engine runningat all times
            M5.update();    // Update buyyom state
            if (M5.BtnA.wasPressed()) {
                break;
            }
        }
    }
}
