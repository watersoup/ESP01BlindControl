
#include <ESP8266WiFi.h>
#include <AccelStepper.h>
#include <WiFiClient.h>

#define OFF -1
#define SPEED 400 
#define MAXSPEED 3000
#define ACCEL 200
#define DEBUG 1
#define MAXSTEPS 100000;

#define EEPROM_ADDRESS 0
struct MotorParameters{
    int direction;
    int limitSetupFlag;
    long int highestPosition;
    long int lowestPosition;
    int currentSliderPosition;
    char name[10];

};

class BlindsObj  {
  public:
    WiFiClient client;
    AccelStepper motor;
    // flags to be used by the webpage;
    String name="";
    String location="";
    String status = ""; // motor status Open-to any non-minPosition Close-to minPosition 
    String oldStatus=""; // is copy of the previous status
    bool blindsOpenFlag = false; // true - is open ; false -- is close
    // INITIAL SETUP FLAG;
    int limitSetupFlag = 0; // 0 niether limitset, 1-lower Limit set ,  2- upperlimit set, 3-bothlimits set;
    long int highestPosition = 8500; //32768; // position to which to move
    long int lowestPosition = 0;
    long int maxPosition = highestPosition;
    long int minPosition = lowestPosition;
    long int sliderMax ;
    long int sliderMin ;
    long int currentSliderPosition; // position of digital slider 
  
    int direction = 1; // default direction of the motor -1 : reverse
    long int IterCount=1;
    unsigned long updateTime = 0;

    int totalEEPROMSize = 3*sizeof(int)+2*sizeof(long int)+ 10*sizeof(char);

    BlindsObj(){
      // default constructor

    }
    // Constructor for BlindsObj
    BlindsObj(int type, int stepPin, int dirPin){
        if(DEBUG) Serial.println(" Initializing the Blinds Object ...");
        motor = AccelStepper(type, stepPin, dirPin);
        motor.setPinsInverted(false,false, true); // not sure if useful
        motor.setAcceleration(ACCEL);
        motor.setSpeed(SPEED);
        motor.setMaxSpeed(MAXSPEED);

        // make sure status is off other wise 
        // motor will go off running;
        status = "OFF";
        // Initialize your additional members here
        sliderMax = 0;
        sliderMin = 100;
        currentSliderPosition = sliderMin;

        EEPROM.begin(totalEEPROMSize);

        if (!isEEPROMRangeEmpty(0,totalEEPROMSize )) {
          if(DEBUG) Serial.println(" EEPROM is not empty ");
          // EEPROM is empty, initialize with default value
          loadMotorParameters();          
        } else{
          if (DEBUG) Serial.println("EEPROM  NO values stored");
          //initiate theEEPROM
        }
    }
    
    // save slider position called pretty often;
    void saveSliderPosition(){
      int address = EEPROM_ADDRESS;
      currentSliderPosition =  getPositionOfSlider();
      EEPROM.put(address, currentSliderPosition);     
      EEPROM.commit();
      delay(20);
    }

    // Function to save motor parameters to EEPROM
    void saveMotorParameters() {
        int address = EEPROM_ADDRESS;
        currentSliderPosition = getPositionOfSlider();

        // Save integer parameters
        EEPROM.put( EEPROM_ADDRESS,currentSliderPosition);
        address += sizeof(int);

        int blindopenflagint =( blindsOpenFlag? 1:0 );
        EEPROM.put(address, blindopenflagint);
        address += sizeof(int);

        EEPROM.put(address,direction);
        address += sizeof(int);

        EEPROM.put(address,limitSetupFlag);
        address += sizeof(int);

        EEPROM.put(address,highestPosition);
        address += sizeof(int);

        EEPROM.put(address,lowestPosition);
        address += sizeof(int);

        // Save String parameter
        EEPROM.put(address, name);
       
        EEPROM.commit(); // Don't forget to commit changes
        if(DEBUG) Serial.println("Saved the parameter of motor  for blinds : "+ name);
        delay(100);

    }
    
    // Function to load motor parameters from EEPROM
    void  loadMotorParameters() {
        
        int address = EEPROM_ADDRESS;
        // Save integer parameters
        EEPROM.get(address,currentSliderPosition);
        address += sizeof(int);
        
        int blindopenflagint;
        EEPROM.get(address, blindopenflagint);
        address += sizeof(int);


        EEPROM.get(address,direction);
        address += sizeof(int);

        EEPROM.get(address,limitSetupFlag);
        address += sizeof(int);

        EEPROM.get(address,highestPosition);
        address += sizeof(long int);

        EEPROM.get(address,lowestPosition);
        address += sizeof(long int);

        EEPROM.get(address, name);        

        if(DEBUG) Serial.println("Loaded the parameter of motor for blinds : "+ name);
        delay(1000);
        if ( name != ""){
          blindsOpenFlag = blindopenflagint ==1;
          minPosition = highestPosition;
          maxPosition = lowestPosition;
          if (direction != 1) setSide(direction);
          motor.setCurrentPosition(getPositionOfMotor(currentSliderPosition));
        }
    }

    // Function to check if EEPROM is empty
    bool isEEPROMRangeEmpty(int startAddress, int endAddress) {
      for (int i = startAddress; i <= endAddress; i++) {
        if (EEPROM.read(i) != 0xFF) {
          return false;
        }
      }
      return true;
    }

