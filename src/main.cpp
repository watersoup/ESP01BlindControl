#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ESPAsyncTCP.h>
#include <AccelStepper.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
#include <LITTLEFS.h>
#include "LittleFSsupport.h"
// #include <wifidata.h>

/////////////////////////////////////////////////////////////////////////////////////////
#define DEBUG 0
#include <BlindsObj.h>
#include <CPhandler.h>
///////////////////////////////////////////////////////////////////////////////

#define CLOSEBLINDS 0
#define OPENBLINDS 1
#define  SHORT_PRESS_TIME 500
#define LONG_PRESS_TIME 2000
#define  MIN_PRESS_TIME 200

//Server Name
String Server_Name = "JRBlinds";
// Set web server port number to 80
// ESP8266WebServer  server(80);
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
DNSServer dns;
CaptiveRequestHandler cp(&server, &dns);

// Assign output variables to GPIO pins
const int stepPin = 0; //GPIO0
const int dirPin = 2; // GPIO02
const int intensityButton = 3 ; // RX Pin; is going to be setup button
                          // if you hold it down longer then 3 sec;
const int onOffButton = 1; // Tx pin get activate once we close Serial 

unsigned long lastUpdateTime = millis();
unsigned long updateInterval = 9000; // upto 5 seconcds;
unsigned long updateGap = 3000;  // update every 1 second;
// Previous time
unsigned long previousTime = 0; 
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;

// button state
int currentState;
long releasedTime;
long pressedTime;
long pressDuration;
bool intensityButtonPressed = false;
#define MIN_ACTIVE_TIME 2000

// Create a new instance of the AccelStepper class:
// AccelStepper stepper = AccelStepper(1, stepPin, dirPin);

BlindsObj C1 ;

// broadasting info based on all prime number;
// default is 'blindName' if we intend to add more than one blind
// to a single ESP01
// Update only 
// status: when divisible by 2,
// limits flag: when divisible by 3
// SliderPosition when divisible by 5
void notifyClients( int x = 1){
  JsonDocument  jsonDoc;
  jsonDoc["blindName"] = C1.getBlindName();
  int openflag =C1.isBlindOpen()?1:0;
  int limitflag = C1.getLimitFlag();
  int pos = C1.getPositionOfSlider();
  if ( x%2 == 0 ){
   jsonDoc["status"] =openflag;
  }
  if (x%3 == 0){
   jsonDoc["limitSetupFlag"] =limitflag;
  }
  if (x%5 == 0){
   jsonDoc["sliderPosition"] = pos;
  }

  jsonDoc.shrinkToFit();
  String jsonString;
  serializeJson(jsonDoc,jsonString);
  if (DEBUG) Serial.println(" NotifyClient " + String(x)+ " :" + jsonString);
  ws.textAll(jsonString);
  lastUpdateTime = millis();
  delay(5*x);
  
}
void notifyLog( String message){
  ws.textAll("LOG :"+ message);
}
void notifyError(AsyncWebSocketClient *client, String message){
  client->text( message);
  client->close(1000, "Good bye");
}


/// @brief This is part of handling request over WebServer
/// @param request: AsyncWebServerRequest 
void handleRoot(AsyncWebServerRequest *request) {
  // Serve the HTML content 
  request->send(LittleFS, "/index.html", "text/html");
}

void handleSetupClick(AsyncWebSocketClient *client, String message){
  // Parse the JSON data
  JsonDocument  jsonDoc;
  deserializeJson(jsonDoc, message);
  jsonDoc.shrinkToFit();
  // Handle the data as needed
  int buttonStatus = jsonDoc["setupLimit"].as<int>();
  int currpos;
  if(DEBUG) Serial.println( "handleSetupClick " + String(buttonStatus));

  long int slidervalue = -1;
  if (C1.getLimitFlag() != 3){
  
  currpos = C1.ifRunningHalt();

  // opening status then set UpperMost position
    if (buttonStatus && (C1.getLimitFlag() != 2) ){
      if (!C1.isBlindOpen())  client->text("LOG : Blind is closed, it should be open !!!");
      slidervalue = C1.setWindowMax(currpos);
      // if (DEBUG) Serial.println("SetupClick: max  Slider Position:"+ String(C1.getPositionOfSlider()));
      //* 0 on the other end means completely open, need to be readjusted;

    }else if ((buttonStatus==0) && (C1.getLimitFlag() != 1)){
      if (C1.isBlindOpen())  client->text("LOG : Blind is Open, it should be closed !!!");
      slidervalue = C1.setWindowLow(currpos);
      // if (DEBUG) Serial.println("SetupClick : Low  Slider Position:"+ String(C1.getPositionOfSlider()));
      // if blind is no closed notify of error in the co-ordination;
    }
    notifyClients(30);

    if (DEBUG)
      Serial.println("ws/setupLimit: sliderPos:"+ String(slidervalue) + " Status:"+
          String(C1.isBlindOpen()) + " Limit:" + String(C1.getLimitFlag()));
  }
}

