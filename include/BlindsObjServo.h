#include <ESP8266WiFi.h>
#include <Servo.h>
#include <WiFiClient.h>

#define OFF -1
#define MAX_POSITION 180
#define MIN_POSITION 0
#define SLIDER_MAX 100
#define SLIDER_MIN 0

class BlindsObjServo {
public:
    WiFiClient client;
    Servo servoMotor;
    // flags to be used by the webpage;
    String name = "";
    String location = "";
    String status = ""; // motor status Open-to any non-minPosition Close-to minPosition 
    String oldStatus = ""; // is copy of the previous status
    bool blindsOpenFlag = false; // true - is open ; false -- is close
    // INITIAL SETUP FLAG;
    int limitSetupFlag = 0; // 0 neither limit set, 1-lower Limit set ,  2- upper limit set, 3-both limits set;
    long int highestPosition = MAX_POSITION;
    long int lowestPosition = MIN_POSITION;
    long int sliderMax = SLIDER_MAX;
    long int sliderMin = SLIDER_MIN;
    long int minPosition = highestPosition;
    long int maxPosition = lowestPosition;

    // Constructor for BlindsObj
    BlindsObjServo() {
        // Initialize your additional members here
        status = "OFF";
        sliderMax = 100;
        sliderMin = 0;
    }

    void setStatus(String newstatus) {
        oldStatus = status;
        status = newstatus;
        if (status == String("Open")) {
            blindsOpenFlag = true;
        } else if (status == String("Close")) {
            blindsOpenFlag = false;
        } else {
            if (oldStatus == String("Open")) {
                blindsOpenFlag = true;
            } else if (oldStatus == String("Close")) {
                blindsOpenFlag = false;
            }
        }
    }

    void setWindowMax(long int Pos) {
        highestPosition = min(Pos, highestPosition);
        limitSetupFlag += 2;
    }

    void setWindowLow(long int Pos) {
        lowestPosition = max(Pos, lowestPosition);
        limitSetupFlag += 1;
    }

    long int getPositionOfSlider(long int Pos = -1) {
        if (Pos == -1) {
            return map(servoMotor.read(), MIN_POSITION, MAX_POSITION, sliderMin, sliderMax);
        }
        return map(Pos, MIN_POSITION, MAX_POSITION, sliderMin, sliderMax);
    }

    long int ifRunningStop() {
        long int currpos = servoMotor.read();
        return currpos;
    }

    void moveBlinds(long int Pos) {
        Pos = min( max(minPosition, Pos), maxPosition);
        servoMotor.write(Pos);
        delay(15); // Allow time for the servo to move (adjust as needed)
        setStatus(blindsOpenFlag ? "Open" : "Close");
    }

    void openBlinds() {
        if (!blindsOpenFlag) {
            moveBlinds(highestPosition);
        }
    }

    void closeBlinds() {
        if (blindsOpenFlag) {
            moveBlinds(lowestPosition);
        }
    }
};