#include <RF24Network.h>
#include <RF24Network_config.h>
#include <Sync.h>

#include <nRF24L01.h>
#include <RF24.h>
#include <RF24_config.h>

/////////////////////////////
// PIN LIST
/////////////////////////////
//  0: RX
//  1: TX
//  2: FLOWER BED LIGHTS
// *3:
//  4:
// *5:
// *6:
//  7: rf24(2)
//  8: rf24(6)
// *9:
//*10:
//*11: rf24(7)
// 12: rf24(4)
// 13: rf24(3)
// A0: PIR Out
// A1: Thermistor Out
// A2:
// A3:
// A4:
// A5:
// A6:
// A7:
// Ground: rf24(1)
// 5V: Vin
// 3v3: rf24(5)
// *PWM
/////////////////////////////

// NODE SPECIFIC CONFIGURATION
const bool debug = true;

// RF24 Radio Setup
RF24 radio(7,8);
RF24Network network(radio);
const uint16_t master_node = 00;
const uint16_t this_node = 05;
struct payload_command {
  byte mode;
  byte param1;
  byte param2;
  byte param3;
};
struct payload_environment {
  int sensor_type;
  int param1;
  int param2;
  int param3;
};

// Configure pins for sensors and switches
#define FLOWER_BED_LIGHTS 2
#define OTHER_RELAY 3
#define ON LOW
#define OFF HIGH

unsigned long currentMillis;
bool testSend = false;
int numTests;
int testsSent = 0;
unsigned long lastTestSent;
unsigned long testSendDelay = 5000;

void setup() {
  radio.begin();
  network.begin(/*channel*/ 90, /*node address*/ this_node);

  if (debug) {
    Serial.begin(115200);
    while (!Serial) {
      delay(1); // wait for serial port to connect. Needed for native USB port only
    }
    Serial.flush();
    delay(1000);
  }
  debugPrinter("Arduino Booting...", 0);
  debugPrinter("My node id is: ", this_node, 0);

  pinMode(FLOWER_BED_LIGHTS, OUTPUT);
  digitalWrite(FLOWER_BED_LIGHTS, OFF);
  pinMode(OTHER_RELAY, OUTPUT);
  digitalWrite(OTHER_RELAY, OFF);

  debugPrinter("Arduino Ready!", 1);
}

void loop() {
  currentMillis = millis();
  inputWatcher();
  testDispatcher();
}

void inputWatcher() {
  network.update();
  if (network.available()) { networkInputProcessor(); }
  if (Serial.available()) { serialInputProcessor(); }
}

void networkInputProcessor() {
  while (network.available()) {
    RF24NetworkHeader header;
    payload_command payload;

    network.read(header,&payload,sizeof(payload));

    if (payload.mode) {
      int mode = payload.mode;
      int param1 = payload.param1;
      int param2 = payload.param2;
      int param3 = payload.param3;

      debugPrinter("Triggered by rf24!", 1);
      debugPrinter("Mode: ", mode, 0);
      debugPrinter("Param1: ", param1, 0);
      debugPrinter("Param2: ", param2, 0);
      debugPrinter("Param3: ", param3, 0);

      modeManager(mode, param1);
    }
  }
}

void serialInputProcessor() {
  while (Serial.available()) {
    int i = 0;
    char serialRequest[13];

    while (true) {
      char inChar = Serial.read();

      if (isDigit(inChar)) {
        serialRequest[i] = inChar;
        i++;
      }

      if (inChar == '\n') {
        debugPrinter("serialRequest: ", serialRequest, 1);
        break;
      }
    }

    char modeArray[4] = {serialRequest[0], serialRequest[1], serialRequest[2]};
    char param1Array[4] = {serialRequest[3], serialRequest[4], serialRequest[5]};
    char param2Array[4] = {serialRequest[6], serialRequest[7], serialRequest[8]};
    char param3Array[4] = {serialRequest[9], serialRequest[10], serialRequest[11]};

    int mode = atoi(modeArray);
    int param1 = atoi(param1Array);
    int param2 = atoi(param2Array);
    int param3 = atoi(param3Array);

    debugPrinter("Triggered by serial!", 1);
    modeManager(mode, param1);
  }
}

void modeManager(int mode, int param1) {
  switch(mode) {
    case 1:
      lightSwitch(mode, param1, FLOWER_BED_LIGHTS);
      break;
    case 2:
      lightSwitch(mode, param1, OTHER_RELAY);
      break;
    case 254:
      debugPrinter("Ping Pong!", 1);
      sendSensorData(254, 254);
      break;
    case 255:
      startHeartbeat(param1);
      break;
    default:
      break;
  }

  debugPrinter("mode: ", mode, 1);
}

void lightSwitch(int mode, int param1, int pin) {
  switch(param1) {
    case 0:
      digitalWrite(pin, OFF);
      break;
    case 1:
      digitalWrite(pin, ON);
      break;
  }

  sendSensorData(mode, param1);
}

void startHeartbeat(int testLimit) {
  debugPrinter("Starting heartbeat test! # of tests: ", testLimit, 1);
  if (testLimit != 0) {
    numTests = testLimit;
  } else {
    numTests = 12;
  }
  testsSent = 1;
  testSend = !testSend;
}

void testDispatcher() {
  if (testSend) {
    if (testsSent <= numTests) {
      bool sendTestNow = ((currentMillis - lastTestSent) > testSendDelay);
      if (sendTestNow) {
        debugPrinter("testSend true and testsSent < numTests", 0);
        debugPrinter("testsSent: ", testsSent, 0);
        sendTest();

      }
    } else if (testsSent > numTests) {
      debugPrinter("testsSent >= numTests", 0);
      debugPrinter("testsSent: ", testsSent - 1, 0);
      testSend = false;
      testsSent = 0;
    }
  }
}

void sendTest() {
  debugPrinter("Sending test now!", 0);
  sendSensorData(255, 255);
  lastTestSent = currentMillis;
  testsSent = testsSent + 1;
}