void handleOnOff(String message){
  // Parse the JSON data
  JsonDocument  jsonDoc;
  deserializeJson(jsonDoc, message);
  jsonDoc.shrinkToFit();
  // Handle the data as needed
  int blindsStatus = jsonDoc["status"].as<int>();
  if(DEBUG){
    Serial.print(" handleOnOff .." + String(C1.isBlindOpen()));
    Serial.flush();
  } 
  if (blindsStatus == OPENBLINDS ) {
      // call appropriate function to open the blind
    if(DEBUG) Serial.println(" Opening .." );
    C1.openBlinds();
  } else if ( blindsStatus == CLOSEBLINDS){
    // call function to close blinds
    if(DEBUG) Serial.println(" closing ..");
    C1.closeBlinds();
  }
  notifyClients(2);
}

void handleSetSliderValue(String message) { //GOOD
  // Handle setting the open range value based on the received value
  JsonDocument  jsonDoc;
  deserializeJson(jsonDoc, message);
  jsonDoc.shrinkToFit();
  int openRangeValue  = jsonDoc["sliderValue"].as<int>();  

  C1.moveBlinds(C1.getPositionOfMotor(openRangeValue));
  if ( DEBUG) Serial.println("WS_setSlider="+ C1.status + "ing " + String(jsonDoc["sliderValue"]));
  notifyClients(10);
}

void handleFirstLoad(AsyncWebServerRequest *request){
   String JSONstring = "";
  if (request->hasArg("blindName") and C1.getBlindName() == ""){
    C1.setBlindName(request->arg("blindName"));
    if (request->hasArg("rightSide") ){
      String direction = request->arg("rightSide");
      direction.toUpperCase();
      int dir = (direction.compareTo("YES")==0? 1 : -1);
      C1.setSide(dir);
      if (DEBUG ) Serial.println("Direction: " + String(dir) + " "+ direction );
    }
    int blindsOpenFlag = (C1.isBlindOpen()?OPENBLINDS:CLOSEBLINDS);

    JSONstring = "{\"limitSetupFlag\":"+ String( C1.getLimitFlag()) + 
                      ",\"blindName\":\"" + C1.getBlindName() +
                      "\",\"status\":" + String(blindsOpenFlag) + "}";
  }
  if (DEBUG) Serial.println( JSONstring);
  request->send(200, "application/json", JSONstring );  
}

// handling factory reset request with secretkey;
// should be called as <ipAddress>/reset?resetKey=<secretkey>
void handleFactoryReset(String message) {
    // Parse the JSON data
    JsonDocument jsonDoc;
    deserializeJson(jsonDoc, message);

    // Check if the factoryReset flag is true and the provided secret key is correct
    if (jsonDoc.containsKey("reset") ) {
        
        // Get the secret key from the message
        String secretKey = jsonDoc["reset"].as<String>();
        
        // Perform additional checks if needed, and then proceed with the factory reset logic
        if (secretKey == "XYZ123"){
          // For example, you can clear the blindName
          C1.FactoryReset();
          ESP.restart();
          
        }
        // Respond with a success message
        ws.textAll("{\"factoryResetStatus\": \"success\"}");

        if (DEBUG) Serial.println("Factory reset completed successfully");
    } else {
        // Respond with an error message
        ws.textAll("{\"factoryResetStatus\": \"error\", \"message\": \"Invalid factory reset request.\"}");
        if (DEBUG) Serial.println("Invalid factory reset request");
    }
}

// Initialize WiFi
// void initWiFi() {
//   WiFi.mode(WIFI_STA);
//   WiFi.hostname(Server_Name);
//   WiFi.begin(ssid, password); 
//   delay(5000);
//   while (WiFi.status() != WL_CONNECTED)
//   {
//     Serial.print(".");
//     delay(1000);
//   }
//   Serial.println("");
//   Serial.print("WiFi connected .. ");
//   Serial.print("Blinds IP address: ");
//   Serial.println(WiFi.localIP());
// }

void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    // New WebSocket client connected
    if (DEBUG) Serial.println("WebSocket client connected");
    client->text("LOG : WebSocket client connected !!!");
  } else if (type == WS_EVT_DISCONNECT) {
    // WebSocket client disconnected
    if (DEBUG) Serial.println("WebSocket client disconnected");
    notifyError(client,"WebSocket client disconnected");
  }else if (type == WS_EVT_DATA) {
    AwsFrameInfo *info = (AwsFrameInfo *)arg;
    // try{
      if (info->opcode == WS_TEXT) {
        // Handle text data received from the client
        String message = String( (const char *)data).substring(0, len);
        // Serial.println("Received message from WebSocket client: " + message);
        
        // Route for handling the momentary setup button click
        if ( message.indexOf("setupLimit") >= 0){      
          handleSetupClick(client, message);
        }
        // route the click of Close/Open blinds.
        if (message.indexOf("status") >=0 ){
            handleOnOff(message);
        }
        // Route for setting the slider value
        if (message.indexOf("sliderValue")>=0){
          if (DEBUG) Serial.println("slider value message Recd..");
          handleSetSliderValue(message);
        }
        // Route for the very first request after load;
        if (message.indexOf("initialize")>=0){
          // notify client of existing parameter if they have already
          // been initialized;

          notifyClients(30);
        }
      }
  }
}

