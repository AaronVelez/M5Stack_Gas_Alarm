#include <M5Stack.h>
#include <Wire.h>
#include <WiFi.h>

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


void setup() {
    ////// Initialize and setup M5Stack
    M5.begin();
    M5.Power.begin();
    M5.Lcd.println(F("M5 started"));

    M5.Lcd.print(F("Start SD: "));
    M5.Lcd.println(sd.begin(SD_CONFIG));



    M5.Lcd.println(F("Setup end"));
}

void loop() {


    if (!LogFile.exists("1985.txt")) {

        LogFile.open(("1985.txt"), O_RDWR | O_CREAT); // Create file

        // Add Metadata
        LogFile.println("Start position of last line send to IoT:\t0");
        LogFile.close(); // Close SD card file to save changes
    }


}
