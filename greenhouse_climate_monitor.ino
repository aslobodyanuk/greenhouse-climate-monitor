#include <ArduinoJson.hpp>
#include <ArduinoJson.h>
#include <EEPROM.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <DHT.h>
#include <Wire.h>
#include <BH1750.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <FS.h>
#include <SimpleKalmanFilter.h>

#include "Models.h"

#define DHT_PIN D4
#define DHT_TYPE DHT21
#define READ_DHT_EVERY 1000

#define LIGHT_METER_SDA_PIN D3
#define LIGHT_METER_SCL_PIN D5
#define READ_LIGHT_METER_EVERY 1000
#define LIGHT_METER_ADDRESS 0x23

#define PROXIMITY_SENSOR_ECHO_PIN D7
#define PROXIMITY_SENSOR_TRIG_PIN D6
#define READ_PROXIMITY_EVERY 1000

#define ROOT_PAGE_HTML_FILE_NAME "/dashboard.html"

#define UTC_OFFSET_SECONDS 10800
#define TIME_HOST_NAME "pool.ntp.org"
#define UPDATE_TIME_EVERY 1000

#define WRITE_TO_CHART_EVERY 3600
#define DAY_CHART_ARRAY_LENGTH 24

const char *WIFI_SSID = "DNIWE";
const char *WIFI_PASSWORD = "9hAiW%oR08521";

ESP8266WebServer _webServer(80);
DHT _dhtSensor(DHT_PIN, DHT_TYPE);
BH1750 _lightMeter(LIGHT_METER_ADDRESS);
SimpleKalmanFilter _humidityFilter(2, 2, 0.01);
SimpleKalmanFilter _temperatureFilter(2, 2, 0.01);
SimpleKalmanFilter _lightFilter(2, 2, 0.01);
WiFiUDP _ntpUDP;
NTPClient _timeClient(_ntpUDP, TIME_HOST_NAME, UTC_OFFSET_SECONDS);
//StaticJsonDocument<200> _jsonMemory;

unsigned long _lastReadDhtSensor;
unsigned long _lastReadLightSensor;
unsigned long _lastReadProximitySensor;
unsigned long _lastTimeUpdate;

float _lastHumidityValue;
float _lastTemperatureValue;
float _lastLightValue;

int _currentDay;
int _lastHourWrittenChart;
float _temperatureDay[DAY_CHART_ARRAY_LENGTH];
float _humidityDay[DAY_CHART_ARRAY_LENGTH];
float _lightDay[DAY_CHART_ARRAY_LENGTH];

Configuration _configuration;

void setup()
{	
	Wire.begin(LIGHT_METER_SDA_PIN, LIGHT_METER_SCL_PIN);
	pinMode(PROXIMITY_SENSOR_TRIG_PIN, OUTPUT);
	pinMode(PROXIMITY_SENSOR_ECHO_PIN, INPUT);

	Serial.begin(9600);

	_lastReadDhtSensor = millis();
	_lastReadLightSensor = millis();
	_lastReadProximitySensor = millis();
	_lastTimeUpdate = millis();

	_dhtSensor.begin();
	SPIFFS.begin();

	DisplayEEPROM();
	LoadConfigFromMemory();

	Serial.println("Connecting to wifi...");
	WiFi.mode(WIFI_STA);
	WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
	while (WiFi.waitForConnectResult() != WL_CONNECTED)
	{
		Serial.println("\r\nConnection Failed! Rebooting...");
		delay(5000);
		ESP.restart();
	}

	IPAddress myIP = WiFi.localIP();
	Serial.print("My IP address: ");
	Serial.println(myIP);

	ConfigureWebServer();	

	if (_lightMeter.begin(BH1750::ONE_TIME_HIGH_RES_MODE)) {
		Serial.println(F("BH1750 initialized."));
	}
	else {
		Serial.println(F("Error initializing BH1750."));
	}

	_timeClient.begin();
	_timeClient.update();
	_currentDay = _timeClient.getDay();
	_lastHourWrittenChart = _timeClient.getHours();

	ClearChartArrays();

	FillArraysWithRandomNumbers();	
}