void serverSetup() {
  // Serial.println("Server Setup in progress...");
  server.addHandler(&ws);
  ws.onEvent(onWsEvent);

  // Route for serving the HTML page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/index.html", "text/html");
  });

  // Route for handling the first load
  server.on("/firstLoad", HTTP_POST, [](AsyncWebServerRequest *request) {
    handleFirstLoad(request);
  });

  server.on("/reset", HTTP_GET, [](AsyncWebServerRequest *request){
  
    if (DEBUG) Serial.println(" *** factoryReset.."+ request->method());
    if ( request->hasArg("resetKey")){
      String key = request->arg("resetKey");
      if (key == "XYZ123"){
        C1.FactoryReset();
        // Respond with a success message
        ws.textAll("{\"factoryResetStatus\": \"success\"}");
        if (DEBUG) Serial.println("Factory reset completed successfully");
      }
    }
  });
}

// Function to handle intensity button press
// if open then goes closes
// else if closed then opens
// if button released it halts and notifies clients.
void handleIntensityButtonPress() {
    
    // Check for intensity button press
    if (digitalRead(intensityButton) == LOW ) {
      // Button pressed, handle the press
      if (!intensityButtonPressed) {
        if (DEBUG) Serial.println(" Intensity Button pressed ");
        else notifyLog( " Intensity Button Pressed");
        if (C1.isBlindOpen()) C1.closeBlinds();
        else if ( !C1.isBlindOpen()) C1.openBlinds();
      }
      intensityButtonPressed = true;
      
    } else if (intensityButtonPressed) {
      // Button released, handle the releas
      notifyLog( " Intensity Button released");
      intensityButtonPressed = false;
      C1.ifRunningStop();
      notifyClients(10);
      // reverse the blinds status open -> close or close -> open
    } 
}

// handleOnOffButtonPress:
// no arguments: if openstatus = open then closes else opens.
// if long-pressed: sets the higest opening position if last status was opening
//              else sets lowest point to which it can go to.
void handleOnOffButtonPress(){
  int currpos;
  currentState = digitalRead(onOffButton);
  if (currentState == LOW){
      if (DEBUG) Serial.println(" handleOnoffButtonPress..");
      else notifyLog( " OnOff Button Pressed");
      pressedTime = millis();
      // Button pressed
      // Wait for button release
      while (digitalRead(onOffButton) == LOW) {
          delay(10);
      }
      pressDuration = millis() - pressedTime;
      if (pressDuration >= LONG_PRESS_TIME){
        //call for setting upper or lower limit;
        notifyLog(" Long Press ...");
        currpos = C1.ifRunningHalt();
        if (C1.isBlindOpen() ) C1.setWindowMax(currpos);
        else if (!C1.isBlindOpen() ) C1.setWindowLow(currpos);
        notifyClients(30);
      } else {
        notifyLog(" Short Press ...");
        if ( C1.isBlindOpen() ) C1.closeBlinds();
        else C1.openBlinds();
      }
  }
}

void setup() {
  
  // Serial.begin(115200, SERIAL_8N1, SERIAL_TX_ONLY);

  // initialize the motor to be used;

  C1 = BlindsObj(1, stepPin, dirPin);
  // Mount the file system
  initFS();

  // Serial.println( "connecting to WIFI...");
  // initWiFi();
  cp.initWifiPortal("JRBlinds","jrblindsGuest");

  server.serveStatic("/", LittleFS, "/");  

  // Add more routes if needed
  serverSetup();
  server.begin();
  // Serial.flush();
  // Serial.end();
  //////////////////////////////////////////////////////////////
  // setup the pins of ESP01;  
  pinMode(dirPin, OUTPUT);
  pinMode(stepPin, OUTPUT);
  pinMode(onOffButton, FUNCTION_3);
  pinMode(intensityButton, FUNCTION_3);
  pinMode(onOffButton, INPUT_PULLUP);
  pinMode(intensityButton, INPUT_PULLUP);

  //////////////////////////////////////////////////////////////
  // Set outputs to LOW
  digitalWrite(dirPin, LOW);
  digitalWrite(stepPin, LOW);
  digitalWrite(onOffButton, HIGH);
  digitalWrite(intensityButton, HIGH);
  // analogWrite(intensityButton,0);
}

void loop(){

    // Check for open button press
    handleOnOffButtonPress();

    // check if intensity button pressed
    handleIntensityButtonPress();
    C1.run((unsigned long)500);
    // notify every 1 second for next 5 seconds from last updatetime
    if ((millis() - C1.updateTime) < updateInterval  && 
              (millis() - lastUpdateTime) > updateGap) {
      notifyClients(5);
      if (DEBUG) Serial.println("IntenButton:" + String(intensityButtonPressed));
      ws.cleanupClients();
    }
    delayMicroseconds(500);


}