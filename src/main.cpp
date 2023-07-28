// Uncomment the following line to enable serial debug output
//#define ENABLE_DEBUG

#ifdef ENABLE_DEBUG
#define DEBUG_ESP_PORT Serial
#define NODEBUG_WEBSOCKETS
#define NDEBUG
#endif

#include <Arduino.h>
#ifdef ESP8266
#include <ESP8266WiFi.h>
#endif
#ifdef ESP32
#include <WiFi.h>
#endif

#include "SinricPro.h"
#include "SinricProSwitch.h"
#include "Stepper.h"
#include <AsyncElegantOTA.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>

#define WIFI_SSID  "WifISSID"
#define WIFI_PASS  "password"
#define APP_KEY    "04XXXX-XXXX-XXXX-XXXX-XXXXXXXXXX"     
#define APP_SECRET "XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXX-XXXXXXX-XXXX-XXXX-XXXX-XXXXXXX"  
#define SWITCH_ID  "62XXXXXXXXXXXX"   
#define BAUD_RATE  9600           


const int stepsPerRevolution = 200;
;

AsyncWebServer server(80);


Stepper          myStepper(stepsPerRevolution, D3, D4);//SETUP THE DIGITAL PINS
SinricProSwitch &mySwitch = SinricPro[SWITCH_ID];  // create new switch device
int              stepsToDo = 0;
bool             motor_is_running = false;



bool onPowerState(const String &deviceId, bool &state) {
    Serial.printf("Device %s power turned %s \r\n", deviceId.c_str(), state?"on":"off");
    if (motor_is_running) 
        return true;  // if the motor is running, ignore the command

    if (state) {              // if device is turned on
        stepsToDo = 9800;   // make 9800 steps in positive direction
    } else {           // if device is turned off
        stepsToDo = -9500;  // make 9800 steps in negative direction 
    }
    return true;  // request handled properly
}

void handleStepper() {
    if (stepsToDo == 0) return;  // return if nothing is to do...

    if (stepsToDo > 0) {     // if we have to do steps in positive direction
        myStepper.step(1);   // make a single step in positive direction
        stepsToDo--;         // reduce the remaining steps by one
    } else {                 // if we have to do steps in negative direction
        myStepper.step(-1);  // make a single step in negative direction
        stepsToDo++;         // reduce the remaining steps (to reduce a negative number we have to add)
    }

    if (stepsToDo == 0) motor_is_running = false;  // if motion is completed set motor_is_busy to false;
}

void setupWiFi() {
    WiFi.begin(WIFI_SSID, WIFI_PASS);  // start wifi
    Serial.printf("Connecting to WiFi \"%s\"", WIFI_SSID);
    while (WiFi.status() != WL_CONNECTED) {
        Serial.printf(".");
        delay(100);
    }
    //Serial.printf("connected\r\n");
    IPAddress localIP = WiFi.localIP();
    Serial.printf("connected!\r\n[WiFi]: IP-Address is %d.%d.%d.%d\r\n", localIP[0], localIP[1], localIP[2], localIP[3]);
}

void setupSinricPro() {
    mySwitch.onPowerState(onPowerState);   // apply onPowerState callback
    SinricPro.begin(APP_KEY, APP_SECRET);  // start SinricPro
    SinricPro.onConnected([](){ Serial.printf("Connected to SinricPro\r\n"); }); 
    SinricPro.onDisconnected([](){ Serial.printf("Disconnected from SinricPro\r\n"); });
}

void setupStepper() {
    // set the speed at 500 rpm:
    myStepper.setSpeed(500);
}



void setup() {
    Serial.begin(BAUD_RATE);
    setupWiFi();
    setupSinricPro();
    setupStepper();
    AsyncElegantOTA.begin(&server);
    server.begin();

}

void loop() {
    SinricPro.handle();  // handle SinricPro commands
    handleStepper();     // handle Stepper motor
}

