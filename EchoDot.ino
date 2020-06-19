#include <Arduino.h>
#include <WiFi.h>
// Johnny 2020 - Compile with HELTEC ESP  2020 Libraries rather than feather


// Rename the credentials.sample.h file to credentials.h and 
// edit it according to your router configuration
#include "credentials.h"

#define _FAUXMO

#ifdef _FAUXMO
#include "fauxmoESP.h"
fauxmoESP fauxmo;
#endif

// -----------------------------------------------------------------------------

#define SERIAL_BAUDRATE     115200
// ONBOARD LED on PIN 13 on ADAFRUIT HUZZAH32 FEATHER
// ONBOARD LED on PIN 2 on doit.am ESP32

#define _ESP_HUZZAH32
//#define _ESP_DOIT

#ifdef _ESP_HUZZAH32
#define LED_YELLOW          13
#define LED_GREEN           5
#define LED_BLUE            0
#define LED_PINK            2
#define LED_WHITE           15
#endif 

#ifdef _ESP_DOIT
#define LED_YELLOW          2
#define LED_GREEN           5
#define LED_BLUE            0
#define LED_PINK            13
#define LED_WHITE           15
#endif 


typedef struct _devicePair {
  String deviceName;
  int pin ;
  bool active_high ;
} DEVICE ;

#define ID_YELLOW           "ELLEEDEE"
#define ID_GREEN            "zappy"
#define ID_BLUE             "burpy"
#define ID_PINK             "slurpy"
#define ID_WHITE            "frumpy"


#define ACTIVE_HIGH          false

DEVICE devices[] = {
              {ID_GREEN, LED_GREEN, ACTIVE_HIGH},
              {ID_BLUE, LED_BLUE, ACTIVE_HIGH},
              {ID_PINK, LED_PINK, ACTIVE_HIGH},
              {ID_YELLOW, LED_YELLOW, !ACTIVE_HIGH},
              {ID_WHITE,LED_WHITE, ACTIVE_HIGH}
             } ;

#define OFF_STATE HIGH 
#define ON_STATE LOW 

// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Wifi
// -----------------------------------------------------------------------------

void wifiSetup() {

    // Set WIFI module to STA mode
    WiFi.mode(WIFI_STA);

    // Connect
    Serial.printf("[WIFI] Connecting to %s ", WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    // Wait
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(100);
    }
    Serial.println();

    // Connected!
    Serial.printf("[WIFI] STATION Mode, SSID: %s, IP address: %s\n", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());

}


void initDevices() {
  for(int idx = 0 ; idx < sizeof(devices)/sizeof(_devicePair) ; idx++) {
    int pin = devices[idx].pin ;
    pinMode(pin, OUTPUT);
    digitalWrite(pin, !devices[idx].active_high);
    Serial.printf("Init Device %s, Pin: %d\n", devices[idx].deviceName, pin);

  }
}

bool searchAndSetDevice(const char *deviceName,bool state, unsigned char value) {
    for(int idx = 0 ; idx < sizeof(devices)/sizeof(_devicePair) ; idx++) {

      if (devices[idx].deviceName.equals(deviceName)) {
        digitalWrite(devices[idx].pin, state ? devices[idx].active_high : !devices[idx].active_high);
        return true ;
      }
    }
    return false ;
}

void setup() {

    // Init serial port and clean garbage
    Serial.begin(SERIAL_BAUDRATE);
    Serial.println();
    Serial.println();

    
    // Wifi
    wifiSetup();
#ifdef _FAUXMO
  initDevices() ;
  fauxmoSetup() ;
#endif 
}

#ifdef _FAUXMO

void fauxmoSetup() {
  
    // By default, fauxmoESP creates it's own webserver on the defined port
    // The TCP port must be 80 for gen3 devices (default is 1901)
    // This has to be done before the call to enable()
    fauxmo.createServer(true); // not needed, this is the default value
    fauxmo.setPort(80); // This is required for gen3 devices

    // You have to call enable(true) once you have a WiFi connection
    // You can enable or disable the library at any moment
    // Disabling it will prevent the devices from being discovered and switched
    fauxmo.enable(true);

    // You can use different ways to invoke alexa to modify the devices state:
    // "Alexa, turn yellow lamp on"
    // "Alexa, turn on yellow lamp
    // "Alexa, set yellow lamp to fifty" (50 means 50% of brightness, note, this example does not use this functionality)

    // Add virtual devices
    for(int idx = 0 ; idx < sizeof(devices)/sizeof(_devicePair) ; idx++) {
        fauxmo.addDevice(devices[idx].deviceName.c_str());
    }

    fauxmo.onSetState([](unsigned char device_id, const char * device_name, bool state, unsigned char value) {
        
        // Callback when a command from Alexa is received. 
        // You can use device_id or device_name to choose the element to perform an action onto (relay, LED,...)
        // State is a boolean (ON/OFF) and value a number from 0 to 255 (if you say "set kitchen light to 50%" you will receive a 128 here).
        // Just remember not to delay too much here, this is a callback, exit as soon as possible.
        // If you have to do something more involved here set a flag and process it in your main loop.
        
        Serial.printf("!!!![MAIN] Device #%d (%s) state: %s value: %d\n", device_id, device_name, state ? "ON" : "OFF", value);

        // Checking for device_id is simpler if you are certain about the order they are loaded and it does not change.
        // Otherwise comparing the device_name is safer.
        searchAndSetDevice(device_name, state, value) ;
        

    });

}
#endif

void loop() {

#ifdef _FAUXMO
    // fauxmoESP uses an async TCP server but a sync UDP server
    // Therefore, we have to manually poll for UDP packets
    fauxmo.handle();
#endif

    // This is a sample code to output free heap every 5 seconds
    // This is a cheap way to detect memory leaks
    static unsigned long last = millis();
    if (millis() - last > 5000) {
        last = millis();
        Serial.printf("[MAIN] Free heap: %d bytes\n", ESP.getFreeHeap());
    }

    // If your device state is changed by any other means (MQTT, physical button,...)
    // you can instruct the library to report the new state to Alexa on next request:
    // fauxmo.setState(ID_YELLOW, true, 255);

}
