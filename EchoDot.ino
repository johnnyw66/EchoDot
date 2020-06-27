#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>

#include <WebServer.h>
#include <DNSServer.h>
#include <ESPmDNS.h>

// Johnny 2020 - Compile with HELTEC ESP  2020 Libraries rather than feather


// Rename the credentials.sample.h file to credentials.h and 
// edit it according to your router configuration
#include "credentials.h"
#include <Wire.h>

#include "Display.h"

// EEPROM SDA/SCL PINS (24C256L)

#define SDAPIN  23
#define SCLPIN  22 

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
#define LED_APPLIANCE_PIN        13
#define TFT_APPLIANCE_PIN        25
#define APPLIANCE1_PIN           32
#define APPLIANCE2_PIN           15
#define APPLIANCE3_PIN           33
#define APPLIANCE4_PIN           27
#define CONFIG_PIN               14

#define STATUS_PIN               26   // A0    
#define BACKLIGHT_PIN            25   // A1
#endif 

#ifdef _ESP_DOIT
#define LED_APPLIANCE_PIN         2
#define TFT_APPLIANCE_PIN        25
#define APPLIANCE1_PIN            5
#define APPLIANCE2_PIN            0
#define APPLIANCE3_PIN            13
#define APPLIANCE4_PIN            15
#define CONFIG_PIN                 1

#define STATUS_PIN               12  
#define BACKLIGHT_PIN            25   

#endif 


typedef struct _devicePair {
  String deviceName;
  int pin ;
  bool state ;
  bool active_high ;
} DEVICE ;

#define LED_APPLIANCE           "Debug"
#define TFT_APPLIANCE           "Config Panel"

// Set these to your desired credentials.
#define CONFIG_SSID "ESP-CONFIG"
#define CONFIG_PASSWORD "123456789"
#define CONFIG_HOSTNAME "esp32" 

#define DEFAULT_APPLIANCE1_NAME "APPLIANCE 1" 
#define DEFAULT_APPLIANCE2_NAME "APPLIANCE 2" 
#define DEFAULT_APPLIANCE3_NAME "APPLIANCE 3" 
#define DEFAULT_APPLIANCE4_NAME "APPLIANCE 4" 

#define ACTIVE_LOW          false
#define ACTIVE_HIGH          true

DEVICE devices[] = {
              {DEFAULT_APPLIANCE1_NAME, APPLIANCE1_PIN, false, ACTIVE_LOW},
              {DEFAULT_APPLIANCE2_NAME, APPLIANCE2_PIN, false, ACTIVE_LOW},
              {DEFAULT_APPLIANCE3_NAME, APPLIANCE3_PIN, false, ACTIVE_LOW},
              {DEFAULT_APPLIANCE4_NAME, APPLIANCE4_PIN, false, ACTIVE_LOW},
              {LED_APPLIANCE, LED_APPLIANCE_PIN, false, ACTIVE_HIGH},
              {TFT_APPLIANCE, TFT_APPLIANCE_PIN, true, ACTIVE_HIGH}
             } ;

#define OFF_STATE HIGH 
#define ON_STATE LOW 

enum runmode {FAUXMOMODE, CONFIGMODE }  RUNMODE ;
enum runmode mode = FAUXMOMODE ;


// Config Stuff

const char *configSSID = CONFIG_SSID ;
const char *configPassword = CONFIG_PASSWORD ;
const char *configHostname = CONFIG_HOSTNAME ;

WebServer server(80);


const byte DNS_PORT = 53;
DNSServer dnsServer;

#define eeprom1 0x50    //Address of 24LC256 eeprom chip

#define EEPROM_SSID_ADDRESS 0 
#define EEPROM_PASSWORD_ADDRESS 64

#define EEPROM_APPLIANCE1_ADDRESS 128
#define EEPROM_APPLIANCE2_ADDRESS 192
#define EEPROM_APPLIANCE3_ADDRESS 256
#define EEPROM_APPLIANCE4_ADDRESS 320

