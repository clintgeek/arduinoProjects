#include <RF24Network.h>
#include <RF24Network_config.h>
#include <Sync.h>

#include <nRF24L01.h>
#include <RF24.h>
#include <RF24_config.h>

#define PHOTOCELL_PIN A1
#define PIR_PIN 3
#define CAL_TIME 30
#define PAUSE 5000

#define WATER_THERM_PIN A0
#define AIR_THERM_PIN A2
// resistance at 25 degrees C
#define THERMISTORNOMINAL 10000
// temp. for nominal resistance (almost always 25 C)
#define TEMPERATURENOMINAL 25
// how many samples to take and average, more takes longer
// but is more 'smooth'
#define NUMSAMPLES 30
// The beta coefficient of the thermistor (usually 3000-4000)
#define BCOEFFICIENT 3950
// the value of the 'other' resistor
#define SERIESRESISTOR 10000

/////////////////////////////
// PIN LIST
/////////////////////////////
//  0: RX
//  1: TX
//  2:
// *3: PIR
//  4:
// *5:
// *6: rf24(2)
//  7: rf24(6)
//  8:
// *9:
//*10:
//*11: rf24(7)
// 12: rf24(4)
// 13: rf24(3)
// A0: Water Thermistor
// A1: Light Meter
// A2: Air Thermistor
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

/////////////////////////////
const bool debug = true; ////
/////////////////////////////

// RF24 Radio Setup
RF24 radio(6,7);
RF24Network network(radio);
const uint16_t this_node = 025;
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

// Global variables
long unsigned int lowIn;
bool lockLow = true;
bool takeLowTime;
int samples[NUMSAMPLES];
unsigned long currentMillis;
unsigned long lastTempCheck;
unsigned long tempCheckDelay = 300000;
unsigned long lastLightCheck;
unsigned long lightCheckDelay = 120000;
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
  pinMode(PIR_PIN, INPUT);
  digitalWrite(PIR_PIN, LOW);
  calibratePIR();
  debugPrinter("Arduino Ready!", 1);
}

void loop() {
  currentMillis = millis();
  readPIR();
  checkLightTimer();
  checkTempTimer();
  inputWatcher();
  testDispatcher();
}

void readPIR() {
  if(digitalRead(PIR_PIN) == HIGH){
    if(lockLow){
      //makes sure we wait for a transition to LOW before any further output is made:
      lockLow = false;
      Serial.println("---");
      Serial.print("motion detected at ");
      Serial.print(millis()/1000);
      Serial.println(" sec");
      Serial.println("---");
      sendSensorData(249, 1);
      delay(50);
    }
    takeLowTime = true;
  }

  if(digitalRead(PIR_PIN) == LOW){
    if(takeLowTime){
      lowIn = millis();          //save the time of the transition from high to LOW
      takeLowTime = false;       //make sure this is only done at the start of a LOW phase
    }
    //if the sensor is low for more than the given pause,
    //we assume that no more motion is going to happen
    if(!lockLow && millis() - lowIn > PAUSE){
      //makes sure this block of code is only executed again after
      //a new motion sequence has been detected
      lockLow = true;
      Serial.println("---");
      Serial.print("motion ended at ");      //output
      Serial.print((millis() - PAUSE)/1000);
      Serial.println(" sec");
      Serial.println("---");
      sendSensorData(249, 0);
      delay(50);
    }
  }
}

void checkLightTimer() {
  bool checkLightNow = ((currentMillis - lastLightCheck) > lightCheckDelay);
  if (!lastLightCheck || checkLightNow) {
    checkLight();
  }
}

void checkLight() {
  int photocellReading = analogRead(PHOTOCELL_PIN);
  debugPrinter("Light: ", photocellReading, 0);
  sendSensorData(248, photocellReading);
  lastLightCheck = millis();
}

void checkTempTimer() {
  bool checkTempNow = ((currentMillis - lastTempCheck) > tempCheckDelay);
  if (!lastTempCheck || checkTempNow) {
    checkBothTemps();
  }
}

void checkBothTemps() {
  checkWaterTemp();
  checkAirTemp();
}

void checkWaterTemp() {
  int degrees_f = checkTemp(WATER_THERM_PIN);

  debugPrinter("Water Temperature (F): ", degrees_f, 0);
  sendSensorData(251, degrees_f);
  lastTempCheck = millis();
}

void checkAirTemp() {
  int degrees_f = checkTemp(AIR_THERM_PIN);

  debugPrinter("Air Temperature (F): ", degrees_f, 0);
  sendSensorData(250, degrees_f);
  lastTempCheck = millis();
}

int checkTemp(int pin) {
  uint8_t i;
  float average;

  // take N samples in a row, with a slight delay
  for (i = 0; i < NUMSAMPLES; i++) {
    samples[i] = analogRead(pin);
    delay(10);
  }

  // average all the samples out
  average = 0;
  for (i = 0; i < NUMSAMPLES; i++) {
    average += samples[i];
  }
  average /= NUMSAMPLES;

  // convert the value to resistance
  average = 1023 / average - 1;
  average = SERIESRESISTOR / average;

  float steinhart;
  int degrees_f;
  steinhart = average / THERMISTORNOMINAL;     // (R/Ro)
  steinhart = log(steinhart);                  // ln(R/Ro)
  steinhart /= BCOEFFICIENT;                   // 1/B * ln(R/Ro)
  steinhart += 1.0 / (TEMPERATURENOMINAL + 273.15); // + (1/To)
  steinhart = 1.0 / steinhart;                 // Invert
  steinhart -= 273.15;                         // convert to C
  steinhart = (steinhart * 9.0) / 5.0 + 32.0; // Convert Celcius to Fahrenheit

  if (steinhart <= 0) {
    degrees_f = 0;
  } else {
    degrees_f = (int)steinhart;
  }

  return degrees_f;
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

void modeManager(int mode, int param1) {
  switch(mode) {
    case 248:
      checkLight();
      break;
    case 250:
    case 251:
    case 252:
    case 253:
    debugPrinter("Checking temps on demand!", 1);
      checkBothTemps();
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

