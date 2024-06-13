#include <BlindsObj.h>
#include <EEPROM.h>
BlindsObj::BlindsObj(){
  // default constructor

}
// Constructor for BlindsObj
BlindsObj::BlindsObj(int type, int stepPin, int dirPin){
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

    if (!isEEPROMRangeEmpty(EEPROM_ADDRESS,totalEEPROMSize )) {
      if(DEBUG) Serial.println(" EEPROM is not empty ");
      // EEPROM is empty, initialize with default value
      loadMotorParameters();          
    } else{
      if (DEBUG) Serial.println("EEPROM  NO values stored");
      //initiate theEEPROM
    }
}

// save slider position called pretty often;
void BlindsObj::saveSliderPosition(){
  int address = EEPROM_ADDRESS;
  currentSliderPosition =  getPositionOfSlider();
  EEPROM.put(address, currentSliderPosition);     
  EEPROM.commit();
  delay(20);
}

// Function to save motor parameters to EEPROM
void BlindsObj::saveMotorParameters() {
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
    savedFlag = true;

    EEPROM.put(address,limitSetupFlag);
    address += sizeof(int);

    EEPROM.put(address,highestPosition);
    address += sizeof(long int);

    EEPROM.put(address,lowestPosition);
    address += sizeof(long int);

    // Save String parameter
    EEPROM.put(address, name);
   
    EEPROM.commit(); // Don't forget to commit changes
    if(DEBUG) Serial.println("Saved the parameter of motor  for blinds : "+ name);
    delay(100);

}

// Function to load motor parameters from EEPROM
void  BlindsObj::loadMotorParameters() {
    
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
      maxPosition = highestPosition;
      minPosition = lowestPosition;
      motor.setSpeed(SPEED);
      motor.setCurrentPosition(getPositionOfMotor(currentSliderPosition));
    }
}

// Function to check if EEPROM is empty
bool BlindsObj::isEEPROMRangeEmpty(int startAddress, int endAddress) {
  for (int i = startAddress; i <= endAddress; i++) {
    if (EEPROM.read(i) != 0xFF) {
      return false;
    }
  }
  return true;
}

// setting blinds open or close;
void BlindsObj::setStatus(String newstatus){
  oldStatus = status;
  status = newstatus;
  if (status == "Open") { 
    blindsOpenFlag=true;
  }else if (status == "Close"){
    blindsOpenFlag=false;
  }
}

// set the highest position of the window blinds can roll up to 
long int BlindsObj::setWindowMax(long int Pos){
  highestPosition = Pos;
  maxPosition = highestPosition;
  limitSetupFlag = limitSetupFlag+ 2;
  currentSliderPosition =  getPositionOfSlider(highestPosition);
  saveMotorParameters();
  delay(100);
  return currentSliderPosition;
}

// set the lowest position of the window that a blind can roll down to.
long int BlindsObj::setWindowLow(long int Pos){
  lowestPosition = Pos;
  minPosition =  lowestPosition;
  limitSetupFlag = limitSetupFlag+ 1;
  currentSliderPosition = getPositionOfSlider(lowestPosition);
  saveMotorParameters();
  delay(100);
  return  currentSliderPosition;
}

//set the side on which the the controls are added;
// if  rightSided slider motor to be attached;
// maxPosition would be in the -ve side;
// minPosition would be still 0;
void BlindsObj::setSide(int dir){
  direction = dir;
  highestPosition = direction*highestPosition;
  maxPosition = highestPosition;
  if(DEBUG) Serial.println(" Setting SIDE **** " + String(dir) + " "+ String(highestPosition) + " " + String(maxPosition));
}

void BlindsObj::setBlindName(String blindname){
  name = blindname;
}