#define EEPROM_DISPMODE_ADDRESS   384 


#define MAXSTRLEN 30

char e_ssid[MAXSTRLEN];
char e_password[MAXSTRLEN];

char e_appliance1[MAXSTRLEN];
char e_appliance2[MAXSTRLEN];
char e_appliance3[MAXSTRLEN];
char e_appliance4[MAXSTRLEN];

char *appliancenames[] = {
    e_appliance1,
    e_appliance2,
    e_appliance3,
    e_appliance4
} ;

int mode_indicator = LOW ;

void setup() {
  // CONFIG PIN
    pinMode(CONFIG_PIN, INPUT_PULLUP);
    pinMode(STATUS_PIN, OUTPUT);
    digitalWrite(STATUS_PIN,mode_indicator) ;
    
    // Init serial port and clean garbage

    Serial.begin(SERIAL_BAUDRATE);
    Serial.println();
    Serial.println();
    
    setupEEPROMConfig() ;
    // Update Appliance Names
    Serial.println("Copying over appliance names stored in EEPROM....") ;
    for(int i = 0 ; i < 4 ; i++) {
      devices[i].deviceName = String(getApplianceName(i)) ;
    }

    initDisplay(DEFAULT_ROTATION,BACKLIGHT_PIN) ;
        
    if (digitalRead(CONFIG_PIN) == 0) {
      Serial.println("CONFIG MODE") ;
      mode = CONFIGMODE ;  
      setupConfigMode() ;
    } else {
      Serial.println("FAUXMO MODE") ;
      mode = FAUXMOMODE ;
      setupFauxmoMode() ;
    }

   testText() ; 
   delay(3000) ;

}



bool dirty = true ;

// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Wifi
// -----------------------------------------------------------------------------

void wifiSetup() {

    // Set WIFI module to STA mode
    WiFi.mode(WIFI_STA);

    // Connect
    Serial.printf("[WIFI] Connecting to %s ", getWiFiSSID());
    WiFi.begin(getWiFiSSID(), getWiFiPassword());

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
  Serial.println("initDevices") ;
  for(int idx = 0 ; idx < sizeof(devices)/sizeof(_devicePair) ; idx++) {
    int pin = devices[idx].pin ;
    pinMode(pin, OUTPUT);
    //digitalWrite(pin, !devices[idx].active_high);
    digitalWrite(pin, devices[idx].state ? devices[idx].active_high : !devices[idx].active_high);
    Serial.printf("Init Device %s, Pin: %d\n", devices[idx].deviceName.c_str(), pin);

  }


   

}

bool searchAndSetDevice(const char *deviceName,bool state, unsigned char value) {
    for(int idx = 0 ; idx < sizeof(devices)/sizeof(_devicePair) ; idx++) {

      if (devices[idx].deviceName.equals(deviceName)) {
        devices[idx].state = state ;
        digitalWrite(devices[idx].pin, state ? devices[idx].active_high : !devices[idx].active_high);
        return true ;
      }
    }
    return false ;
}



void setupFauxmoMode() {
    // Wifi
    wifiSetup() ;
    
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
        dirty = true ;
        // Checking for device_id is simpler if you are certain about the order they are loaded and it does not change.
        // Otherwise comparing the device_name is safer.
        searchAndSetDevice(device_name, state, value) ;
        

    });

}
#endif


void loop() {

  if (mode == FAUXMOMODE) {
    fauxmoLoop() ;  
  } else {
    configLoop() ;
  }

}


