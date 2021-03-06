/*
 ################################################
 # Team NYUAD, Green / Vaponic Wall Code v1.1.1 #
 # Slave module side code                       #
 # ESP8266 WIFI Module                          #
 # 2018                                         #
 # Author: woswos                               #
 ################################################

                                              //^\\
                                          //^\\ #
    q_/\/\   q_/\/\    q_/\/\   q_/\/\      #   #
 ||||`    /|/|`     <\<\`    |\|\`     #   #
 #*#*#**#**#**#*#*#**#**#**#****#*#**#**#**#*#*#**#**#
 */

/**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**/
/**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**/
// Default libraries
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPClient.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

// For running functions at intervals
// https://github.com/arkhipenko/TaskScheduler
#include <TaskScheduler.h>

// For parsing the data transferred from the server
// https://github.com/bblanchon/ArduinoJson
#include <ArduinoJson.h>

// Default I2C library
#include <Wire.h>

// Adafruit unified sensors library
#include <Adafruit_Sensor.h>

// For the light sensor
// https://github.com/adafruit/Adafruit_TSL2561
#include <Adafruit_TSL2561_U.h>

// For the pressure, altitute and temperature sensor
// https://github.com/adafruit/Adafruit_BMP085_Unified
#include <Adafruit_BMP085_U.h>

// For the humidity and temperature sensor (temperature is inccuarate)
// https://github.com/adafruit/DHT-sensor-library
#include <DHT.h>
#include <DHT_U.h>

// For the I2C ADC converter
// https://github.com/adafruit/Adafruit_ADS1X15
#include <Adafruit_ADS1015.h>

// For the one-wire interface
// https://github.com/PaulStoffregen/OneWire
#include <OneWire.h>

// For the dallas temperature sensor
// https://github.com/milesburton/Arduino-Temperature-Control-Library
#include <DallasTemperature.h>

// Defining the maximum avaliable pwm value for the pins of ESP8266
#define PWMRANGE 1023
/**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**/
/**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**//**/


bool enableDebugging = true;   // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< CHANGE THESE VARIABLES
bool enableSerialBanner = true;   // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< CHANGE THESE VARIABLES


////////////////////////////////////////////////////////
// slave unit vars begin:
// Device type option: fogger, light, pump, valve,
//                     lightSensor, pressureSensor, flowSensor, humiditySensor,
//                     phSensor, ecSensor
const String deviceType = "ecSensor";   // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< CHANGE THESE VARIABLES
const String deviceId = "1"; // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< CHANGE THESE VARIABLES

// Transistor control pin
const int transistorPin = 14;

// I2C Pins
const int sdaPin = 5;   // >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> bunları degistirmek simdilik bi ise yaramıyor
const int sclPin = 4;
//const int i2cAddressChangePin = 12;

// One wire interface pin (for sensors that communicate over only one data)
// Data pin is the digital I/O pin, not the analog one
int oneWireInterfacePin = 13;
int oneWireInterfacePin2 = 12;

int transistorPinPwmValue = 0;
int previousTransistorPinPwmValue = 0;
int tempDeviceIsEnabled = 1;

int deviceState = LOW;
unsigned long previousMillis = 0;
unsigned long dataUpdateInterval = 10000; // in milliseconds
long interval = 1000;

// Enable either deepsleep or wifi off or none
bool enableDeepSleep = false;
bool enableWifiOffWhenNotUsed = false;

String isSensor = "0";
String sensorReading = "0";
bool sensorDataTransferRequest = false;

// slave unit vars end;
////////////////////////////////////////////////////////


////////////////////////////////////////////////////////
// Update these with values suitable for your network.
// cnk vars begin:
unsigned long currentTime;
const char* ssid = "2.4g";
const char* password = "dauyndauyn";
// cnk vars end;
////////////////////////////////////////////////////////


////////////////////////////////////////////////////////
// http request vars begin:
String masterHostName = "vaponicwalltestpi";
String phpFile = "index.php";

String serverAddreess;
String query;
String payload = "-1";

bool httpRequestError = true;
bool updateDataUpdateIntervals = false;
unsigned long previousServerDataUpdateInterval = dataUpdateInterval;
bool lostServerConnection = false;

// Default values
int deviceIsEnabled = 0;
int runOnlyOnce = 1;
int powerPercent = 0;
unsigned long serverDataUpdateInterval = 1;
int serverEnableDeepSleep = 0;
int serverEnableWifiOffWhenNotUsed = 0;
long serverCurrentUnixTime;
unsigned long onDuration = 15;
unsigned long offDuration = 60;
// http request vars end;
////////////////////////////////////////////////////////


////////////////////////////////////////////////////////
// Callback prototypes for the task manager
void httpRequestCallback();
void httpRequestParserCallback();
void parsedDataToRegularSlaveUnitVariablesCallback();
void updateDataUpdateIntervalsCallback();
void runFoggerCallback();
void runLightCallback();
void runValveCallback();
void runPumpCallback();
void runSensorCallback();
////////////////////////////////////////////////////////


////////////////////////////////////////////////////////
//Tasks
Task tHttpRequest(dataUpdateInterval, TASK_FOREVER, &httpRequestCallback);

Task tHttpRequestParser(dataUpdateInterval, TASK_FOREVER, &httpRequestParserCallback);

Task tParsedDataToRegularSlaveUnitVariables(dataUpdateInterval, TASK_FOREVER, &parsedDataToRegularSlaveUnitVariablesCallback);

Task tUpdateDataUpdateIntervals(dataUpdateInterval, TASK_FOREVER, &updateDataUpdateIntervalsCallback);

// Fogger runs every seconds, so that it can run the run it on the seconds level (aka. resolution)
Task tRunFogger(1000, TASK_FOREVER, &runFoggerCallback);

