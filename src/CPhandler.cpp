#include <CPhandler.h>


// Constructor implementation
CaptiveRequestHandler::CaptiveRequestHandler(AsyncWebServer *server, DNSServer *dns) {
    this->Server = server;
    this->dns = dns;
    this->wifiManager = new AsyncWiFiManager(server, dns);
}

// Callback function to indicate that we should save the config
void CaptiveRequestHandler::saveConfigCallback() {
    shouldSaveConfig = true;
}

// Callback function for entering configuration mode
void CaptiveRequestHandler::configModeCallback(AsyncWiFiManager *myWiFiManager) {
    Serial.println("Entered config mode");
    Serial.println(WiFi.softAPIP());
    Serial.println(myWiFiManager->getConfigPortalSSID());
}

// Initiate WifiManager
void CaptiveRequestHandler::initWifiPortal(const char * APname="", 
        const char * APpwd = ""){
    
    wifiManager->setAPCallback([this](AsyncWiFiManager *myWiFiManager) {
        this->configModeCallback(myWiFiManager);
    });

    wifiManager->setSaveConfigCallback([this]() {
        this->saveConfigCallback();
    });
    
    wifiManager->setConfigPortalTimeout(180);


    //fetches ssid and pass and tries to connect
    //if it does not connect it starts an access point with the specified name
    //here  "AutoConnectAP"
    //and goes into a blocking loop awaiting configuration
    if(!wifiManager->autoConnect(APname, APpwd)) {
        Serial.println("failed to connect and hit timeout.....");
        //reset and try again, or maybe put it to deep sleep
        ESP.restart();
        delay(1000);
    }
    //if you get here you have connected to the WiFi
    Serial.println("connected...yeey :)");
    Serial.println("local ip");
    Serial.println(WiFi.localIP());
    //set config save notify callback
    Serial.println(" starting the file system...");
    WiFi.hostname(APname);
    WiFi.reconnect();
}