// factory resetting 
int BlindsObj::FactoryReset(){
  if(DEBUG) Serial.println("Resetting  blindsObj" + name);
  name = "";
  highestPosition = MAXSTEPS;
  lowestPosition = 0;
  maxPosition = highestPosition;
  minPosition = lowestPosition;
  sliderMax = 0;
  sliderMin = 100;
  direction = 1;
  limitSetupFlag = 0;
  status = "OFF";
  // Initialize your additional members here
  currentSliderPosition = sliderMin;
  motor.setCurrentPosition(getPositionOfMotor(currentSliderPosition));
  if (DEBUG) Serial.println(" Emptying the memory");
  for (int i = EEPROM_ADDRESS; i <= totalEEPROMSize; i++) {
    EEPROM.put(i, 0xFF);
  }
  EEPROM.commit();
  delay(1000);
  return 1;
}

// Gett the position in terms of the slider based on mechanical position
long int BlindsObj::getPositionOfSlider(long int Pos=-1){
  if (Pos == -1){
    return map(motor.currentPosition(), lowestPosition,highestPosition,sliderMin, sliderMax);  
  }
  updateTime = millis();
  return  map(Pos,lowestPosition,highestPosition, sliderMin,sliderMax);
}

// Get the position translated from slider to actual machine
int BlindsObj::getPositionOfMotor(int sliderPos=-1){
  updateTime = millis(); 
  if (sliderPos==-1){
      return  motor.currentPosition();
  }
  return map(sliderPos, sliderMin,sliderMax,lowestPosition,highestPosition);
}

// some basic variable retrieval functions;
int BlindsObj::getLimitFlag(){
  return limitSetupFlag;
}
bool BlindsObj::isBlindOpen(){
  return blindsOpenFlag;
}
String BlindsObj::getBlindName(){
  return name;
}
String BlindsObj::getSide(){
  return ( direction == 1?"LEFT": "RIGHT");
}

long int BlindsObj::ifRunningStop(){
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
long int BlindsObj::ifRunningHalt(){
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
void BlindsObj::moveBlinds(long int Pos){
  int currpos = ifRunningHalt();
  blindsOpenFlag = (direction*Pos) > (direction*currpos) ;
  if (blindsOpenFlag)
    motor.setSpeed((direction)*(SPEED));
  else motor.setSpeed((direction)*(-SPEED));
  motor.moveTo(Pos);
  status = (blindsOpenFlag? "Open":"Close");
  updateTime  = millis();
}

void BlindsObj::run(unsigned long delay=1000){

  if ((status.indexOf("OFF")!=0) && (motor.distanceToGo()*(status == "Open"?1:-1)*(direction) > 0 )){
      if (DEBUG && ( IterCount%1000) ==0 ) Serial.println( " running...");
      motor.run();
      // if (IterCount%10 == 0) saveSliderPosition();
      updateTime = millis();
      savedFlag = false;
  } else{
      if (DEBUG && ( IterCount%10000) ==0 ) Serial.println( " stopping...");
      motor.stop();
      setStatus("OFF");
      if (!savedFlag) {
        saveMotorParameters();
      }
  }
  delayMicroseconds(delay);

  IterCount = IterCount+1;
}
//open CW
void BlindsObj::openBlinds(){
    ifRunningStop();
    motor.setSpeed((direction)*(SPEED));
    if(DEBUG) Serial.println(" Speed :" + String(direction*SPEED) +" max: " + String(maxPosition));
    motor.moveTo(maxPosition);
    delayMicroseconds(500);
    updateTime  = millis();
    setStatus( "Open");
    saveMotorParameters();
}
//turn CCW
void BlindsObj::closeBlinds(){
    ifRunningStop();
    motor.setSpeed( (direction)*(-SPEED));
    if(DEBUG) Serial.println(" Speed :" + String(direction*(-1)*SPEED) +" min: " + String(minPosition));
    motor.moveTo(minPosition);
    delayMicroseconds(500);
    updateTime  = millis();
    setStatus( "Close" );
    // saveMotorParameters();
}