Task tRunLight(dataUpdateInterval, TASK_FOREVER, &runLightCallback);

Task tRunValve(dataUpdateInterval, TASK_FOREVER, &runValveCallback);

Task tRunPump(dataUpdateInterval, TASK_FOREVER, &runPumpCallback);

Task tRunSensor((dataUpdateInterval+1000), TASK_FOREVER, &runSensorCallback);

Scheduler taskManager;
////////////////////////////////////////////////////////


////////////////////////////////////////////////////////
////////////////////// FUNCTIONS ///////////////////////
////////////////////////////////////////////////////////

// Setting up the system for the first use. This function executes the other
//      setup functions in the required order.
void setup() {

        ////////////////////////////////////////////////////////
        //////////////// SLAVE UNIT SETUP BEGIN ////////////////
        ////////////////////////////////////////////////////////
        pinMode(transistorPin, OUTPUT);
        digitalWrite(transistorPin, LOW);
        ////////////////////////////////////////////////////////
        ////////////// SLAVE UNIT SETUP COMPLETE ///////////////
        ////////////////////////////////////////////////////////


        ////////////////////////////////////////////////////////
        //////////////// WIFI - OTA SETUP BEGIN ////////////////
        ////////////////////////////////////////////////////////
        Serial.begin(115200);

        if(enableSerialBanner == true) {
                camelArtAndBanner();
        }

        Serial.println("Booting");

        setupWifi(); // wifi cnk procedure;
        arduinoOTASetup();
        ///////////////////////////////////////////////////////
        ////////////// WIFI - OTA SETUP COMPLETE //////////////
        ///////////////////////////////////////////////////////


        ///////////////////////////////////////////////////////
        ////////////////// SLAVE SETUP BEGIN //////////////////
        ///////////////////////////////////////////////////////

        slaveSetup();

        ///////////////////////////////////////////////////////
        //////////////// SLAVE SETUP COMPLETE /////////////////
        ///////////////////////////////////////////////////////


        ///////////////////////////////////////////////////////
        ////////// TASK MANAGER SETUP PART 1 - BEGIN //////////
        ///////////////////////////////////////////////////////

        setupTaskManagerAddTasks();

        ///////////////////////////////////////////////////////
        /////// TASK MANAGER SETUP PART 1 - COMPLETE //////////
        ///////////////////////////////////////////////////////


        ///////////////////////////////////////////////////////
        ///////// TASK MANAGER SETUP PART 2 - BEGIN ///////////
        ///////////////////////////////////////////////////////

        setupTaskManagerEnableTasks();

        ///////////////////////////////////////////////////////
        /////// TASK MANAGER SETUP PART 2 - COMPLETE //////////
        ///////////////////////////////////////////////////////

        Serial.println(" ");
        Serial.print("Millis(): ");
        Serial.println(millis());
        Serial.println("//////////////////////////////////");
        Serial.println("//        SETUP COMPLETED       //");
        Serial.println("//      RUNNING THE SYSTEM      //");
        Serial.println("//////////////////////////////////");
        Serial.println(" ");

        delay(1000);

}

// Sets  up the wifi network and ensures that the devices is connected to the
//     network with the given SSID and password.
void setupWifi() {
        // wifi network cnk begin:
        Serial.println();
        Serial.print("Connecting to ");
        Serial.println(ssid);

        if(enableWifiOffWhenNotUsed) {
                // Disable the WiFi persistence.  The ESP8266 will not load and save WiFi settings in the flash memory.
                WiFi.persistent( false );
        }

        WiFi.hostname("vaponicwall-" + deviceType + "-" + deviceId);

        WiFi.begin(ssid, password);
        while (WiFi.status() != WL_CONNECTED) {
                delay(200);
                Serial.print(".");
        }

        randomSeed(micros());

        Serial.println("");
        Serial.println("WiFi connected");
        Serial.println("IP address: ");
        Serial.println(WiFi.localIP());
        // wifi network cnk end;
}

// Arduino Over-The-Air updates setup.
// This is usefull for uplading new code to the chip without actually connecting
//      the usb cable.
void arduinoOTASetup() {
        // ota setup begin:

        // Port defaults to 8266
        // ArduinoOTA.setPort(8266);

        // Hostname defaults to esp8266-[ChipID]
        String slaveHostName =  "vaponicwall " + deviceType + " " + deviceId;

        // Length (with one extra character for the null terminator)
        int str_len = slaveHostName.length() + 1;

        // Prepare the character array (the buffer)
        char char_array[str_len];

        slaveHostName.toCharArray(char_array, str_len);

        ArduinoOTA.setHostname(char_array);

        // No authentication by default
        // ArduinoOTA.setPassword((const char *)"123");

        ArduinoOTA.onStart([]() {
                Serial.println("Start");

                // Disable tasks during the update
                taskManager.disableAll();
        });
        ArduinoOTA.onEnd([]() {
                Serial.println("\nEnd");
        });
        ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {

                // Disable tasks during the update
                taskManager.disableAll();

                Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
        });
        ArduinoOTA.onError([](ota_error_t error) {
                Serial.printf("Error[%u]: ", error);
                if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
                else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
                else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
                else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
                else if (error == OTA_END_ERROR) Serial.println("End Failed");

                // Enable tasks if there is an error, so that system can keep running
                taskManager.enableAll();
        });
        ArduinoOTA.begin();
        // ota setup end;

}