void fauxmoLoop() {

#ifdef _FAUXMO
    // fauxmoESP uses an async TCP server but a sync UDP server
    // Therefore, we have to manually poll for UDP packets
    fauxmo.handle();
#endif

    // This is a sample code to output free heap every 5 seconds
    // This is a cheap way to detect memory leaks
    static unsigned long last = millis();
    if (millis() - last > 5000 || dirty) {
        last = millis();
        Serial.printf("[MAIN] Free heap: %d bytes\n", ESP.getFreeHeap());
        mode_indicator = ~mode_indicator ;
        digitalWrite(STATUS_PIN, mode_indicator) ;

        if (dirty) {
            displayDeviceStatus() ;
            dirty = false ;
        }
    }

    // If your device state is changed by any other means (MQTT, physical button,...)
    // you can instruct the library to report the new state to Alexa on next request:
    // fauxmo.setState(ID_YELLOW, true, 255);

}

void displayDeviceStatus() {
  char sbuff[64] ;
  
  displayClear() ;

  displayMessage(" *** DEVICE STATUS ***", 4, 10) ;    
  
  
  for(int idx = 0 ; idx < sizeof(devices)/sizeof(_devicePair) ; idx++) {
    bool state = devices[idx].state  ;
    sprintf(sbuff,"%s",(char *)devices[idx].deviceName.c_str()) ;
    displayMessage(sbuff, 4, 10 + 40 + idx * 24) ;    
    sprintf(sbuff,"%s",(devices[idx].state ? "ON" : "OFF")) ;
    displayMessage(sbuff, 192 + 4, 10 + 40 + idx * 24) ;    
  }
  
}


// Config Code From here onwards
String buildString(const String baseStr,...) {
  va_list ap;
  va_start(ap, baseStr);

  String s = String(baseStr) ;
  int argc = 1 ;
  char *rep ;
  
  while((rep  = va_arg(ap, char*)) != 0) {
      char sBuf[8] ;
      sprintf(sBuf,"$%d",argc++) ; 
      s.replace(sBuf,rep) ;

  }

  va_end(ap);

  return s ;
    
}

char *getWiFiSSID() {
  return e_ssid ;
}

char *getWiFiPassword() {
  return e_password ;
}

char *getApplianceName(int index) {
  return appliancenames[index] ;
}

void retrieveEEPROMInt(unsigned int eeaddress, int *val) {
  
}

void retrieveEEPROMString(unsigned int eeaddress, char *appname, int maxstrlen) {
  
  Serial.print("String @(") ;
  Serial.print(eeaddress) ;
  Serial.print(") ") ;
    
  String s = eepromReadCredentialsString(eeaddress,appname,maxstrlen) ;
  
  if (s == 0) {
    Serial.println("** NULL**") ;
  } else {
    Serial.println(s) ;
  }
  
}

void setupConfigMode() {

  setupConfigServer() ; 

}

int backLightValue ;

void setupEEPROMConfig()
{
  
  setupWire() ;  

  Serial.println("Reading EEPROM Credentials") ;
  retrieveEEPROMString(EEPROM_SSID_ADDRESS,e_ssid,MAXSTRLEN) ;
  retrieveEEPROMString(EEPROM_PASSWORD_ADDRESS,e_password,MAXSTRLEN) ;
  
  retrieveEEPROMString(EEPROM_APPLIANCE1_ADDRESS,e_appliance1,MAXSTRLEN) ;
  retrieveEEPROMString(EEPROM_APPLIANCE2_ADDRESS,e_appliance2,MAXSTRLEN) ;
  retrieveEEPROMString(EEPROM_APPLIANCE3_ADDRESS,e_appliance3,MAXSTRLEN) ;
  retrieveEEPROMString(EEPROM_APPLIANCE4_ADDRESS,e_appliance4,MAXSTRLEN) ;

  retrieveEEPROMInt(EEPROM_DISPMODE_ADDRESS,&backLightValue) ;

  Serial.println("Credential and Appliances EE Reading Completed") ;
  Serial.println(backLightValue) ;
}


void resetEEPROM() {
  setDefaultRouterWiFiCredentialsEEPROM() ;
  setDefaultApplianceNamesEEPROM() ;
}

