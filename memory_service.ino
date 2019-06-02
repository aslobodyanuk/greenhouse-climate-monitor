void LoadConfigFromMemory()
{
	Serial.println("Loading config from memory.");
	
	EEPROM.begin(100);
	if (EEPROM.read(0) != 228)
		WriteDefaultConfigToMemory();

	_configuration.Latitude = EEPROM.read(1);
	_configuration.Longitude = EEPROM.read(2);
	_configuration.DesiredTemperature = EEPROM.read(3);
	_configuration.DesiredLightning = EEPROM.read(4);
	_configuration.CloudsSimulationPercent = EEPROM.read(5);
	EEPROM.end();
}

String GetJsonConfig()
{
	return "{ \"Latitude\": " + (String)_configuration.Latitude + ", \"Longitude\": " + (String)_configuration.Longitude + ", \"DesiredTemperature\": " + (String)_configuration.DesiredTemperature + ", \"CloudsSimulationPercent\": " + (String)_configuration.CloudsSimulationPercent + ", \"DesiredLightning\": " + (String)_configuration.DesiredLightning + " }";
}

void WriteConfigToMemory()
{
	Serial.println("Writing current config to memory.");
	EEPROM.begin(100);
	EEPROM.write(0, 228);
	EEPROM.write(1, _configuration.Latitude);
	EEPROM.write(2, _configuration.Longitude);
	EEPROM.write(3, _configuration.DesiredLightning);
	EEPROM.write(4, _configuration.DesiredTemperature);
	EEPROM.write(5, _configuration.CloudsSimulationPercent);
	EEPROM.end();
}

void WriteDefaultConfigToMemory()
{
	Serial.println("Writing default config to memory.");
	Configuration defaultConfig = GetDefaultConfiguration();
	EEPROM.begin(100);
	EEPROM.write(0, 228);
	EEPROM.write(1, defaultConfig.Latitude);
	EEPROM.write(2, defaultConfig.Longitude);
	EEPROM.write(3, defaultConfig.DesiredTemperature);
	EEPROM.write(4, defaultConfig.DesiredLightning);
	EEPROM.write(5, defaultConfig.CloudsSimulationPercent);
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

Configuration GetDefaultConfiguration()
{
	Configuration output;
	output.Latitude = 1;
	output.Longitude = 1;
	output.DesiredTemperature = 20.5;
	output.DesiredLightning = 500;
	output.CloudsSimulationPercent = 20;
	return output;
}