// Gets the initial variables from the server and makes sure that the system is
//      ready for the first run.
void slaveSetup() {

        // A line break for more readibility
        Serial.println(" ");
        Serial.println("/////////////////////////////////////////////////////////////////////////////////////");
        Serial.println("Connecting to server to get slave configuration");
        Serial.println("Please make sure that the following variables are correct for this slave device");
        Serial.print("Device Type: ");
        Serial.println(deviceType);
        Serial.print("Device ID: ");
        Serial.println(deviceId);
        Serial.println("/////////////////////////////////////////////////////////////////////////////////////");

        httpRequestCallback();

        httpRequestParserCallback();

        // Need to keep the device out of the inifite reboot loop
        updateDataUpdateIntervals = false;

        parsedDataToRegularSlaveUnitVariablesCallback();

        if(httpRequestError == false) {

                // A line break for more readibility
                Serial.println(" ");

                Serial.println("//////////////////////////////////");
                Serial.print("Updated variables:");
                Serial.println(" ");

                // Output to serial monitor
                Serial.print("deviceIsEnabled:");
                Serial.println(deviceIsEnabled);

                Serial.print("runOnlyOnce:");
                Serial.println(runOnlyOnce);

                Serial.print("powerPercent:");
                Serial.println(powerPercent);

                Serial.print("onDuration (milliseconds):");
                Serial.println(onDuration);

                Serial.print("offDuration (milliseconds):");
                Serial.println(offDuration);

                Serial.print("serverDataUpdateInterval (milliseconds):");
                Serial.println(dataUpdateInterval);

                Serial.print("serverEnableDeepSleep:");
                Serial.println(serverEnableDeepSleep);

                Serial.print("serverEnableWifiOffWhenNotUsed:");
                Serial.println(serverEnableWifiOffWhenNotUsed);

                Serial.print("serverCurrentUnixTime:");
                Serial.println(serverCurrentUnixTime);

                Serial.println("//////////////////////////////////");
                // A line break for more readibility
                Serial.println(" ");

        } else {
                // A line break for more readibility
                Serial.println(" ");

                Serial.println("///////////////////////////////////////////////////////////////////");
                Serial.println("// Couldn't update variables due to the server connection error  //");
                Serial.println("///////////////////////////////////////////////////////////////////");

                // A line break for more readibility
                Serial.println(" ");

        }

}

// Adds all of the tasks delared to the task manager.
void setupTaskManagerAddTasks() {

        Serial.println(" ");

        Serial.println("////////////////////////////////////////////////////////////////////");
        taskManager.init();
        Serial.println("*Initialized Scheduler (Task Manager)*");

        Serial.println(" ");

        taskManager.addTask(tHttpRequest);
        tHttpRequest.setInterval(dataUpdateInterval);
        Serial.println("Added 'HTTP request' task");

        taskManager.addTask(tHttpRequestParser);
        tHttpRequestParser.setInterval(dataUpdateInterval);
        Serial.println("Added 'HTTP request parser' task");

        taskManager.addTask(tParsedDataToRegularSlaveUnitVariables);
        tParsedDataToRegularSlaveUnitVariables.setInterval(dataUpdateInterval);
        Serial.println("Added 'Parsed data to regular slave unit variables' task");

        taskManager.addTask(tUpdateDataUpdateIntervals);
        tUpdateDataUpdateIntervals.setInterval(dataUpdateInterval);
        Serial.println("Added 'Update data update intervals' task");

        taskManager.addTask(tRunFogger);
        Serial.println("Added 'Run fogger' task");

        taskManager.addTask(tRunLight);
        tRunLight.setInterval(dataUpdateInterval);
        Serial.println("Added 'Run light' task");

        taskManager.addTask(tRunValve);
        tRunValve.setInterval(dataUpdateInterval);
        Serial.println("Added 'Run valve' task");

        taskManager.addTask(tRunPump);
        tRunPump.setInterval(dataUpdateInterval);
        Serial.println("Added 'Run pump' task");

        taskManager.addTask(tRunSensor);
        tRunSensor.setInterval(dataUpdateInterval);
        Serial.println("Added 'Run sensor' task");
        Serial.println("////////////////////////////////////////////////////////////////////");

}

// Only enables the required tasks on the task manager.
void setupTaskManagerEnableTasks() {

        Serial.println(" ");
        Serial.println("////////////////////////////////////////////////////////////////////");

        tHttpRequest.enable();
        Serial.println("Enabled 'HTTP request' task");

        tHttpRequestParser.enable();
        Serial.println("Enabled 'HTTP request parser' task");

        tParsedDataToRegularSlaveUnitVariables.enable();
        Serial.println("Enabled 'Parsed data to regular slave unit variables' task");

        tUpdateDataUpdateIntervals.enable();
        Serial.println("Enabled 'Update data update intervals' task");

        if((deviceType == "fogger") or (deviceType == "FOGGER") or (deviceType == "Fogger")) {

                isSensor = "0";

                tRunFogger.enable();
                Serial.println("////////////////////////////////////////////////////////////////////");
                Serial.println("//////////           Enabled 'Run fogger' task            //////////");

        } else if ((deviceType == "light") or (deviceType == "LIGHT") or (deviceType == "Light")) {

                isSensor = "0";

                tRunLight.enable();
                Serial.println("////////////////////////////////////////////////////////////////////");
                Serial.println("//////////           Enabled 'Run light' task            //////////");

        } else if ((deviceType == "valve") or (deviceType == "VALVE") or (deviceType == "Valve")) {

                isSensor = "0";

                tRunValve.enable();
                Serial.println("////////////////////////////////////////////////////////////////////");
                Serial.println("//////////           Enabled 'Run valve' task            //////////");

        } else if ((deviceType == "pump") or (deviceType == "PUMP") or (deviceType == "Pump")) {

                isSensor = "0";

                tRunPump.enable();
                Serial.println("////////////////////////////////////////////////////////////////////");
                Serial.println("//////////           Enabled 'Run pump' task            //////////");

        } else if ((deviceType.indexOf("Sensor") > 0) or (deviceType.indexOf("SENSOR") > 0) or (deviceType.indexOf("sensor") > 0)) {

                isSensor = "1";

                tRunSensor.enable();
                Serial.println("////////////////////////////////////////////////////////////////////");
                Serial.println("//////////           Enabled 'Run sensor' task            //////////");
        }

        Serial.println("////////////////////////////////////////////////////////////////////");

}

