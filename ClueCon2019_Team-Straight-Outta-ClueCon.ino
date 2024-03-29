// UCTRONICS Smart Robot Car demo (C)2018 uctronics
// Web: http://www.uctronics.com
// This demo support bluetooth control and IR remote control.
// This demo support Graphical programming
// Step1: Upload the Smart_Robot_Car_K0070_Scratch firmware
// Step2: Download the mblock software from http://www.mblock.cc/software/mblock/mblock3/
// Step3: Install The UCTRONICS_Smart_Robot_Car extension
// Step4: Enjoy your Graphical programming.

// Mainly work:
//After power on, it will come in automatic avoidance obstacle mode.
// At the same time, you also can control it by IR controller and bluetooth controller.
//If you use IR controller or bluetooth controller to control it, it will automatic leave 
//automatic avoidance obstacle mode.

/*
     Team Straight Outta ClueCon Customizations  (Chris Cline & Jeff Benson)
 ---------------------------------------------------------------------------------------------------------
 *   1) Defined static variables for speeds and delays.
 *   2) Customized the forward speed to 255 (max speed)
 *   3) Customized long delay between ultrasound scanning to be shorter.
 *   4) Commented out #2 motors.
 *   5) Informed the car to not stop at the start of the turn loop, but rather just slow down. 
 *   6) If the car determines that the distance between both left and right are similar, stop the car from moving forward.
 *   7) If the car determines that the distance between both left and right are similar, double the backup distance.
 *   8) Extend the distance used to scan for object detection from about 4" to about 10".
 ---------------------------------------------------------------------------------------------------------
 */


#include <UCMotor.h>
#include <Servo.h>
#include "UCNEC.h"
typedef struct MeModule
{
  int device;
  int port;
  int slot;
  int pin;
  int index;
  float values[3];
} MeModule;
union {
  byte byteVal[4];
  float floatVal;
  long longVal;
} val;

union {
  byte byteVal[8];
  double doubleVal;
} valDouble;

union {
  byte byteVal[2];
  short shortVal;
} valShort;


String mVersion = "0d.01.105";
boolean isAvailable = false;
boolean isBluetooth = false;
boolean isDetecte = false;
int len = 52;
char buffer[52];
char bufferBt[52];
byte index = 0;
byte dataLen;
boolean isStart = false;
char serialRead;
// define the device ID
#define ROBOTCAR 54
#define ULTRASONIC_SENSOR 55
#define SERVO 56
#define IR_SENSOR 57
#define FORWARD_SPEED 255           // 200 DEFAULT
#define TURN_SPEED 200              // 200 DEFAULT
#define ULTRASONIC_READ_WAIT 30000   //30000 DEFAULT
#define SHORT_DELAY 100              // 100 Default
#define LONG_DELAY 100               // 300 Default

#define GET 1
#define RUN 2
#define RESET 4
#define START 5
unsigned char prevc = 0;
double lastTime = 0.0;
double currentTime = 0.0;
uint8_t keyPressed = 0;
uint8_t command_index = 0;

bool isSmartMode  = true;
bool isIrMode  = true;
unsigned int S;
unsigned int Sleft;
unsigned int Sright;

uint32_t irValue = 0;

#define TURN_DIST 40

#define TRIG_PIN A2
#define ECHO_PIN A3
#define SERVO_PIN 10

UC_DCMotor leftMotor1(3, MOTOR34_64KHZ);
UC_DCMotor rightMotor1(4, MOTOR34_64KHZ);
//UC_DCMotor leftMotor2(1, MOTOR34_64KHZ);
//UC_DCMotor rightMotor2(2, MOTOR34_64KHZ);