void loop()
{
	_webServer.handleClient();

	if (millis() - _lastReadDhtSensor > READ_DHT_EVERY)
	{
		float temperatureReading = _dhtSensor.readTemperature();
		float humidityReading = _dhtSensor.readHumidity();

		if (!isnan(temperatureReading))
			_lastTemperatureValue = _temperatureFilter.updateEstimate(temperatureReading);

		if (!isnan(humidityReading))
			_lastHumidityValue = _humidityFilter.updateEstimate(humidityReading);

		Serial.print("Temp: ");
		Serial.print(temperatureReading);
		Serial.print(" E: ");
		Serial.println(_lastTemperatureValue);

		Serial.print("Humidity: ");
		Serial.print(humidityReading);
		Serial.print(" E: ");
		Serial.println(_lastHumidityValue);

		_lastReadDhtSensor = millis();
	}

	if (millis() - _lastReadLightSensor > READ_LIGHT_METER_EVERY)
	{
		float lux = _lightMeter.readLightLevel();

		if (!isnan(lux))
			_lastLightValue = _lightFilter.updateEstimate(lux);

		Serial.print("Light: ");
		Serial.print(lux);
		Serial.print(" E: ");
		Serial.println(_lastLightValue);

		_lastReadLightSensor = millis();
	}

	/*if (millis() - _lastReadProximitySensor > READ_PROXIMITY_EVERY)
	{
		int duration, cm;
		digitalWrite(PROXIMITY_SENSOR_TRIG_PIN, LOW);
		delayMicroseconds(2);
		digitalWrite(PROXIMITY_SENSOR_TRIG_PIN, HIGH);
		delayMicroseconds(10);
		digitalWrite(PROXIMITY_SENSOR_TRIG_PIN, LOW);
		duration = pulseIn(PROXIMITY_SENSOR_ECHO_PIN, HIGH);

		if (duration > 0)
		{
			Serial.print(duration);
			Serial.print(" dur   ");

			cm = duration / 58;
			Serial.print(cm);
			Serial.println(" cm");
		}

		_lastReadProximitySensor = millis();
	}*/

	if (millis() - _lastTimeUpdate > UPDATE_TIME_EVERY)
	{
		_timeClient.update();

		Serial.print("Hour: ");
		Serial.println(_timeClient.getHours());

		Serial.print("Time: ");
		Serial.println(_timeClient.getEpochTime());

		Serial.print("LastHour: ");
		Serial.println(_lastHourWrittenChart);

		//Update chart values every hour for a day
		if (_timeClient.getHours() > _lastHourWrittenChart || (_timeClient.getHours() == 0 && _lastHourWrittenChart == 23))
		{
			_lastHourWrittenChart = _timeClient.getDay();
			UpdateChartData();
		}

		_lastTimeUpdate = millis();
	}
}

void FillArraysWithRandomNumbers()
{
	for (int counter = 0; counter < DAY_CHART_ARRAY_LENGTH; counter++)
	{
		_temperatureDay[counter] = random(0, 30);
		_humidityDay[counter] = random(0, 30);
		_lightDay[counter] = random(0, 30);
	}
}

void UpdateChartData()
{
	Serial.println("Updating chart data.");
	//If last reading was faulty - reset state, to ensure that data will be written one more time
	if (LastReadingAvaliable() == false)
	{
		_lastHourWrittenChart = _timeClient.getDay() - 1;
		return;
	}

	if (_timeClient.getDay() != _currentDay)
	{
		ClearChartArrays();
		_currentDay = _timeClient.getDay();
	}
	FillNewValuesInChart();
}

void ClearChartArrays()
{
	for (int counter = 0; counter < DAY_CHART_ARRAY_LENGTH; counter++)
	{
		_temperatureDay[counter] = NAN;
		_humidityDay[counter] = NAN;
		_lightDay[counter] = NAN;
	}
}

void FillNewValuesInChart()
{
	if (LastReadingAvaliable())
	{
		_temperatureDay[_timeClient.getHours()] = _lastTemperatureValue;
		_humidityDay[_timeClient.getHours()] = _lastHumidityValue;
		_lightDay[_timeClient.getHours()] = _lastLightValue;
	}
}

bool LastReadingAvaliable()
{
	if (isnan(_lastHumidityValue) == false
		&& isnan(_lastTemperatureValue) == false
		&& isnan(_lastLightValue) == false)
		return true;
	else
		return false;
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