// Prints the banner for the code and a camel art to the serial monitor.
// This is here only for fun purposes and it doesn't serve any functionality
//      in the first place. That being said, I like that ASCII camel art :)
void camelArtAndBanner() {

        Serial.println(" ");

        Serial.println("################################################");
        Serial.println("# Team NYUAD, Green / Vaponic Wall Code v1.1.1 #");
        Serial.println("# Sleve module side code                       #");
        Serial.println("# ESP8266 WIFI Module                          #");
        Serial.println("# 2018                                         #");
        Serial.println("# Author: woswos                               #");
        Serial.println("################################################");

        Serial.println(" ");

        // This looks reqular on serial monitor
        Serial.println("                                              //^\\      ");
        Serial.println("                                          //^\\ #        ");
        Serial.println("     q_/\/\     q_/\/\       q_/\/\  q_/\/\        #   # ");
        Serial.println("       ||||`    /|/|`     <\<\`    |\|\`       #   #     ");
        Serial.println("   #*#*#**#**#**#*#*#**#**#**#****#*#**#**#**#*#*#**#**# ");

        Serial.println(" ");

}

// Does the https requests for the code. Connects to the main server and gets
//      the reply in terms of a JSON formatted string.
// This function also tries to handle possible server connection errors to keep
//      the system running with the old information even if there is a problem
//      with the main server.
void httpRequestCallback() {

        // wait for WiFi connection
        if ((WiFi.status() == WL_CONNECTED)) {

                HTTPClient http;

                serverAddreess = "http://" + masterHostName + "/" + phpFile;

                http.begin(serverAddreess);

                http.addHeader("Content-Type", "application/x-www-form-urlencoded; charset=UTF-8");

                String deviceDataTransferRequest = " ";

                // If the device is not a sensor, don't send a reading accidentally
                if (sensorDataTransferRequest) {
                        deviceDataTransferRequest = "1";
                } else {
                        deviceDataTransferRequest = "0";
                        sensorReading = "0";
                }

                query = "deviceType=" + deviceType + "&deviceId=" + deviceId + "&deviceIp=" + WiFi.localIP().toString() + "&deviceDataTransferRequest=" + deviceDataTransferRequest + "&dataToTransfer=" + sensorReading;

                // For debugging
                //Serial.println(query);

                // Send query to the server
                http.POST(query);

                // For debugging
                //http.writeToStream(&Serial);

                // Retrieve the returned result
                payload = http.getString();

                http.end();

                // Change the http request error flag if the result is unsuccessful
                if((payload == " ") or (payload == "-1") or (payload == NULL) or (payload == "ERROR")) {
                        httpRequestError = true;

                        lostServerConnection = true;

                        // Because there is nothing to parse
                        tHttpRequestParser.disable();
                        tParsedDataToRegularSlaveUnitVariables.disable();

                        if(enableDebugging) {
                                // A line break for more readibility
                                Serial.println(" ");

                                Serial.print("Server addres:");
                                Serial.println(serverAddreess);

                                Serial.print("HTTP Query:");
                                Serial.println(query);

                                Serial.print("HTTP Response: ");
                                Serial.println(payload);

                                // A line break for more readibility
                                Serial.println(" ");
                        }

                        // A line break for more readibility
                        Serial.print("Millis(): ");
                        Serial.println(millis());
                        Serial.println("////////////////////////////////");
                        Serial.println("//   LOST SERVER CONNECTION   //");
                        Serial.println("////////////////////////////////");
                        Serial.println(" ");

                } else{

                        if(enableDebugging) {
                                // A line break for more readibility
                                Serial.println(" ");

                                Serial.print("Server addres:");
                                Serial.println(serverAddreess);

                                Serial.print("HTTP Query:");
                                Serial.println(query);

                                Serial.print("HTTP Response: ");
                                Serial.println(payload);

                                // A line break for more readibility
                                Serial.println(" ");
                        }

                        httpRequestError = false;


                        tHttpRequestParser.enable();
                        tParsedDataToRegularSlaveUnitVariables.enable();


                        if ((sensorDataTransferRequest) and (enableDebugging)) {
                                Serial.println("Sent the sensor value to the server!");

                                // A line break for more readibility
                                Serial.println(" ");
                        }

                        // Reverting back to the original value
                        sensorDataTransferRequest = false;

                        if(lostServerConnection == true) {

                                lostServerConnection = false;

                                // A line break for more readibility
                                Serial.println(" ");
                                Serial.print("Millis(): ");
                                Serial.println(millis());
                                Serial.println("////////////////////////////////");
                                Serial.println("// RESTORED SERVER CONNECTION //");
                                Serial.println("////////////////////////////////");
                                Serial.println(" ");
                        }

                }

        } else {
                // A line break for more readibility
                Serial.println(" ");
                Serial.print("Millis(): ");
                Serial.println(millis());
                Serial.println("////////////////////////////////");
                Serial.println("//    LOST WIFI CONNECTION    //");
                Serial.println("////////////////////////////////");
                Serial.println(" ");

                lostServerConnection == true;

                // Because there is nothing to parse
                tHttpRequestParser.disable();
                tParsedDataToRegularSlaveUnitVariables.disable();
        }

}

