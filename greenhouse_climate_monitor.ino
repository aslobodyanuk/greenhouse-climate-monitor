#include <AutoPID.h>
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
#define READ_LIGHT_METER_EVERY 100
#define LIGHT_METER_ADDRESS 0x23

#define PROXIMITY_SENSOR_ECHO_PIN D7
#define PROXIMITY_SENSOR_TRIG_PIN D6
#define READ_PROXIMITY_EVERY 1000

#define ROOT_PAGE_HTML_FILE_NAME "/dashboard.html"

#define MILLIS_IN_ONE_DAY 86400000
#define UTC_OFFSET_SECONDS 10800
#define TIME_HOST_NAME "pool.ntp.org"
#define UPDATE_TIME_EVERY 1000

#define WRITE_TO_CHART_EVERY 3600
#define DAY_CHART_ARRAY_LENGTH 24

#define DATA_SIMULATION_WRITE_EVERY 1000

#define WIFI_CONNECT_MAX_TRIES 2
#define WIFI_AP_NAME "Climate Monitor"
#define WIFI_AP_PASSWORD "12344321"

const char *WIFI_SSID = "DNIWE";
const char *WIFI_PASSWORD = "9hAiW%oR08521";

#define LIGHT_PID_TIME_STEP 100
#define LIGHT_PID_OUTPUT_MIN 0
#define LIGHT_PID_OUTPUT_MAX 255
#define LIGHT_PID_KP 0.066
#define LIGHT_PID_KI 0.1196
#define LIGHT_PID_KD 0.0091

#define LIGHT_OUTPUT_PIN D2

ESP8266WebServer _webServer(80);
DHT _dhtSensor(DHT_PIN, DHT_TYPE);
BH1750 _lightMeter(LIGHT_METER_ADDRESS);
SimpleKalmanFilter _humidityFilter(2, 2, 0.01);
SimpleKalmanFilter _temperatureFilter(2, 2, 0.01);
SimpleKalmanFilter _lightFilter(2, 2, 0.01);
WiFiUDP _ntpUDP;
NTPClient _timeClient(_ntpUDP, TIME_HOST_NAME, UTC_OFFSET_SECONDS);

//Total sun time in millis
unsigned long _totalSunTime = 0;
unsigned long _millisCurrentDayStart;

//Calculated current day length based on coordinates and Epoch time from NTP
double _currentDayLengthCalculated;
bool _enableAtrificialLightning = true;

unsigned long _lastReadDhtSensor;
unsigned long _lastReadLightSensor;
unsigned long _lastReadProximitySensor;
unsigned long _lastTimeUpdate;

unsigned long _dataSimulationLastWrite;
int _dataSimulationCurrentHour;

float _lastHumidityValue;
float _lastTemperatureValue;
float _lastLightValue;

int _currentDay;
int _lastHourWrittenChart;
float _temperatureDay[DAY_CHART_ARRAY_LENGTH];
float _humidityDay[DAY_CHART_ARRAY_LENGTH];
float _lightDay[DAY_CHART_ARRAY_LENGTH];

Configuration _configuration;

double _desiredLightning;
double _lastLightValuePID;
double _lightPIDOutput;
AutoPID _lightPID(&_lastLightValuePID, &_desiredLightning, &_lightPIDOutput, LIGHT_PID_OUTPUT_MIN, LIGHT_PID_OUTPUT_MAX, LIGHT_PID_KP, LIGHT_PID_KI, LIGHT_PID_KD);

