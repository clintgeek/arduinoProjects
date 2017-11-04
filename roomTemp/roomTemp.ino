#include <RF24Network.h>
#include <RF24Network_config.h>
#include <Sync.h>

#include <nRF24L01.h>
#include <RF24.h>
#include <RF24_config.h>

// configure thermistor
#define AIR_THERM_PIN A0
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
int samples[NUMSAMPLES];

// RF24 Radio Setup
RF24 radio(6,7);
RF24Network network(radio);
const uint16_t this_node = 015;
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

// global variables
bool debug = true;
bool testSend = false;
int numTests;
int testsSent = 0;
unsigned long currentMillis;
unsigned long lastTempCheck;
unsigned long tempCheckDelay = 300000;
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
  debugPrinter("Arduino Ready!", 1);
}

void loop() {
  currentMillis = millis();
  checkTempTimer();
  inputDispatcher();
  testDispatcher();
}

void checkTempTimer() {
  bool checkTempNow = ((currentMillis - lastTempCheck) > tempCheckDelay);
  if (!lastTempCheck || checkTempNow) {
    checkAirTemp();
  }
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

void inputDispatcher() {
  network.update();
  if(network.available()) {
    networkInputProcessor();
  }
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

      debugPrinter("Triggered by rf24!", 0);
      debugPrinter("Mode: ", mode, 0);
      debugPrinter("Param1: ", param1, 0);
      debugPrinter("Param2: ", param2, 0);
      debugPrinter("Param3: ", param3, 1);

      modeManager(mode, param1);
    }
  }
}

void modeManager(int mode, int param1) {
  switch(mode) {
    case 250:
    case 251:
    case 252:
    case 253:
      checkAirTemp();
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
}