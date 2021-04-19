#include <M5Stack.h>
#include <WiFi.h>

unsigned long BeepStr = 0;
const int Note = 2000;      // Alarm beep note frequency in Hz
const int BeepLgth = 250;   // Alarm beep length in ms

#define NOTE_D0 -1
#define NOTE_D1 294
#define NOTE_D2 330
#define NOTE_D3 350
#define NOTE_D4 393
#define NOTE_D5 441
#define NOTE_D6 495
#define NOTE_D7 556

#define NOTE_DL1 147
#define NOTE_DL2 165
#define NOTE_DL3 175
#define NOTE_DL4 196
#define NOTE_DL5 221
#define NOTE_DL6 248
#define NOTE_DL7 278

#define NOTE_DH1 589
#define NOTE_DH2 661
#define NOTE_DH3 700
#define NOTE_DH4 786
#define NOTE_DH5 882
#define NOTE_DH6 990
#define NOTE_DH7 112


void setup() {
    // Initialize the M5Stack object
    M5.begin();

    /*
      Power chip connected to gpio21, gpio22, I2C device
      Set battery charging voltage and current
      If used battery, please call this function in your project
    */
    M5.Power.begin();
    //M5.Speaker.setBeep(NOTE_DH2, 2000);
    //M5.Speaker.setVolume(128);
    //M5.update();

    M5.Lcd.printf("M5Stack Speaker test:\r\n");





}

void loop() {
    if (M5.BtnA.wasPressed()) {
        M5.Lcd.printf("A wasPressed \r\n");
        M5.Speaker.beep(); //beep
        M5.update();
    }


    if (M5.BtnB.wasPressed())
    {
        M5.Lcd.printf("B wasPressed \r\n");
        M5.Speaker.tone(NOTE_DH2, 1000); //frequency 3000, with a duration of 200ms
        M5.update();
    }

    for (int i = 0; i <= 10; i++) {
        BeepStr = millis();
        int j = 0;
        while (millis() - BeepStr < BeepLgth) {
            if (j == 0) {
                M5.Speaker.tone(Note, BeepLgth);
                M5.update();
            }
            j += 1;
            //thing.handle();     // Keep IoT engine running
        }
        while (millis() - BeepStr < (BeepLgth * 2)) {
            M5.Speaker.mute();
            //thing.handle();     // Keep IoT engine running
        }
    }



}