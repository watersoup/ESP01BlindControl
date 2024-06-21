
#include <ESP8266WiFi.h>
#include <AccelStepper.h>
#include <WiFiClient.h>

#define OFF -1
#define SPEED 400 
#define MAXSPEED 3000
#define ACCEL 200
#ifndef DEBUG
#define DEBUG 0
#endif
#define MAXSTEPS 100000;

#define EEPROM_ADDRESS 0
// struct MotorParameters{
//     int direction;
//     int limitSetupFlag;
//     long int highestPosition;
//     long int lowestPosition;
//     int currentSliderPosition;
//     char name[10];
// };

class BlindsObj  {
  private:
    WiFiClient client;
    AccelStepper motor;
    // flags to be used by the webpage;
    String name="";
    String location="";
    String oldStatus=""; // is copy of the previous status
    bool blindsOpenFlag = false; // true - is open ; false -- is close
    // INITIAL SETUP FLAG;
    int limitSetupFlag = 0; // 0 niether limitset, 1-lower Limit set ,  2- upperlimit set, 3-bothlimits set;
    long int highestPosition = MAXSTEPS; //32768; // highest point it can go when blinds is open 
    long int lowestPosition = 0;  // lowest position it can go when window is closed
    long int maxPosition = highestPosition; // these are temporary max and min position
    long int minPosition = lowestPosition;
    long int sliderMax ;
    long int sliderMin ;
    long int currentSliderPosition; // position of digital slider 
  
    long int IterCount=1;
    bool savedFlag = false ; // true - saved, false - not saved;

    int totalEEPROMSize = 4*sizeof(int)+2*sizeof(long int)+ 10*sizeof(char);
  
  public:
    String status = ""; // motor status Open-to any non-minPosition Close-to minPosition 
    int direction = 1; // default direction of the motor -1 : reverse
    unsigned long updateTime = 0;

    BlindsObj();

    // Constructor for BlindsObj
    BlindsObj(int type, int stepPin, int dirPin);

    // construtor for BlindsObj with EnablePin
    BlindsObj(int type, int stepPin, int dirPin, int enablePin);
    
    // save slider position called pretty often;
    void saveSliderPosition();

    // Function to save motor parameters to EEPROM
    void saveMotorParameters();
    
    // Function to load motor parameters from EEPROM
    void  loadMotorParameters();

    // Function to check if EEPROM is empty
    bool isEEPROMRangeEmpty(int startAddress, int endAddress) ;
    // setting blinds open or close;
    void setStatus(String newstatus);
    // set the highest position of the window blinds can roll up to 
    long int setWindowMax(long int Pos);    
    // set the lowest position of the window that a blind can roll down to.
    long int setWindowLow(long int Pos);

    //set the side on which the the controls are added;
    // if  rightSided slider motor to be attached;
    // maxPosition would be in the -ve side;
    // minPosition would be still 0;
    void setSide(int dir);

    // set the name of the blinds;
    void setBlindName(String );
    int FactoryReset();
   
    // Gett the position in terms of the slider based on mechanical position
    long int getPositionOfSlider(long int Pos=-1);

    // Get the position translated from slider to actual machine
    int getPositionOfMotor(int sliderPos=-1);
    
    // some basic variable retrieval functions;
    int getLimitFlag();
    bool isBlindOpen();
    String getBlindName();
    String getSide();
    
    long int ifRunningStop();
    // This function halt the motor at faster pace and by increasing acceleration
    // and after stop is complete it resets it acceleration back to original value;
    long int ifRunningHalt();
    // set the goal for the motor to move, make sure to set the updateTime.
    void moveBlinds(long int Pos);
    void run(unsigned long delay=1000);
    //open CW
    void openBlinds();
    //turn CCW
    void closeBlinds();
};