void setDefaultRouterWiFiCredentialsEEPROM() {
  updateEEPROMString(e_ssid,"YOUR ROUTER SSID", EEPROM_SSID_ADDRESS) ;
  updateEEPROMString(e_password,"YOUR ROUTER PASSWORD", EEPROM_PASSWORD_ADDRESS) ;

}
void setDefaultApplianceNamesEEPROM() {

  updateEEPROMString(e_appliance1, DEFAULT_APPLIANCE1_NAME, EEPROM_APPLIANCE1_ADDRESS) ;
  updateEEPROMString(e_appliance2, DEFAULT_APPLIANCE2_NAME, EEPROM_APPLIANCE2_ADDRESS) ;
  updateEEPROMString(e_appliance3, DEFAULT_APPLIANCE3_NAME, EEPROM_APPLIANCE3_ADDRESS) ;
  updateEEPROMString(e_appliance4, DEFAULT_APPLIANCE4_NAME, EEPROM_APPLIANCE4_ADDRESS) ;

}

void setupWire() {
  Wire.begin(SDAPIN,SCLPIN);  
  Serial.print("SDA ") ;
  Serial.println(SDAPIN) ;
  Serial.print("SCL ") ;
  Serial.println(SCLPIN) ;
}




/** Is this an IP? */
boolean isIp(String str) {
  for (size_t i = 0; i < str.length(); i++) {
    int c = str.charAt(i);
    if (c != '.' && (c < '0' || c > '9')) {
      return false;
    }
  }
  return true;
}

/** IP to String? */
String toStringIp(IPAddress ip) {
  String res = "";
  for (int i = 0; i < 3; i++) {
    res += String((ip >> (8 * i)) & 0xFF) + ".";
  }
  res += String(((ip >> 8 * 3)) & 0xFF);
  return res;
}


bool eepromSaveString(String s, unsigned int eeaddress) {
  char eeBuffer[33] ;

  Serial.print("EEPROM: Saving String:");
  Serial.print(s) ;
  Serial.print(" at address ") ;
  Serial.println(eeaddress) ;
  s.toCharArray((char *)eeBuffer,s.length() + 1) ;
  eeBuffer[s.length()] = 0 ;
  
  writeEEPROM(eeprom1, eeaddress,eeBuffer) ;
  return true ;

}


String eepromReadCredentialsString(unsigned int eeaddress, char *buff, int maxsize) {
  readEEPROM(eeprom1,  eeaddress,  
                 buff, maxsize) ; 
  return buff ;
}

void setupConfigServer() {

  Serial.println("Configuring access point...");

  WiFi.softAP(configSSID,configPassword);         
      
  IPAddress apIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(apIP);

  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.start(DNS_PORT, "*", apIP);
  
  server.on("/", HTTP_GET, handleRootGet);
  server.on("/wifi", HTTP_GET, handleRootGet);

  server.on("/", HTTP_POST, handleRootPost);
  
  server.on("/appliances", HTTP_GET, handleApplianceGet);
  server.on("/appliances", HTTP_POST, handleAppliancePost);
  
  server.on("/reset", HTTP_GET, handleResetGet);

  server.on("/status", HTTP_GET, handleStatusGet);
  server.on("/status", HTTP_POST, handleStatusGet);

  server.onNotFound(handleNotFound);
  
  server.begin();
  
  if(!MDNS.begin(configHostname)) {
     Serial.println("Error starting mDNS");
     return;
  } else {
        MDNS.addService("http", "tcp", 80);
  }

  Serial.println("Server started");
}

void updateEEPROMString( char *buff, String value, unsigned int eeaddress) {
    strcpy(buff, value.c_str()) ;
    eepromSaveString(value, eeaddress) ;

}
String links = F("<p><a href='/wifi'>Configure the WiFi connection</a></p>"
             "<p><a href='/appliances'>Set up appliance names</a></p>"
             "<p><a href='/status'>Status</a></p>"
             ) ;