// Converts the string (in JSON format) gathered by the http request to regular
//      arduino variables. So that they can be used in the rest of the code.
void httpRequestParserCallback() {

        // If there are no problems with the http result continue, otherwise keep the
        // old variables
        if (httpRequestError == false) {

                // Parsing
                const size_t bufferSize = JSON_OBJECT_SIZE(9) + 200;
                DynamicJsonBuffer jsonBuffer(bufferSize);

                const char* json = "{\"deviceIsEnabled\":1,\"runOnlyOnce\":0,\"powerPercent\":100,\"dataUpdateInterval\":3,\"enableDeepSleep\":0,\"enableWifiOffWhenNotUsed\":0,\"serverCurrentUnixTime\":1533904819,\"onDuration\":5,\"offDuration\":10}";

                JsonObject& root = jsonBuffer.parseObject(payload);

                deviceIsEnabled = root["deviceIsEnabled"]; // 0
                runOnlyOnce = root["runOnlyOnce"]; // 0
                powerPercent = root["powerPercent"]; // 100
                serverDataUpdateInterval = root["dataUpdateInterval"]; // 2
                serverEnableDeepSleep = root["enableDeepSleep"]; // 0
                serverEnableWifiOffWhenNotUsed = root["enableWifiOffWhenNotUsed"]; // 0
                serverCurrentUnixTime = root["serverCurrentUnixTime"];                 // 1533838233

                // Check if they are -1, they should be -1 if the device is a light
                if ((root["onDuration"] > 0 ) and (root["offDuration"] > 0 )) {

                        onDuration = root["onDuration"];

                        // convert seconds to milliseconds for millis()
                        offDuration = root["offDuration"];

                } else {

                        // If there is a problem, revert back to default values

                        onDuration = 15;

                        offDuration = 60;

                }


                // For debugging
                if(enableDebugging) {

                        // A line break for more readibility
                        Serial.println(" ");

                        Serial.print("Parsed variables from HTTP response:");
                        Serial.println(" ");

                        // Output to serial monitor
                        Serial.print("deviceIsEnabled:");
                        Serial.println(deviceIsEnabled);

                        Serial.print("runOnlyOnce:");
                        Serial.println(runOnlyOnce);

                        Serial.print("powerPercent:");
                        Serial.println(powerPercent);

                        Serial.print("onDuration (seconds):");
                        Serial.println(onDuration);

                        Serial.print("offDuration (seconds):");
                        Serial.println(offDuration);

                        Serial.print("serverDataUpdateInterval (seconds):");
                        Serial.println(serverDataUpdateInterval);

                        Serial.print("serverEnableDeepSleep:");
                        Serial.println(serverEnableDeepSleep);

                        Serial.print("serverEnableWifiOffWhenNotUsed:");
                        Serial.println(serverEnableWifiOffWhenNotUsed);

                        Serial.print("serverCurrentUnixTime:");
                        Serial.println(serverCurrentUnixTime);

                        // A line break for more readibility
                        Serial.println(" ");
                }

                tParsedDataToRegularSlaveUnitVariables.enable();
        } else {

                if(enableDebugging) {
                        // A line break for more readibility
                        Serial.println(" ");

                        Serial.println("Didn't parsed variables");

                        // A line break for more readibility
                        Serial.println(" ");
                }

                tParsedDataToRegularSlaveUnitVariables.disable();
        }
}

// Does the conversation between the variables generated with the JSON parsing
//      function parsedDataToRegularSlaveUnitVariablesCallback().
// Convert the data types, checks the min and max values of the variables.
// Basically, it does the housekeeping
void parsedDataToRegularSlaveUnitVariablesCallback() {

        // If there are no problems with the http result continue, otherwise keep the
        // old variables
        if (httpRequestError == false) {

                ///////////////////////////////////////////////////////
                //////////// VARIABLE CONVERSATION - BEGIN ////////////
                ///////////////////////////////////////////////////////

                // Value mapping to PWM value for the power percent value
                transistorPinPwmValue = map(powerPercent, 0, 100, 0, 1023);

                // if the new value is same, don't update the value
                if((not (dataUpdateInterval == serverDataUpdateInterval*1000)) and (serverDataUpdateInterval > 0 )) {
                        // convert seconds to milliseconds for millis()
                        dataUpdateInterval = serverDataUpdateInterval*1000;

                        updateDataUpdateIntervals = true;

                        // bad practice but needed to make it work more stable
                        delay(500);

                } else {

                        // In order to prevent any mistakes in the database to cause problems
                        // on the slave unit
                        if(serverDataUpdateInterval == 0 ) {

                                // A line break for more readibility
                                Serial.println(" ");
                                Serial.print("Millis(): ");
                                Serial.println(millis());
                                Serial.println("//////////////////////////////////");
                                Serial.println("// PLEASE CHECK THE DATA UPDATE //");
                                Serial.println("//    INTERVAL VALUE FOR THIS   //");
                                Serial.println("//      SPECIFIC SLAVE UNIT     //");
                                Serial.println("//////////////////////////////////");
                                Serial.println(" ");
                        }

                        updateDataUpdateIntervals = false;

                }

                // convert seconds to milliseconds for millis()
                onDuration = (onDuration)*1000;

                // convert seconds to milliseconds for millis()
                offDuration = (offDuration)*1000;


                // Convert to boolean variable
                if(serverEnableDeepSleep == 1) {
                        enableDeepSleep = true;
                } else if(serverEnableDeepSleep == 0) {
                        enableDeepSleep = false;
                }

                // Convert to boolean variable
                if(serverEnableWifiOffWhenNotUsed == 1) {
                        enableWifiOffWhenNotUsed = true;
                } else if(serverEnableWifiOffWhenNotUsed == 0) {
                        enableWifiOffWhenNotUsed = false;
                }

                ///////////////////////////////////////////////////////
                ///////////// VARIABLE CONVERSATION - END /////////////
                ///////////////////////////////////////////////////////

                if(enableDebugging) {

                        Serial.println("Updated ALL VARIABLES successfully");
                }

                // For testing purposes
                //updateTransistorPin();

        } else {

                if(enableDebugging) {
                        // A line break for more readibility
                        Serial.println(" ");

                        Serial.println("Didn't update variables");

                        // A line break for more readibility
                        Serial.println(" ");
                }
        }

}

