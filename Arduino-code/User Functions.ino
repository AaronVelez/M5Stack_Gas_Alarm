////// Get time from NTP an dupdate RTC and local time variables
bool GetNTPTime() {
    if (timeClient.forceUpdate()) {
        UTC_t = timeClient.getEpochTime();
        // set system time to UTC unix timestamp
        setTime(UTC_t);
        // Set RTC time to UTC time from system time
        rtc.adjustRtc( year(), month(), day(), weekday() - 1, hour(), minute(), second() );
        // Convert to local time
        local_t = mxCT.toLocal(UTC_t);
        // Set system time lo local time
        setTime(local_t);
        LastNTP = UTC_t;
        if (debug) { M5.Lcd.println(F("NTP client update success!")); }
        return true;
    }
    else {
        if (debug) { M5.Lcd.println(F("NTP update not succesfull")); }
        return false;
    }
}


////// Get time from RTC and update UTC and local time variables
void GetRTCTime() {
    rtc.read();                         // Get UTC time from RTC
    setTime(rtc.hour,                   // Set system time to UTC time
        rtc.minute,
        rtc.second,
        rtc.day,
        rtc.month,
        rtc.year);
    UTC_t = now();                      // Get UTC time from system time in UNIX format
    local_t = mxCT.toLocal(UTC_t);      // Calculate local time in UNIX format
    setTime(local_t);                   // Set system time to local
    s = second();                       // Set time variables to local time from system time
    m = minute();
    h = hour();
    dy = day();
    mo = month();
    yr = year();
}