String endBody = F("</body></html>");

String header = F("<html><head></head><body>") ;

/** Redirect to captive portal if we got a request for another domain. Return true in that case so the page handler do not try to handle the request again. */
boolean captivePortal() {
  if (!isIp(server.hostHeader()) && server.hostHeader() != (String(configHostname) + ".local")) {
    Serial.println("Request redirected to captive portal");
    server.sendHeader("Location", String("http://") + toStringIp(server.client().localIP()), true);
    server.send(302, "text/plain", "");   // Empty content inhibits Content-length header so we have to close the socket ourselves.
    server.client().stop(); // Stop is needed because we sent no content length
    return true;
  }
  return false;
}


void handleNotFound() {
  captivePortal() ;  
}

String credentialsForm = "<form action=\"/\" method=\"POST\">"
                          "<input type=\"text\" name=\"ssid\" placeholder=\"SSID\"  value=\"$1\"></br>"
                          "<input type=\"password\" name=\"password\" placeholder=\"Password\"  value=\"$2\"></br>"
                          "<input type=\"submit\" value=\"SetUp\">"
                          "</form><p>Please input Router WiFi Credentials</p>";
                          
String appliancesForm = "<form action=\"/appliances\" method=\"POST\">"
                    "<input type=\"text\" name=\"appliance1\" placeholder=\"appliance 1\" value=\"$1\"></br>"
                    "<input type=\"text\" name=\"appliance2\" placeholder=\"appliance 2\" value=\"$2\"></br>"
                    "<input type=\"text\" name=\"appliance3\" placeholder=\"appliance 3\" value=\"$3\"></br>"
                    "<input type=\"text\" name=\"appliance4\" placeholder=\"appliance 4\" value=\"$4\"></br>"
                    "<input type=\"submit\" value=\"SetUp\">"
                    "</form><p>Appliance names used by your Echo Dot</p>";

void setPageHeader() {
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");

}

void handleResetGet() {
    resetEEPROM() ;
    setPageHeader() ;
    String page ;
    page += header ;
    page += F("<h1>ResetEEPROM Completed</h1>");
 
    page += links ;
    page += endBody ;
            
   server.send(200, "text/html",page) ;
   

}




void handleAppliancePost() {
  setPageHeader() ;
    
  if( ! server.hasArg("appliance1") || ! server.hasArg("appliance2") 
      ||  ! server.hasArg("appliance3") || ! server.hasArg("appliance4")
      || server.arg("appliance1") == NULL || server.arg("appliance2") == NULL
      || server.arg("appliance3") == NULL || server.arg("appliance4") == NULL
      ) { 
    server.send(400, "text/plain", "400: Invalid Request");         // The request is invalid, so send HTTP status 400
    return;
  }

  Serial.println("Saving....") ;
  
  updateEEPROMString(e_appliance1, server.arg("appliance1"), EEPROM_APPLIANCE1_ADDRESS) ;
  updateEEPROMString(e_appliance2, server.arg("appliance2"), EEPROM_APPLIANCE2_ADDRESS) ;
  updateEEPROMString(e_appliance3, server.arg("appliance3"), EEPROM_APPLIANCE3_ADDRESS) ;
  updateEEPROMString(e_appliance4, server.arg("appliance4"), EEPROM_APPLIANCE4_ADDRESS) ;
  
  String page ;
  page += header ;
  page += F("<h1>Appliances Set</h1>");
 
  page += links ;
  page += endBody ;

  server.send(200, "text/html",page) ;
}

void handleApplianceGet() {
   setPageHeader() ;
   String page;
   page += header ;
   page += F("<h1>Set Up Appliances</h1>");

   page += buildString(appliancesForm,e_appliance1,e_appliance2,e_appliance3,e_appliance4,NULL).c_str() ;
   page += links ;
   page += endBody;
   
   server.send(200, "text/html",page) ;
   

}


