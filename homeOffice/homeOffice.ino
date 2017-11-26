/////////////////////////////
// PIN LIST
/////////////////////////////
//  0: RX
//  1: TX
//  2:
// *3:
//  4: KEY(OPT)
// *5: DHT22
// *6: BLUE
//  7: rf24(2)
//  8: rf24(6)
// *9: RED
//*10: GREEN
//*11: rf24(7)
// 12: rf24(4)
// 13: rf24(3)
// A0: KEY(R0-G)
// A1: KEY(R1-O)
// A2: KEY(R2-OW)
// A3: KEY(C0-Br)
// A4: TV(+)
// A5: KEY(C1-BrW)
// A6: KEY(C2-BW)
// A7: KEY(C3-B)
// Ground: rf24(1), DHT(4)
// 5V: DHT(1), Vin
// 3v3: rf24(5)
// *PWM
/////////////////////////////

#include <Adafruit_SleepyDog.h>
#include <Key.h>
#include <Keypad.h>
#include <RF24Network.h>
#include <RF24Network_config.h>
#include <Sync.h>
#include <nRF24L01.h>
#include <printf.h>
#include <RF24.h>
#include <RF24_config.h>

// Initialize Global Variables
int rVal = 0;
int gVal = 0;
int bVal = 255;
int mode = 3;
int shift;
const int breatheSpeed = 30;
unsigned long currentMillis;
bool testSend = false;
int numTests;
int testsSent = 0;
unsigned long lastTestSent;
const unsigned long testSendDelay = 5000;

// NODE SPECIFIC CONFIGURATION
const int THIS_NODE = 01;
const bool debug = false;

// Configure RGB Strip
const int gOutPin = 10;
const int rOutPin = 9;
const int bOutPin = 6;

// Keypad Setup
const byte ROWS = 4;
const byte COLS = 3;
const char charKeys[ROWS][COLS] = {
  { 1, 2, 3 },
  { 4, 5, 6 },
  { 7, 8, 9 },
  { 10, 11, 12 }
};
const byte rowPins[ROWS] = {A0, A1, A2, A3};
const byte colPins[COLS] = {A4, A5, 2};
Keypad kpd = Keypad(makeKeymap(charKeys), rowPins, colPins, ROWS, COLS);

// RF24 Radio Setup
RF24 radio(7,8);
RF24Network network(radio);
const uint16_t master_node = 00;
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

void setup() {
  if (debug) {
    Serial.begin(115200);
  }

  debugPrinter("Arduino Booting...", 0);

  digitalWrite(rOutPin, LOW);
  pinMode(rOutPin, OUTPUT);
  digitalWrite(gOutPin, LOW);
  pinMode(gOutPin, OUTPUT);
  digitalWrite(bOutPin, LOW);
  pinMode(bOutPin, OUTPUT);

  int countdownMS = Watchdog.enable();

  radio.begin();
  network.begin(/*channel*/ 90, /*node address*/ THIS_NODE);
  powerOnSelfTest();

  debugPrinter("My node id is: ", THIS_NODE, 0);
  debugPrinter("Arduino Ready!", 1);
}

void loop() {
  currentMillis = millis();
  Watchdog.reset();
  testDispatcher();
  inputWatcher();
}

void inputWatcher() {
  network.update();
  if (network.available()) { networkInputProcessor(); }
  if (Serial.available()) { serialInputProcessor(); }
  if (kpd.getKeys()) { keypadInputProcessor(); }
}

void sendButtonPress(int buttonId, bool verifyPress) {
  debugPrinter("set button pressed:", buttonId, 1);
  if (verifyPress) { verifyButtonPress(); }
  sendSensorData(buttonId, 1);
  delay(500);
  sendSensorData(buttonId, 0);
}

void networkInputProcessor() {
  while (network.available()) {
    RF24NetworkHeader header;
    payload_command payload;

    network.read(header,&payload,sizeof(payload));

    if (payload.mode && (payload.mode != mode)) {
      int mode = payload.mode;
      int param1 = payload.param1;
      int param2 = payload.param2;
      int param3 = payload.param3;

      debugPrinter("Network Mode: ", mode, 0);
      if (param1 || param2 || param3) {
        debugPrinter("Param1: ", param1, 0);
        debugPrinter("Param2: ", param2, 0);
        debugPrinter("Param3: ", param3, 0);
      }

      inputManager(mode, param1, param2, param3);
    }
  }
}

void serialInputProcessor() {
  char serialRequest[13] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

  while (Serial.available()) {
    int i = 0;

    while (true) {
      char inChar = Serial.read();

      if (isDigit(inChar)) {
        serialRequest[i] = inChar;
        i++;
      }

      if (inChar == '\n') {
        serialRequest[13] = inChar;
        Serial.flush();
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

    debugPrinter("Serial Mode: ", mode, 0);
    if (param1 || param2 || param3) {
      debugPrinter("Param1: ", param1, 0);
      debugPrinter("Param2: ", param2, 0);
      debugPrinter("Param3: ", param3, 0);
    }

    inputManager(mode, param1, param2, param3);
  }
}

void keypadInputProcessor() {
  for (int i = 0; i < LIST_MAX; i++) // Scan the whole key list.
  {
    if ( kpd.key[i].stateChanged )   // Only find keys that have changed state.
    {
      switch (kpd.key[i].kstate) // Report active key state : IDLE, PRESSED, HOLD, or RELEASED
      {
        case PRESSED:
          shift = 1;
          debugPrinter("PRESSED: ", kpd.key[i].kcode + shift, 0);
          break;
        case HOLD:
          shift = 13;
          debugPrinter("HELD: ", kpd.key[i].kcode + shift, 0);
          break;
        case RELEASED:
          debugPrinter("RELEASED: ", kpd.key[i].kcode + shift, 0);
          inputManager(kpd.key[i].kcode + shift);
          break;
        case IDLE:
          break;
        default:
          break;
      }
    }
  }
}

void inputManager(int mode) {
  inputManager(mode, 0, 0, 0);
}

void inputManager(int mode, int param1, int param2, int param3) {
  if (mode == 001 && (param1 || param2 || param3)) {
    setRgb(param1, param2, param3);
  }

  modeManager(mode, param1);
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