Servo neckControllerServoMotor;
UCNEC myIR(2);
void setup() {
  pinMode(ECHO_PIN, INPUT); //Set the connection pin output mode Echo pin
  pinMode(TRIG_PIN, OUTPUT);//Set the connection pin output mode trog pin
  neckControllerServoMotor.attach(SERVO_PIN);
  neckControllerServoMotor.write(90);
  delay(2000);
  neckControllerServoMotor.detach();
  delay(SHORT_DELAY);
  myIR.begin();
  Serial.begin(115200);
  Serial.print("Version: ");
  Serial.println(mVersion);
}
void loop() {
  currentTime = millis() / 1000.0 - lastTime;
  readSerial();
  if (isAvailable) {
    unsigned char c = serialRead & 0xff;
    if (c == 0x55 && isStart == false) {
      if (prevc == 0xff) {
        index = 1;
        isStart = true; isSmartMode = false;
      }
    } else {
      prevc = c;
      if (isStart) {
        if (index == 2) {
          dataLen = c;
        } else if (index > 2) {
          dataLen--;
        }
        writeBuffer(index, c);
      }
    }
    index++;
    if (index > 51) {
      index = 0;
      isStart = false;
    }
    if (isStart && dataLen == 0 && index > 3) {
      isStart = false;
      parseData();
      index = 0;
    } else if (!isStart) {
      if (serialRead >= 1 && serialRead <= 5) { //0x01->forward  0x02->backward  0x03->left  0x04-> right  0x05->stop
        if(serialRead == 1)  {isDetecte = true;}
        else  {isDetecte = false;}
        leftMotor1.run(serialRead); rightMotor1.run(serialRead);
//        leftMotor2.run(serialRead); rightMotor2.run(serialRead);
        leftMotor1.setSpeed(FORWARD_SPEED); rightMotor1.setSpeed(FORWARD_SPEED);
//        leftMotor2.setSpeed(FORWARD_SPEED); rightMotor2.setSpeed(FORWARD_SPEED);
        neckControllerServoMotor.detach();
        delay(SHORT_DELAY);
        isSmartMode = false;
        myIR.begin();
      }
      if (serialRead == 0x06) { //automatic obstacle avoidance
        neckControllerServoMotor.attach(SERVO_PIN);
        neckControllerServoMotor.write(90);
        isSmartMode = true;
      }
       if (serialRead == 0x11) {
        Serial.write(0xAA);
        Serial.print("{\"version\":\"K0070\"}");
        Serial.write(0xAB);
      }
      if (serialRead == 0x10) { //automatic obstacle avoidance
        neckControllerServoMotor.attach(SERVO_PIN);
        neckControllerServoMotor.write(90);
        //Serial.println("Come in ");
        delay(SHORT_DELAY);
        neckControllerServoMotor.detach();
        delay(SHORT_DELAY);
        isSmartMode = false;
        myIR.begin();
        leftMotor1.run(0x05); rightMotor1.run(0x05);
//        leftMotor2.run(0x05); rightMotor2.run(0x05);
      }
    }
  }
  if (isSmartMode) {
    S = readPing();
    if (S>0 && S <= TURN_DIST ) {
      isIrMode = false;
      neckControllerServoMotor.attach(SERVO_PIN);
      neckControllerServoMotor.write(90);
      turn();
    } else if (S > TURN_DIST) {
      leftMotor1.run(1); rightMotor1.run(1);//1-> forward
//      leftMotor2.run(1); rightMotor2.run(1);
      leftMotor1.setSpeed(FORWARD_SPEED); rightMotor1.setSpeed(FORWARD_SPEED);
//      leftMotor2.setSpeed(FORWARD_SPEED); rightMotor2.setSpeed(FORWARD_SPEED);
      if (isIrMode == 0) {
        isIrMode = true;
        neckControllerServoMotor.detach();
        delay(SHORT_DELAY);
        myIR.begin();
      }
    } 
  }
  

}
unsigned char readBuffer(int index) {
  return isBluetooth ? bufferBt[index] : buffer[index];
}
void writeBuffer(int index, unsigned char c) {
  if (isBluetooth) {
    bufferBt[index] = c;
  } else {
    buffer[index] = c;
  }
}
void writeHead() {
  writeSerial(0xff);
  writeSerial(0x55);
}
void writeEnd() {
  Serial.println();
#if defined(__AVR_ATmega32U4__)
  Serial1.println();
#endif
}
void writeSerial(unsigned char c) {
  Serial.write(c);
#if defined(__AVR_ATmega32U4__)
  Serial1.write(c);
#endif
}
void readSerial() {
  isAvailable = false;
  if (Serial.available() > 0) {
    isAvailable = true;
    isBluetooth = false;
    serialRead = Serial.read();
  }
  if(isDetecte){
  S = readPing();
  if (S> 0 && S <= TURN_DIST ) {
    leftMotor1.run(5); rightMotor1.run(5);//5-> stop
    leftMotor1.setSpeed(200); rightMotor1.setSpeed(200);
  }
  }
  while (myIR.available())
  {
    irValue =  myIR.read();
  }
  if (irValue == 0xFF46B9 || irValue == 0xFF18E7)  //forward
  {

    irValue = 0; isSmartMode = false;

    irValue = 0;isDetecte = true;

    leftMotor1.run(1); rightMotor1.run(1);//1-> forward
//    leftMotor2.run(1); rightMotor2.run(1);
    leftMotor1.setSpeed(FORWARD_SPEED); rightMotor1.setSpeed(FORWARD_SPEED);
//    leftMotor2.setSpeed(FORWARD_SPEED); rightMotor2.setSpeed(FORWARD_SPEED);
  } else if (irValue == 0xFF15EA || irValue == 0xFF52AD) { //backward

    irValue = 0; isSmartMode = false;

    irValue = 0;isDetecte = false;

    leftMotor1.run(2); rightMotor1.run(2);//2-> backward
//    leftMotor2.run(2); rightMotor2.run(2);
    leftMotor1.setSpeed(FORWARD_SPEED); rightMotor1.setSpeed(FORWARD_SPEED);
//    leftMotor2.setSpeed(FORWARD_SPEED); rightMotor2.setSpeed(FORWARD_SPEED);
  } else if (irValue == 0xFF44BB || irValue == 0xFF08F7) { // left

    irValue = 0; isSmartMode = false;

    irValue = 0;isDetecte = false;

    leftMotor1.run(3); rightMotor1.run(3);//3-> left
//    leftMotor2.run(3); rightMotor2.run(3);
    leftMotor1.setSpeed(FORWARD_SPEED); rightMotor1.setSpeed(FORWARD_SPEED);
//    leftMotor2.setSpeed(FORWARD_SPEED); rightMotor2.setSpeed(FORWARD_SPEED);
  } else if (irValue == 0xFF43BC || irValue == 0xFF5AA5) { //right

    irValue = 0; isSmartMode = false;

    irValue = 0;isDetecte = false;

    leftMotor1.run(4); rightMotor1.run(4);//4-> right
//    leftMotor2.run(4); rightMotor2.run(4);
    leftMotor1.setSpeed(FORWARD_SPEED); rightMotor1.setSpeed(FORWARD_SPEED);
//    leftMotor2.setSpeed(FORWARD_SPEED); rightMotor2.setSpeed(FORWARD_SPEED);
  } else if (irValue == 0xFF40BF|| irValue == 0xFF1CE3) {  //stop

    irValue = 0; isSmartMode = false;

    irValue = 0;isDetecte = false;

    leftMotor1.run(5); rightMotor1.run(5);//5-> stop
//    leftMotor2.run(5); rightMotor2.run(5);
    leftMotor1.setSpeed(FORWARD_SPEED); rightMotor1.setSpeed(FORWARD_SPEED);
//    leftMotor2.setSpeed(FORWARD_SPEED); rightMotor2.setSpeed(FORWARD_SPEED);
  }
}
/*
  ff 55 len idx action device port  slot  data a
  0  1  2   3   4      5      6     7     8
*/
void parseData() {
  isStart = false;
  int idx = readBuffer(3);
  command_index = (uint8_t)idx;
  int action = readBuffer(4);
  int device = readBuffer(5);
  switch (action) {
    case GET: {
        writeHead();
        writeSerial(idx);
        readSensor(device);
        writeEnd();
      }
      break;
    case RUN: {
        runModule(device);
        callOK();
      }
      break;
    case RESET: {
        //reset
        leftMotor1.run(5); rightMotor1.run(5);
//        leftMotor2.run(5); rightMotor2.run(5);
        leftMotor1.setSpeed(0); rightMotor1.setSpeed(0);
//        leftMotor2.setSpeed(0); rightMotor2.setSpeed(0);
        neckControllerServoMotor.write(90);
        delay(SHORT_DELAY);
        neckControllerServoMotor.detach();
        delay(SHORT_DELAY);
        isSmartMode = false;
        myIR.begin();
        callOK();
      }
      break;
    case START: {
        //start
        callOK();
      }
      break;
  }
}
void callOK() {
  writeSerial(0xff);
  writeSerial(0x55);
  writeEnd();
}
void sendByte(char c) {
  writeSerial(1);
  writeSerial(c);
}
void sendString(String s) {
  int l = s.length();
  writeSerial(4);
  writeSerial(l);
  for (int i = 0; i < l; i++) {
    writeSerial(s.charAt(i));
  }
}
void sendFloat(float value) {
  writeSerial(0x2);
  val.floatVal = value;
  writeSerial(val.byteVal[0]);
  writeSerial(val.byteVal[1]);
  writeSerial(val.byteVal[2]);
  writeSerial(val.byteVal[3]);
}
void sendShort(double value) {
  writeSerial(3);
  valShort.shortVal = value;
  writeSerial(valShort.byteVal[0]);
  writeSerial(valShort.byteVal[1]);
}
void sendDouble(double value) {
  writeSerial(2);
  valDouble.doubleVal = value;
  writeSerial(valDouble.byteVal[0]);
  writeSerial(valDouble.byteVal[1]);
  writeSerial(valDouble.byteVal[2]);
  writeSerial(valDouble.byteVal[3]);
}
short readShort(int idx) {
  valShort.byteVal[0] = readBuffer(idx);
  valShort.byteVal[1] = readBuffer(idx + 1);
  return valShort.shortVal;
}
float readFloat(int idx) {
  val.byteVal[0] = readBuffer(idx);
  val.byteVal[1] = readBuffer(idx + 1);
  val.byteVal[2] = readBuffer(idx + 2);
  val.byteVal[3] = readBuffer(idx + 3);
  return val.floatVal;
}
long readLong(int idx) {
  val.byteVal[0] = readBuffer(idx);
  val.byteVal[1] = readBuffer(idx + 1);
  val.byteVal[2] = readBuffer(idx + 2);
  val.byteVal[3] = readBuffer(idx + 3);
  return val.longVal;
}
void turn() {
  leftMotor1.run(1); rightMotor1.run(1);//5-> stop
//  leftMotor2.run(5); rightMotor2.run(5);
  leftMotor1.setSpeed(100); rightMotor1.setSpeed(100);
//  leftMotor2.setSpeed(0); rightMotor2.setSpeed(0);
  neckControllerServoMotor.write(150);
  delay(LONG_DELAY);
  S = readPing();
  Sleft = S;
  neckControllerServoMotor.write(90);
  delay(LONG_DELAY);
  neckControllerServoMotor.write(30);
  delay(LONG_DELAY);
  S = readPing();
  Sright = S;
  neckControllerServoMotor.write(90);
  delay(LONG_DELAY);
  if (Sleft <= TURN_DIST && Sright <= TURN_DIST) {
    leftMotor1.run(5); rightMotor1.run(5);//5-> stop the wheels so we can adjust the backup speed correctly.
    leftMotor1.run(2); rightMotor1.run(2);//2-> backward
//    leftMotor2.run(2); rightMotor2.run(2);
    leftMotor1.setSpeed(TURN_SPEED); rightMotor1.setSpeed(TURN_SPEED);
//    leftMotor2.setSpeed(TURN_SPEED); rightMotor2.setSpeed(TURN_SPEED);
    delay(200);  // Extend the backup duration.
    int x = random(1);
    if (x = 0) {
      leftMotor1.run(4); rightMotor1.run(4);//4-> right
//      leftMotor2.run(4); rightMotor2.run(4);//4-> right
      leftMotor1.setSpeed(TURN_SPEED); rightMotor1.setSpeed(TURN_SPEED);
//      leftMotor2.setSpeed(TURN_SPEED); rightMotor2.setSpeed(TURN_SPEED);
    }
    else {
      leftMotor1.run(3); rightMotor1.run(3);//3-> left
//      leftMotor2.run(3); rightMotor2.run(3);
      leftMotor1.setSpeed(TURN_SPEED); rightMotor1.setSpeed(TURN_SPEED);
//      leftMotor2.setSpeed(TURN_SPEED); rightMotor2.setSpeed(TURN_SPEED);
    }
    delay(LONG_DELAY);
  } else {
    if (Sleft >= Sright) {
      leftMotor1.run(3); rightMotor1.run(3);//3-> left
//      leftMotor2.run(3); rightMotor2.run(3);
      leftMotor1.setSpeed(TURN_SPEED); rightMotor1.setSpeed(TURN_SPEED);
//      leftMotor2.setSpeed(TURN_SPEED); rightMotor2.setSpeed(TURN_SPEED);
    } else {
      leftMotor1.run(4); rightMotor1.run(4);//4-> right
//      leftMotor2.run(4); rightMotor2.run(4);//4-> right
      leftMotor1.setSpeed(TURN_SPEED); rightMotor1.setSpeed(TURN_SPEED);
//      leftMotor2.setSpeed(TURN_SPEED); rightMotor2.setSpeed(TURN_SPEED);
    }
    delay(LONG_DELAY);
  }
}
void runModule(int device) {
  //0xff 0x55 0x6 0x0 0x1 0xa 0x9 0x0 0x0 0xa
  int port = readBuffer(6);
  int pin = port;
  switch (device) {
    case SERVO: {
        int angle = readBuffer(6);
        if (angle >= 0 && angle <= 180)
        {
          neckControllerServoMotor.attach(SERVO_PIN);
          neckControllerServoMotor.write(angle);
        }
      }
      break;
    case ROBOTCAR: {
        int directionMode = readBuffer(6);
        int motorSpeed = readBuffer(8);
        leftMotor1.run(directionMode);   rightMotor1.run(directionMode);
//        leftMotor2.run(directionMode);   rightMotor2.run(directionMode);
        leftMotor1.setSpeed(motorSpeed); rightMotor1.setSpeed(motorSpeed);
//        leftMotor2.setSpeed(motorSpeed); rightMotor2.setSpeed(motorSpeed);
      }
      break;
  }
}

