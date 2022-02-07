// Function to get setpoints from txt files form SD card
float Get_Setpoint(const char* Setpoint_File) {

	ExFile SetpointsFile;
	char read[25];
	String read_str;
	float setpoint;
	bool FileOK = SetpointsFile.open(Setpoint_File, O_RDWR);
	if (debug) {
		Serial.print(F("Setpoints file name: "));
		Serial.println(Setpoint_File);
		Serial.print(F("File open success: "));
		Serial.println(FileOK);
	}
	SetpointsFile.rewind();
	SetpointsFile.fgets(read, sizeof(read));     // Get first line
	SetpointsFile.close();

	read_str = String(read);
	read_str = read_str.substring(0, read_str.indexOf("\r"));
	setpoint = read_str.toFloat();
	if (debug) {
		Serial.print(F("SD setpoint: "));
		Serial.println(setpoint);
	}
	return setpoint;

}


// Function to set setpoints from txt files form SD card
void Set_Setpoint(const char* Setpoint_File, float setpoint) {

	ExFile SetpointsFile;

	SetpointsFile.remove(Setpoint_File);
	SetpointsFile.open(Setpoint_File, O_RDWR | O_CREAT);

	SetpointsFile.println(setpoint);
	SetpointsFile.close();

}
