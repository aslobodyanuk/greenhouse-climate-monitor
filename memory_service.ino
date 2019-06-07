void LoadConfigFromMemory()
{
	Serial.println("Loading config from memory.");
	
	int curAddress = 0;
	EEPROM.begin(100);
	if (EEPROM.read(curAddress) != 228)
		WriteDefaultConfigToMemory();
	
	curAddress += sizeof(int);
	_configuration = EEPROM.get(curAddress, _configuration);
	EEPROM.end();
}

String GetJsonConfig()
{
	return "{ \"Latitude\": " + (String)_configuration.Latitude + ", \"Longitude\": " + (String)_configuration.Longitude + ", \"DesiredTemperature\": " + (String)_configuration.DesiredTemperature + ", \"CloudsSimulationPercent\": " + (String)_configuration.CloudsSimulationPercent + ", \"DesiredLightning\": " + (String)_configuration.DesiredLightning + ", \"SimulateData\": " + (String)_configuration.SimulateData + " }";
}

void WriteConfigToMemory()
{
	Serial.println("Writing current config to memory.");
	int curAddress = 0;
	EEPROM.begin(100);
	EEPROM.write(0, 228);
	curAddress += sizeof(int);
	EEPROM.put(curAddress, _configuration);
	EEPROM.end();

	_currentDayLengthCalculated = getCurrentDayLength(_timeClient.getEpochTime(), _configuration.Latitude, _configuration.Longitude);
	CalculateAndSetDesiredLightning();
}

void WriteDefaultConfigToMemory()
{
	Serial.println("Writing default config to memory.");
	Configuration defaultConfig = GetDefaultConfiguration();
	int curAddress = 0;
	EEPROM.begin(100);
	EEPROM.write(0, 228);
	curAddress += sizeof(int);
	EEPROM.put(curAddress, defaultConfig);
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
	output.SimulateData = false;
	return output;
}

void CalculateAndSetDesiredLightning()
{
	if (_configuration.CloudsSimulationPercent == 0)
		_desiredLightning = _configuration.DesiredLightning;
	else
		_desiredLightning = _configuration.DesiredLightning - (_configuration.DesiredLightning * _configuration.CloudsSimulationPercent / 100);
	Serial.print("Calculated new desired lightning: ");
	Serial.println(_desiredLightning);
}