void readSensor(int device) {
  /**************************************************
      ff 55 len idx action device port slot data a
       0  1  2   3   4      5      6    7    8
  ***************************************************/
  float value = 0.0;
  int port, slot, pin;
  port = readBuffer(6);
  pin = port;
  switch (device) {
    case ULTRASONIC_SENSOR: {
        value = readPing();
        sendFloat(value);
      }break;  
  }
}

int getUltrasonicVal(void)
  {
      unsigned char cnt = 0;
      long cm, beginTime, stopTime;
      long waitCount = 0;
      pinMode(TRIG_PIN, OUTPUT); pinMode(ECHO_PIN, INPUT);
      digitalWrite(TRIG_PIN, LOW); delayMicroseconds(2);
      digitalWrite(TRIG_PIN, HIGH); delayMicroseconds(5);
      digitalWrite(TRIG_PIN, LOW);  waitCount = 0;
      while (!(digitalRead(ECHO_PIN) == 1)) {
            if (++waitCount >= ULTRASONIC_READ_WAIT)
              break;
      }
      beginTime = micros(); waitCount = 0;
      while (!(digitalRead(ECHO_PIN) == 0)) {
            if (++waitCount >= ULTRASONIC_READ_WAIT)
              break;
      }
      stopTime = micros();
      cm  = (float)(stopTime - beginTime) / 1000000 * 34000 / 2.5; // Extend the distance we leverage for calculating object detection.
      return cm;
}

int readPing(void)
{
  int Res[3];
  int res;
  int maxvalue; 
  int minvalue;
  
  for(int i=0;i<3;i++)
  {
    Res[i] = getUltrasonicVal();  
    delay(10); 
  }
  maxvalue = Res[0] >= Res[1] ? Res[0] : Res[1];
  maxvalue = maxvalue >= Res[2] ? maxvalue : Res[2];
  minvalue = Res[0] <= Res[1] ? Res[0] : Res[1];
  minvalue = minvalue <= Res[2] ? minvalue : Res[2];
  res = Res[0] + Res[1] + Res[2] - maxvalue - minvalue;
  return res;
}