String settingsPage = "<p>WiFi : $1 </p>"
                      "<p>Password : $2 </p>"
                      "<p>Appliance 1 : $3 </p>"
                      "<p>Appliance 2 : $4 </p>"
                      "<p>Appliance 3 : $5 </p>"
                      "<p>Appliance 4 : $6 </p>" ;

String buildSettingsPage() {
    return buildString(settingsPage, e_ssid, e_password, e_appliance1, e_appliance2, e_appliance3, e_appliance4,NULL) ;
}

void handleRootPost() {
  setPageHeader() ;
  
  if( ! server.hasArg("ssid") || ! server.hasArg("password") 
      || server.arg("ssid") == NULL || server.arg("password") == NULL) { // If the POST request doesn't have username and password data
    server.send(400, "text/plain", "400: Invalid Request");         // The request is invalid, so send HTTP status 400
    return;
  }

  updateEEPROMString(e_ssid, server.arg("ssid"), EEPROM_SSID_ADDRESS) ;
  updateEEPROMString(e_password, server.arg("password"), EEPROM_PASSWORD_ADDRESS) ;


  
  String page ;
  page += header ;
  
  page += F("<h1>Router WiFi Credentials Set</h1>");

  page += buildSettingsPage() ;

  page += links ;
  page += endBody ;
            
  server.send(200, "text/html",page) ;

}



void handleStatusGet() {
  setPageHeader() ;
  
  String page ;

  page += header ;
  page += F("<h1>Status/Settings</h1>");

  page += buildSettingsPage() ;

  page += links ;
  page += endBody ;        
  server.send(200, "text/html",page) ;

}

void handleRootGet() {

  setPageHeader() ;

   String page;
   
   page += header ;
   page += F("<h1>Set Up WiFi Credentials</h1>");

   page += buildString(credentialsForm, e_ssid, e_password, NULL).c_str() ;
    
   page += links ;
   page += endBody ;
   
   server.send(200, "text/html",page) ;

}



void configLoop() {
  dnsServer.processNextRequest();
  server.handleClient() ;

  static unsigned long last = millis();
  if (millis() - last > 500 || dirty) {
        last = millis();
        //Serial.printf("[CONFIG MAIN LOOP] Free heap: %d bytes\n", ESP.getFreeHeap());
        mode_indicator = ~mode_indicator ;
        digitalWrite(STATUS_PIN, mode_indicator) ;
        if (dirty) {
          displayConfigStatus() ;
          dirty = false ;
        }
  }
}

void displayConfigStatus() {
  char sbuff[64] ;
  
  displayClear() ;

  displayMessage("CONFIG AP CREDENTIALS", 4, 10) ;    
  displayMessage("SSID: ", 4, 10 + 2*24) ;
  displayMessage((char *)configSSID, 4 + 128, 10 + 2*24) ;

  displayMessage("PASSWORD: ", 4, 10 + 3*24) ;
  displayMessage((char *)configPassword, 4 + 128, 10 + 3*24) ;   

  setTextSize(1) ;
  displayMessage("Connect to Config Access Point above", 4, 10 + 5*24 + 0*10) ;
  displayMessage("and open your browser.", 4, 10 + 5*24 + 1*10) ;

  displayMessage("set the mode link to 'normal' when", 4, 10 + 5*24 + 3*10) ;
  displayMessage("completed.", 4, 10 + 5*24 + 4*10) ;


}

// Using External EEPROM SDA on ESP32 goes to PIN 5 on 24C256, SCL on ESP32 goes to Pin 6 on 24C256
// PIN 7 on 24C256 is tied to GND
// 24C256 I2C address lines are held 'LOW' with internal PDs (making their address 0x50) 

