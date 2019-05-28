void LoadConfigFromMemory()
{
	Serial.println("Loading config from memory.");

	if (EEPROM.read(0) != 228)
		WriteDefaultConfigToMemory();

	EEPROM.begin(100);
	_latitude = EEPROM.read(1);
	_longitude = EEPROM.read(2);
	_desiredTemperature = EEPROM.read(3);
	_cloudsSimulationPercent = EEPROM.read(4);
	EEPROM.end();
}

String GetJsonConfig()
{
	return "{ \"Latitude\": " + (String)_latitude + ", \"Longitude\": " + (String)_longitude + ", \"DesiredTemperature\": " + (String)_desiredTemperature + +", \"CloudsSimulationPercent\": " + (String)_cloudsSimulationPercent + " }";
}

void WriteConfigToMemory()
{
	Serial.println("Writing current config to memory.");
	EEPROM.begin(100);
	EEPROM.write(0, 228);
	EEPROM.write(1, _latitude);
	EEPROM.write(2, _longitude);
	EEPROM.write(3, _desiredTemperature);
	EEPROM.write(4, _cloudsSimulationPercent);
	EEPROM.end();
}

void WriteDefaultConfigToMemory()
{
	Serial.println("Writing default config to memory.");
	EEPROM.begin(100);
	EEPROM.write(0, 228);
	EEPROM.write(1, 1);
	EEPROM.write(2, 1);
	EEPROM.write(3, 20);
	EEPROM.write(4, 10);
	EEPROM.end();
}

void DisplayEEPROM()
{
	EEPROM.begin(100);
	for (int counter = 0; counter < 20; counter++)
	{
		Serial.println((String)EEPROM.read(counter));
	}
	EEPROM.end();
}