    // setting blinds open or close;
    void setStatus(String newstatus){
      oldStatus = status;
      status = newstatus;
      if (status == "Open") { 
        blindsOpenFlag=true;
      }else if (status == "Close"){
        blindsOpenFlag=false;
      }
    }

    // set the highest position of the window blinds can roll up to 
    long int setWindowMax(long int Pos){
      highestPosition = Pos;
      maxPosition = highestPosition;
      limitSetupFlag = limitSetupFlag+ 2;
      currentSliderPosition =  getPositionOfSlider(highestPosition);
      return currentSliderPosition;
    }
    
    // set the lowest position of the window that a blind can roll down to.
    long int setWindowLow(long int Pos){
      lowestPosition = Pos;
      minPosition =  lowestPosition;
      limitSetupFlag = limitSetupFlag+ 1;
      currentSliderPosition = getPositionOfSlider(lowestPosition);
      return  currentSliderPosition;
    }

    //set the side on which the the controls are added;
    // if  rightSided slider motor to be attached;
    // maxPosition would be in the -ve side;
    // minPosition would be still 0;
    void setSide(int dir){
      direction = dir;
      highestPosition = direction*highestPosition;
      maxPosition = direction*maxPosition;
    }

    int FactoryReset(){
      if(DEBUG) Serial.println("Resetting  blindsObj" + name);
      name = "";
      highestPosition = 40000;
      lowestPosition = 0;
      sliderMax = 0;
      sliderMin = 100;
      direction = 1;
      limitSetupFlag = 0;
      status = "OFF";
      // Initialize your additional members her
      currentSliderPosition = sliderMin;
      if (DEBUG) Serial.println(" Emptying the memory");
      for (int i = EEPROM_ADDRESS; i <= totalEEPROMSize; i++) {
        EEPROM.put(i, 0xFF);
      }
      return 1;
    }
   
    // Gett the position in terms of the slider based on mechanical position
    long int getPositionOfSlider(long int Pos=-1){
      if (Pos == -1){
        return map(motor.currentPosition(), minPosition,maxPosition,sliderMin, sliderMax);  
      }
      updateTime = millis();
      return  map(Pos,minPosition,maxPosition, sliderMin,sliderMax);
    }

    // Get the position translated from slider to actual machine
    int getPositionOfMotor(int sliderPos=-1){
      updateTime = millis(); 
      if (sliderPos==-1){
          return  motor.currentPosition();
      }
      return map(sliderPos, sliderMin,sliderMax,minPosition,maxPosition);
    }
    
    // some basic variable retrieval functions;
    int getLimitFlag(){
      return limitSetupFlag;
    }
    bool isBlindOpen(){
      return blindsOpenFlag;
    }
    String getBlindName(){
      return name;
    }
    String getSide(){
      return ( direction == 1?"LEFT": "RIGHT");
    }
  
    long int ifRunningStop(){
      setStatus("OFF");
      long int Pos;
      if (motor.isRunning()){
        motor.stop();
      }
      Pos = motor.currentPosition();
      delayMicroseconds(500);
      return  Pos;
    }

    // This function halt the motor at faster pace and by increasing acceleration
    // and after stop is complete it resets it acceleration back to original value;
    long int ifRunningHalt(){
      long int Pos;      
      if (motor.isRunning()){
        motor.setAcceleration(1000);
        motor.stop();
        motor.setAcceleration(ACCEL);
      }
      setStatus("OFF");
      Pos = motor.currentPosition();
      delayMicroseconds(500);
      return Pos;
    }

    // set the goal for the motor to move, make sure to set the updateTime.
    void moveBlinds(long int Pos){
      int currpos = ifRunningHalt();
      blindsOpenFlag = (direction*Pos) > (direction*currpos) ;
      if (blindsOpenFlag)
        motor.setSpeed((direction)*(SPEED));
      else motor.setSpeed((direction)*(-SPEED));
      motor.moveTo(Pos);
      status = (blindsOpenFlag? "Open":"Close");
      updateTime  = millis();
      saveMotorParameters();
    }

    void run(unsigned long delay=1000){

      if ((status.indexOf("OFF")!=0) && (motor.distanceToGo() != 0 )){
          if (DEBUG && ( IterCount%1000) ==0 ) Serial.println( " running...");
          motor.run();
          // if (IterCount%10 == 0) saveSliderPosition();
          updateTime = millis();
      } else{
          if (DEBUG && ( IterCount%10000) ==0 ) Serial.println( " stopping...");
          motor.stop();
          setStatus("OFF");
      }
      delayMicroseconds(delay);

      IterCount = IterCount+1;
    }
    //open CW
    void openBlinds(){
        ifRunningStop();
        motor.setSpeed((direction)*(SPEED));
        motor.moveTo(maxPosition);
        delayMicroseconds(500);
        updateTime  = millis();
        setStatus( "Open");
        // saveMotorParameters();
    }
    //turn CCW
    void closeBlinds(){
        ifRunningStop();
        motor.setSpeed( (direction)*(-SPEED));
        motor.moveTo(minPosition);
        delayMicroseconds(500);
        updateTime  = millis();
        setStatus( "Close" );
        // saveMotorParameters();
    }
};