void setup()
{
	//Initialize pins
	Wire.begin(LIGHT_METER_SDA_PIN, LIGHT_METER_SCL_PIN);
	pinMode(PROXIMITY_SENSOR_TRIG_PIN, OUTPUT);
	pinMode(PROXIMITY_SENSOR_ECHO_PIN, INPUT);
	pinMode(LIGHT_OUTPUT_PIN, OUTPUT);

	//Initialize serial communication
	Serial.begin(9600);

	//Initialize time counters
	_lastReadDhtSensor = millis();
	_lastReadLightSensor = millis();
	_lastReadProximitySensor = millis();
	_lastTimeUpdate = millis();
	_dataSimulationLastWrite = millis();
	_millisCurrentDayStart = millis();

	//Initialize sensors
	_dhtSensor.begin();
	if (_lightMeter.begin(BH1750::ONE_TIME_HIGH_RES_MODE)) {
		Serial.println(F("BH1750 initialized."));
	}
	else {
		Serial.println(F("Error initializing BH1750."));
	}

	//Initialize internal memory
	SPIFFS.begin();

	//Load configuration from EEPROM
	LoadConfigFromMemory();

	//Connect to wifi
	bool wifiConnectionResult = ConnectToWifi(0);
	if (wifiConnectionResult == false)
		CreateWiFiAPPoint();

	//Start web server
	ConfigureWebServer();

	//Initialize time client and all related variables
	_timeClient.begin();
	_timeClient.update();
	_currentDay = _timeClient.getDay();
	_lastHourWrittenChart = _timeClient.getHours();
	_currentDayLengthCalculated = getCurrentDayLength(_timeClient.getEpochTime(), _configuration.Latitude, _configuration.Longitude);

	//Calculate new desired lightning based on cloudness
	CalculateAndSetDesiredLightning();
	
	//Reset variables if SimulatedData mode enabled
	if (_configuration.SimulateData)
	{
		_currentDay = 0;
		_dataSimulationCurrentHour = 0;				
	}
	
	//Clear chart arrays with NAN values to ensure no bad data gets to charts
	ClearChartArrays();

	//Configure time step for PID library
	_lightPID.setTimeStep(LIGHT_PID_TIME_STEP);
}