// Updates the task manager intervals when changed in the database.
// This function ensures that the intervals are changed properly without crashing
//      the task manager and the whole system.
void updateDataUpdateIntervalsCallback() {

        if(updateDataUpdateIntervals) {
                updateDataUpdateIntervals = false;

                // not the light sensor, the light device
                if ((deviceType == "light") or (deviceType == "LIGHT") or (deviceType == "Light")) {
                        tRunLight.disable();
                        tRunLight.setInterval(dataUpdateInterval);
                        tRunLight.enable();

                } else if ((deviceType.indexOf("Valve") > 0) or (deviceType.indexOf("VALVE") > 0) or (deviceType.indexOf("valve") > 0)) {
                        tRunValve.disable();
                        tRunValve.setInterval(dataUpdateInterval);
                        tRunValve.enable();

                } else if ((deviceType.indexOf("Pump") > 0) or (deviceType.indexOf("PUMP") > 0) or (deviceType.indexOf("pump") > 0)) {
                        tRunPump.disable();
                        tRunPump.setInterval(dataUpdateInterval);
                        tRunPump.enable();

                } else if ((deviceType.indexOf("Sensor") > 0) or (deviceType.indexOf("SENSOR") > 0) or (deviceType.indexOf("sensor") > 0)) {
                        // All sensors
                        tRunSensor.disable();
                        tRunSensor.setInterval(dataUpdateInterval);
                        tRunSensor.enable();
                }

                tHttpRequest.disable();
                tHttpRequestParser.disable();
                tParsedDataToRegularSlaveUnitVariables.disable();
                tUpdateDataUpdateIntervals.disable();

                tHttpRequest.setInterval(dataUpdateInterval);
                tHttpRequestParser.setInterval(dataUpdateInterval);
                tParsedDataToRegularSlaveUnitVariables.setInterval(dataUpdateInterval);
                tUpdateDataUpdateIntervals.setInterval(dataUpdateInterval);

                delay(500);

                tHttpRequest.enable();
                tHttpRequestParser.enable();
                tParsedDataToRegularSlaveUnitVariables.enable();
                tUpdateDataUpdateIntervals.enable();

                // A line break for more readibility
                Serial.println(" ");
                Serial.print("Millis(): ");
                Serial.println(millis());
                Serial.println("//////////////////////////////////");
                Serial.println("//            UPDATED           //");
                Serial.println("//          DATA UPDATE         //");
                Serial.println("//           VARIABLES          //");
                Serial.println("//////////////////////////////////");
                Serial.println(" ");
        }

}

// Turns the connected fogger(s) ON and OFF based on the onDuration and offDuration
//      gathered from the main server.
// Has a resolution of 1 seconds, which means the function
//      can change state only once per second. This resolution is more than
//      enough for fogger(s) to operate properly.
// Utilizes the updateTransistorPin() to turn on and off the fogger(s)
// Connected to the transistorPin
void runFoggerCallback() {

        unsigned long currentMillis = millis();

        if (currentMillis - previousMillis >= interval) {
                // Save the current time to compare "later"
                previousMillis = currentMillis;

                if (deviceState) {
                        // Fogger is currently on, set time to stay off
                        interval = offDuration;
                } else {
                        // LED is currently off, set time to stay on
                        interval = onDuration;
                }

                // Toggle the LED's state, Fancy, eh!?
                deviceState = !(deviceState);
        }

        updateTransistorPin();

}

// Dims tha lights based on the transistorPinPwmValue gathered from the main server.
// This function tries to dim lights smootly by gradually dimming.
// Utilizes the updateTransistorPin() to apply the PWM values to the connected light(s).
// Connected to the transistorPin
void runLightCallback() {

        deviceState = HIGH;

        // In order to prevent direct shut down for lights
        if (powerPercent == 0) {
                powerPercent = 1;
        }

        tempDeviceIsEnabled  = deviceIsEnabled;

        // In order to prevent direct shut down for lights
        if (deviceIsEnabled == 0) {
                deviceIsEnabled = 1;
                transistorPinPwmValue = 1;
        }

        // Slowly adjust the voltage
        if (previousTransistorPinPwmValue < transistorPinPwmValue) {

                int tempPwmValue = transistorPinPwmValue;

                for (int i = previousTransistorPinPwmValue; i <= tempPwmValue; i = i + 10) {

                        transistorPinPwmValue = i;

                        delay(30);

                        updateTransistorPin();
                }

        } else if (previousTransistorPinPwmValue > transistorPinPwmValue) {

                int tempPwmValue = transistorPinPwmValue;

                for (int i = previousTransistorPinPwmValue; i >= tempPwmValue; i = i - 10) {

                        transistorPinPwmValue = i;

                        delay(30);

                        updateTransistorPin();
                }


        } else if (previousTransistorPinPwmValue == transistorPinPwmValue) {

                // some case that will be implemented in the future
                //updateTransistorPin();

        }

        // To prevent current leaks
        if (transistorPinPwmValue > 999 ) {
                transistorPinPwmValue = 1023;
        } else if (transistorPinPwmValue < 26 ) {
                transistorPinPwmValue = 0;
                powerPercent = 0;
        }

        updateTransistorPin();

        deviceIsEnabled = tempDeviceIsEnabled;

        previousTransistorPinPwmValue = transistorPinPwmValue;

}