void wireScanClient() {
  byte error, address;
  int nDevices;
  Serial.println("Scanning...");
  nDevices = 0;
  for(address = 1; address < 127; address++ ) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    if (error == 0) {
      Serial.print("I2C device found at address 0x");
      if (address<16) {
        Serial.print("0");
      }
      Serial.println(address,HEX);
      nDevices++;
    }
    else if (error==4) {
      Serial.print("Unknow error at address 0x");
      if (address<16) {
        Serial.print("0");
      }
      Serial.println(address,HEX);
    }    
  }
  if (nDevices == 0) {
    Serial.println("No I2C devices found\n");
  }
  else {
    Serial.println("done\n");
  }
  delay(5000);          
}


 

void clearEEPROMString(unsigned int eeaddress) {
     
    Wire.beginTransmission(eeprom1);
    Wire.write((int)((eeaddress) >> 8));   // MSB
    Wire.write((int)((eeaddress) & 0xFF)); // LSB
    Wire.write((byte)0);
    Wire.endTransmission();
    delay(10) ;
  
}

// 
void writeEEPROM(int deviceaddress, unsigned int eeaddress,  char* data) 
{
  // Uses Page Write for 24LC256
  // Allows for 64 byte page boundary
  // Splits string into max 16 byte writes
  unsigned char i=0, counter=0;
  unsigned int  address;
  unsigned int  page_space;
  unsigned int  page=0;
  unsigned int  num_writes;
  unsigned int  data_len=0;
  unsigned char first_write_size;
  unsigned char last_write_size;  
  unsigned char write_size;  
  
  // Calculate length of data
  do{ data_len++; } while(data[data_len]);   
  Serial.print("writeEEPROM: Data Len ") ;
  Serial.println(data_len) ;

  // Calculate space available in first page
  page_space = int(((eeaddress/64) + 1)*64)-eeaddress;

  // Calculate first write size
  if (page_space>16){
     first_write_size=page_space-((page_space/16)*16);
     if (first_write_size==0) first_write_size=16;
  }   
  else 
     first_write_size=page_space; 
    
  // calculate size of last write  
  if (data_len>first_write_size) 
     last_write_size = (data_len-first_write_size)%16;   
  
  // Calculate how many writes we need
  if (data_len>first_write_size)
     num_writes = ((data_len-first_write_size)/16)+2;
  else
     num_writes = 1;  
     
  i=0;   
  address=eeaddress;
  for(page=0;page<num_writes;page++) 
  {
     if(page==0) write_size=first_write_size;
     else if(page==(num_writes-1)) write_size=last_write_size;
     else write_size=16;
  
     Wire.beginTransmission(deviceaddress);
     Wire.write((int)((address) >> 8));   // MSB
     Wire.write((int)((address) & 0xFF)); // LSB
     counter=0;
     do{ 
        Wire.write((byte) data[i]);
        i++;
        counter++;
     } while((data[i]) && (counter<write_size));  
     Wire.write(0) ;          //Terminate 
     Wire.endTransmission();
     address+=write_size;   // Increment address for next write
     
     delay(6);  // needs 5ms for page write
  }
}


void readEEPROM(int deviceaddress, unsigned int eeaddress, int *data) {
  Wire.beginTransmission(deviceaddress);
  Wire.write((int)(eeaddress >> 8));   // MSB
  Wire.write((int)(eeaddress & 0xFF)); // LSB
  Wire.endTransmission();
  int val = (int)Wire.read() ;
  *data = val ;
}

void readEEPROM(int deviceaddress, unsigned int eeaddress,  
                  char* data, unsigned int num_chars) 
{
  unsigned char i=0;
  Wire.beginTransmission(deviceaddress);
  Wire.write((int)(eeaddress >> 8));   // MSB
  Wire.write((int)(eeaddress & 0xFF)); // LSB
  Wire.endTransmission();
 
  Wire.requestFrom(deviceaddress,num_chars);
 
  while(Wire.available() && i < num_chars) data[i++] = Wire.read();

}