void loop()
{	
	_lightPID.run();

	if (_enableAtrificialLightning)
		analogWrite(LIGHT_OUTPUT_PIN, _lightPIDOutput);

	/*Serial.print("Light: ");
	Serial.print(_lastLightValuePID);
	Serial.print(" PID: ");
	Serial.print(_lightPIDOutput);
	Serial.print(" Millis: ");
	Serial.println(millis());*/

	_webServer.handleClient();

	if (millis() - _lastReadDhtSensor > READ_DHT_EVERY)
	{
		float temperatureReading = _dhtSensor.readTemperature();
		float humidityReading = _dhtSensor.readHumidity();

		if (!isnan(temperatureReading))
			_lastTemperatureValue = _temperatureFilter.updateEstimate(temperatureReading);

		if (!isnan(humidityReading))
			_lastHumidityValue = _humidityFilter.updateEstimate(humidityReading);

		/*Serial.print("Temp: ");
		Serial.print(temperatureReading);
		Serial.print(" E: ");
		Serial.println(_lastTemperatureValue);

		Serial.print("Humidity: ");
		Serial.print(humidityReading);
		Serial.print(" E: ");
		Serial.println(_lastHumidityValue);*/

		_lastReadDhtSensor = millis();
	}

	if (millis() - _lastReadLightSensor > READ_LIGHT_METER_EVERY)
	{
		float lux = _lightMeter.readLightLevel();

		if (!isnan(lux))
		{
			_lastLightValue = _lightFilter.updateEstimate(lux);
			_lastLightValuePID = lux;
		}

		if (millis() - _millisCurrentDayStart > MILLIS_IN_ONE_DAY)
		{
			_totalSunTime = 0;
			_millisCurrentDayStart = millis();
			Serial.println("Current millis day has passed away. Resetting millis day counter and total sun time.");
		}

		if (_lastLightValue >= _desiredLightning * 0.9)
			_totalSunTime += millis() - _lastReadLightSensor;		

		if (_totalSunTime / 1000 >= _currentDayLengthCalculated)
		{
			_enableAtrificialLightning = false;
			analogWrite(LIGHT_OUTPUT_PIN, 0);
			Serial.println("Reached 'currentDayLengthCalculated', disabling artificial lightning.");
		}
			
		/*Serial.print("Light: ");
		Serial.print(_lastLightValuePID);
		Serial.print(" PID Output: ");
		Serial.print(_lightPIDOutput);
		Serial.print(" Est: ");
		Serial.println(_lastLightValue);*/

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

	if (_configuration.SimulateData && millis() - _dataSimulationLastWrite > DATA_SIMULATION_WRITE_EVERY)
	{
		Serial.println("Writing simulation data.");
		if (_dataSimulationCurrentHour >= DAY_CHART_ARRAY_LENGTH)
		{
			_dataSimulationCurrentHour = 0;
			_currentDay++;
			ClearChartArrays();
		}

		UpdateChartData(_currentDay, _dataSimulationCurrentHour);
		_dataSimulationCurrentHour++;

		_dataSimulationLastWrite = millis();
	}
	else if (_configuration.SimulateData == false && millis() - _lastTimeUpdate > UPDATE_TIME_EVERY)
	{
		_timeClient.update();		

		/*Serial.print("Time: ");
		Serial.print(_timeClient.getFormattedTime());

		Serial.print(" MilisTime: ");
		Serial.println(_timeClient.getEpochTime());*/

		/*Serial.print("LastHour: ");
		Serial.println(_lastHourWrittenChart);*/

		//Update chart values every hour for a day
		if (_timeClient.getHours() > _lastHourWrittenChart || (_timeClient.getHours() == 0 && _lastHourWrittenChart == 23))
		{
			_lastHourWrittenChart = _timeClient.getHours();
			UpdateChartData(_timeClient.getDay(), _timeClient.getHours());
		}

		_lastTimeUpdate = millis();
	}
}

bool ConnectToWifi(int triesCounter)
{
	Serial.println("Connecting to wifi...");
	WiFi.mode(WIFI_STA);
	WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
	if (triesCounter <= WIFI_CONNECT_MAX_TRIES && WiFi.waitForConnectResult() != WL_CONNECTED)
	{
		Serial.println("\r\nConnection Failed! Trying again...");
		delay(3000);
		return ConnectToWifi(++triesCounter);
	}
	else if (WiFi.waitForConnectResult() != WL_CONNECTED)
	{
		Serial.println("Reached WIFI_CONNECT_MAX_TRIES, stopped trying to connect to WIFI.");
		return false;
	}

	IPAddress myIP = WiFi.localIP();
	Serial.print("My IP address: ");
	Serial.println(myIP);
	return true;
}

void CreateWiFiAPPoint()
{
	Serial.println("Creating a WIFI AP...");
	WiFi.mode(WIFI_AP);
	bool result = WiFi.softAP(WIFI_AP_NAME, WIFI_AP_PASSWORD);
	if (result == true)
	{
		Serial.print("Started a new WIFI AP with name: ");
		Serial.println(WIFI_AP_NAME);
	}
	else
		Serial.println("Failed to start a WIFI AP.");
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

void UpdateChartData(int currentDayReading, int currentHourReading)
{
	Serial.println("Updating chart data.");
	//If last reading was faulty - reset state, to ensure that data will be written one more time
	if (LastReadingAvaliable() == false)
	{
		Serial.println("Some of chart values are not avaliable.");
		_lastHourWrittenChart = currentHourReading - 1;
		return;
	}

	if (currentDayReading != _currentDay)
	{
		ClearChartArrays();
		_currentDay = currentDayReading;
	}
	FillNewValuesInChart(currentHourReading);
}

void ClearChartArrays()
{
	Serial.println("Clearing chart arrays.");
	for (int counter = 0; counter < DAY_CHART_ARRAY_LENGTH; counter++)
	{
		_temperatureDay[counter] = NAN;
		_humidityDay[counter] = NAN;
		_lightDay[counter] = NAN;
	}
}

void FillNewValuesInChart(int currentHourReading)
{
	if (LastReadingAvaliable())
	{
		_temperatureDay[currentHourReading] = _lastTemperatureValue;
		_humidityDay[currentHourReading] = _lastHumidityValue;
		_lightDay[currentHourReading] = _lastLightValue;
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