// Connected to the transistorPin
void runValveCallback() {

        if (deviceIsEnabled == 0) {
                deviceState = LOW;
                transistorPinPwmValue = 0;
        } else if (deviceIsEnabled == 1) {
                deviceState = HIGH;
        }

        updateTransistorPin();

}

// Connected to the transistorPin
void runPumpCallback() {

        if (deviceIsEnabled == 0) {
                deviceState = LOW;
                transistorPinPwmValue = 0;
        } else if (deviceIsEnabled == 1) {
                deviceState = HIGH;
        }

        updateTransistorPin();

}

// Selects and runs the right sensor reading function based on the deviceType.
// Triggers httpRequestCallback() to send the reading to the database.
void runSensorCallback() {

        if(enableDebugging) {

                // A line break for more readibility
                Serial.println(" ");

                Serial.println("Reading the sensor value...");

        }

        if((deviceType.indexOf("light") >= 0) or (deviceType.indexOf("LIGHT") >= 0) or (deviceType.indexOf("Light") >= 0)) {

                // sensorReading is a "String", <i> for you information </i>
                sensorReading = lightSensorReading();

                sensorDataTransferRequest = true;

        } else if((deviceType.indexOf("pressure") >= 0) or (deviceType.indexOf("PRESSURE") >= 0) or (deviceType.indexOf("Pressure") >= 0)) {

                // sensorReading is a "String", <i> for you information </i>
                sensorReading = pressureSensorReading();

                sensorDataTransferRequest = true;

        } else if((deviceType.indexOf("flow") >= 0) or (deviceType.indexOf("FLOW") >= 0) or (deviceType.indexOf("Flow") >= 0)) {

                // sensorReading is a "String", <i> for you information </i>
                sensorReading = flowSensorReading();

                sensorDataTransferRequest = true;

        } else if((deviceType.indexOf("humidity") >= 0) or (deviceType.indexOf("HUMIDITY") >= 0) or (deviceType.indexOf("Humidity") >= 0)) {

                // sensorReading is a "String", <i> for you information </i>
                sensorReading = humiditySensorReading();

                sensorDataTransferRequest = true;

        } else if((deviceType.indexOf("ph") >= 0) or (deviceType.indexOf("PH") >= 0) or (deviceType.indexOf("Ph") >= 0)) {

                // sensorReading is a "String", <i> for you information </i>
                sensorReading = phSensorReading();

                sensorDataTransferRequest = true;

        } else if((deviceType.indexOf("ec") >= 0) or (deviceType.indexOf("EC") >= 0) or (deviceType.indexOf("Ec") >= 0)) {

                // sensorReading is a "String", <i> for you information </i>
                sensorReading = ecSensorReading();

                sensorDataTransferRequest = true;

        } else {

                sensorDataTransferRequest = false;

                if(enableDebugging) {

                        // A line break for more readibility
                        Serial.println(" ");

                        Serial.println("I am not a sensor, how did I end up executing the sensor function?");

                        // A line break for more readibility
                        Serial.println(" ");
                }

        }

        if((enableDebugging) and (sensorDataTransferRequest))  {

                Serial.println("Now sending to the server.");

        }

}

// Conected to I2C
String lightSensorReading() {

        Adafruit_TSL2561_Unified tsl = Adafruit_TSL2561_Unified(TSL2561_ADDR_FLOAT, 12345);

        tsl.begin();

        tsl.enableAutoRange(true);

        tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_402MS);

        sensors_event_t event;

        tsl.getEvent(&event);

        // Convert the reading to string, because it can handle every kind of data,
        // even if the sensor is changed in the future
        String reading = String(event.light);

        return reading;
}

// Conected to I2C
String pressureSensorReading() {

        String reading = "";

        Adafruit_BMP085_Unified bmp = Adafruit_BMP085_Unified(10085);

        bmp.begin();

        sensors_event_t event;
        bmp.getEvent(&event);

        float temperature;
        bmp.getTemperature(&temperature);

        float seaLevelPressure = SENSORS_PRESSURE_SEALEVELHPA;

        // Pressure + Temperature + Altitude
        reading = String(event.pressure) + "_" + String(temperature) + "_" + bmp.pressureToAltitude(seaLevelPressure, event.pressure);

        return reading;
}

// THIS FUNCTION IS NOT READY
String flowSensorReading() {

        String reading = "";

        reading = "sik";


        return reading;
}

// Connected to oneWireInterfacePin
String humiditySensorReading() {

        String reading = "";

        #define DHTPIN  oneWireInterfacePin

        // Uncomment the type of sensor in use:
        #define DHTTYPE           DHT11     // DHT 11
        //#define DHTTYPE           DHT22     // DHT 22 (AM2302)
        //#define DHTTYPE           DHT21     // DHT 21 (AM2301)

        DHT_Unified dht(DHTPIN, DHTTYPE);

        // Initialize device.
        dht.begin();

        // Get temperature event and print its value.
        sensors_event_t event;
        dht.temperature().getEvent(&event);

        String temp = String(event.temperature);

        // Get humidity event and print its value.
        dht.humidity().getEvent(&event);

        String hum = String(event.relative_humidity);

        // Humidity + temperature
        reading = hum + "_" + temp;

        return reading;
}

