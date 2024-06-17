#include <ESPAsyncWifiManager.h>
#include <ESPAsyncWebServer.h>

class CaptiveRequestHandler {
private:
  DNSServer *dns;
  AsyncWiFiManager *wifiManager;
  AsyncWebServer *Server;
  
  public:
    //flag for saving data
      bool shouldSaveConfig = false;

    CaptiveRequestHandler(AsyncWebServer *server,DNSServer *dns) ;
    void saveConfigCallback();
    void configModeCallback (AsyncWiFiManager *);
    void initWifiPortal(const char * APname, const char * APpwd );
};