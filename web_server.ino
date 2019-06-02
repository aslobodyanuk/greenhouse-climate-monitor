void ConfigureWebServer()
{
	_webServer.on("/", HandleRootPage);
	_webServer.on("/getChartData", HandleGetChartsData);
	_webServer.on("/getLatestData", HandleGetLatestReadings);	
	_webServer.on("/getConfig", HandleGetConfig);
	_webServer.on("/saveConfig", HandleSaveConfig);
	_webServer.onNotFound([]() {
		if (!HandleFileRead(_webServer.uri()))
			_webServer.send(404, "text/plain", "404: Not Found");
	});
	_webServer.begin();
	
	Serial.println("HTTP server started");
}

String GetContentType(String filename) { // convert the file extension to the MIME type
	if (filename.endsWith(".html")) return "text/html";
	else if (filename.endsWith(".css")) return "text/css";
	else if (filename.endsWith(".js")) return "application/javascript";
	else if (filename.endsWith(".ico")) return "image/x-icon";
	else if (filename.endsWith(".woff")) return "application/font-woff";
	else if (filename.endsWith(".woff2")) return "application/font-woff2";
	else if (filename.endsWith(".ttf")) return "application/x-font-ttf";
	else if (filename.endsWith(".eot")) return "application/vnd.ms-fontobject";
	return "text/plain";
}

bool HandleFileRead(String path) { // send the right file to the client (if it exists)
	Serial.println("HandleFileRead: " + path);
	if (path.endsWith("/")) path = ROOT_PAGE_HTML_FILE_NAME;         // If a folder is requested, send the index file
	String contentType = GetContentType(path);            // Get the MIME type
	if (SPIFFS.exists(path)) {                            // If the file exists
		File file = SPIFFS.open(path, "r");                 // Open it
		size_t sent = _webServer.streamFile(file, contentType); // And send it to the client
		file.close();                                       // Then close the file again	
		return true;
	}
	Serial.println("\tFile Not Found");
	return false;                                         // If the file doesn't exist, return false
}

void HandleNotFound()
{
	_webServer.send(404, "text/plain", "Not found.");
}

void HandleRootPage()
{
	HandleFileRead(ROOT_PAGE_HTML_FILE_NAME);
}

void HandleGetConfig()
{
	String outputJson = GetJsonConfig();
	Serial.println("Sending config json: " + outputJson);
	_webServer.send(200, "application/json", outputJson);
}

void HandleGetChartsData()
{
	String outputJson = "{ \"TemperatureDayArray\": " + GetChartArrayForWeb(_temperatureDay) + ", \"HumidityDayArray\": " + GetChartArrayForWeb(_humidityDay) + ", \"LightDayArray\": " + GetChartArrayForWeb(_lightDay) + " }";
	Serial.println("Sending charts json: " + outputJson);
	_webServer.send(200, "application/json", outputJson);
}

void HandleGetLatestReadings()
{
	String outputJson = "{ \"Temperature\": " + (String)_lastTemperatureValue + ", \"Humidity\": " + (String)_lastHumidityValue + ", \"Light\": " + (String)_lastLightValue + ", \"Time\": \"" + _timeClient.getFormattedTime() + "\", \"UptimeSeconds\": " + millis() / 1000 + " }";
	Serial.println("Sending latest values json: " + outputJson);
	_webServer.send(200, "application/json", outputJson);
}

void HandleSaveConfig()
{
	_configuration.Latitude = _webServer.arg("Latitude").toFloat();
	_configuration.Longitude = _webServer.arg("Longitude").toFloat();
	_configuration.DesiredTemperature = _webServer.arg("DesiredTemperature").toFloat();
	_configuration.DesiredLightning = _webServer.arg("DesiredLightning").toFloat();
	_configuration.CloudsSimulationPercent = _webServer.arg("CloudsSimulationPercent").toFloat();

	Serial.print("Recieved new config: ");
	Serial.println(GetJsonConfig());

	WriteConfigToMemory();

	Serial.println("Saved new config.");
	_webServer.send(200, "text/plain", "OK");
}

String GetChartArrayForWeb(float chartValues[])
{
	int numOfCorrectValues = 0;
	float tempValues[DAY_CHART_ARRAY_LENGTH];
	for (int counter = 0; counter < DAY_CHART_ARRAY_LENGTH; counter++)
	{
		if (isnan(chartValues[counter]) == false)
		{
			tempValues[numOfCorrectValues] = chartValues[counter];
			numOfCorrectValues++;
		}
	}
	String output = "[";
	for (int counter = 0; counter < numOfCorrectValues; counter++)
	{
		if (counter + 1 == numOfCorrectValues)
			output += tempValues[counter];
		else
			output += (String)tempValues[counter] + ",";
	}
	output += "]";
	return output;
}