// Connected to ADC to I2C converter
String phSensorReading() {

        int sensorPinOnADC = 0;

        String reading = "";

        // Based on https://www.dfrobot.com/wiki/index.php/PH_meter(SKU:_SEN0161)

        //Store the average value of the sensor feedback
        unsigned long int avgValue;
        float b;
        int buf[10],temp;

        //Get 10 sample value from the sensor for smooth the value
        for(int i=0; i<10; i++) {
                buf[i] = getI2cAdcConverterValue(sensorPinOnADC);
                delay(50);
        }

        //sort the analog from small to large
        for(int i = 0; i < 9; i++) {
                for(int j = i + 1; j < 10; j++) {
                        if(buf[i] > buf[j]) {
                                temp = buf[i];
                                buf[i] = buf[j];
                                buf[j] = temp;
                        }
                }
        }

        avgValue = 0;

        //take the average value of 6 center sample
        for(int i = 2; i < 8; i++) {
                avgValue += buf[i];
        }

        //convert the analog into millivolt
        float phValue = (float)avgValue * 5.0 / 1023.0 / 6.0;

        //convert the millivolt into pH value
        phValue = 3.5 * phValue;

        reading = String(phValue);

        return reading;
}

// Connected to ADC to I2C converter
String ecSensorReading() {

        int sensorPinOnADC = 0;
        int sensorPinOnADC2 = 1;

        String reading = "";

        // Based on https://www.dfrobot.com/wiki/index.php/Analog_EC_Meter_SKU:DFR0300

        //Store the average value of the sensor feedback
        unsigned long int avgValue;
        float b;
        int buf[10],temp;

        float temperature,ECcurrent;

        // Get the temperature of the liquid
        temperature = (dallasTemperatureSensor().toFloat());

        //Get 10 sample value from the sensor for smooth the value
        for(int i=0; i<10; i++) {
                buf[i] = getI2cAdcConverterValue(sensorPinOnADC);
                delay(50);
        }

        //sort the analog from small to large
        for(int i = 0; i < 9; i++) {
                for(int j = i + 1; j < 10; j++) {
                        if(buf[i] > buf[j]) {
                                temp = buf[i];
                                buf[i] = buf[j];
                                buf[j] = temp;
                        }
                }
        }

        avgValue = 0;

        //take the average value of 6 center sample
        for(int i = 2; i < 8; i++) {
                avgValue += buf[i];
        }

        //convert the analog into millivolt
        float averageVoltage = (float)avgValue * 5000 / 1023.0 / 6.0;

        //temperature compensation formula: fFinalResult(25^C) = fFinalResult(current)/(1.0+0.0185*(fTP-25.0));
        float TempCoefficient = 1.0 + 0.0185 * (temperature - 25.0);

        float CoefficientVolatge=(float)averageVoltage/TempCoefficient;

        if(CoefficientVolatge <= 448) {
                //1ms/cm<EC<=3ms/cm
                ECcurrent = 6.84 * CoefficientVolatge - 64.32;

        } else if(CoefficientVolatge <= 1457) {
                //3ms/cm<EC<=10ms/cm
                ECcurrent = 6.98 * CoefficientVolatge - 127;

        } else {
                //10ms/cm<EC<20ms/cm
                ECcurrent = 5.3 * CoefficientVolatge + 2278;
        }

        //convert us/cm to ms/cm
        ECcurrent /= 1000;

        reading = String(ECcurrent) + "_" + String(temperature);

        return reading;
}

// Connected to oneWireInterfacePin
String dallasTemperatureSensor() {

        String reading = "";

        // Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
        OneWire oneWire(oneWireInterfacePin);

        // Pass our oneWire reference to Dallas Temperature.
        DallasTemperature sensors(&oneWire);
        // Start up the library
        sensors.begin();

        // call sensors.requestTemperatures() to issue a global temperature
        // request to all devices on the bus
        sensors.requestTemperatures(); // Send the command to get temperatures

        // After we got the temperatures, we can print them here.
        // We use the function ByIndex, and as an example get the temperature from the first sensor only.

        reading = String(sensors.getTempCByIndex(0));

        return reading;

}

// Reads the value ADC value at the given pin and converts to the 0 - 1023 range
//      for the compability with current libraries that are based on the arduino
//      analog input, which gives values between 0 and 1023 instead of 0 and 32767.
int getI2cAdcConverterValue(int x){

        Adafruit_ADS1115 ads; /* Use this for the 16-bit version */
        //Adafruit_ADS1015 ads;     /* Use thi for the 12-bit version */

        ads.begin();

        int16_t adc;

        adc = ads.readADC_SingleEnded(x);

        // Converting the reading to unsigned 10 bit value
        adc = map(adc, -27100, 27100, -1023, 1023);

        //    adc = map(adc, -1023, 1023, 0, 1023);

        // Making sure that values are in the bounds
        if(adc < 0) {
                adc = 0;
        }

        if(adc > 1023) {
                adc = 1023;
        }

        /*
           float voltage = adc * (5.0 / 1023.0);
           // write the voltage value to the serial monitor:
           Serial.print("voltage: ");
           Serial.println(voltage);
         */

        return adc;
}

// Turns ON or OFF the transistorPin based on the transistorPinPwmValue variable
void updateTransistorPin() {

        // Simply, if the device shouldn't be turned off, turn off
        if((deviceIsEnabled == 0) or (powerPercent == 0) or (deviceState == LOW)) {

                digitalWrite(transistorPin, LOW);

                if(enableDebugging) {
                        // A line break for more readibility
                        Serial.println(" ");

                        Serial.println("Turned off the device because device is not enabled or power percent is 0");

                        // A line break for more readibility
                        Serial.println(" ");
                }

                // Simply, if the device should be turned on, turn on
        } else if ((deviceIsEnabled == 1) and (powerPercent != 0) and (deviceState == HIGH)) {

                analogWrite(transistorPin, transistorPinPwmValue);

                if(enableDebugging) {
                        // A line break for more readibility
                        Serial.println(" ");

                        Serial.print("Turned on the device. PWM value is ");
                        Serial.println(transistorPinPwmValue);

                        // A line break for more readibility
                        Serial.println(" ");
                }

        }

}

void loop() {

        ArduinoOTA.handle();

        // Run task manager tasks
        taskManager.execute();

}
