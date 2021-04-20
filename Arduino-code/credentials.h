// credentials.h

#ifndef _CREDENTIALS_h
#define _CREDENTIALS_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

#define WIFI_SSID "Your SSID here"
#define WIFI_PASSWD "WLAN AP password here"
#define IoT_CREDENTIAL "Thinger credential here"

#endif

