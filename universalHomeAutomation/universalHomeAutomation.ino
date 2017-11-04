#include <DHT.h>
#include <DHT_U.h>

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

/////////////////////////////
// NODE LIST
/////////////////////////////
# define RASPBERRY_PI 00
# define HOME_OFFICE 01
# define MASTER_BEDROOM 02
# define OUTDOOR_ATTIC 03
# define AQUARIUM_LIGHTS 04
/////////////////////////////

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

// NODE SPECIFIC CONFIGURATION
 const uint16_t this_node = MASTER_BEDROOM;
 const bool has_dht = true;
 #define DHTTYPE DHT11
 const bool has_key = false;
 const bool has_tv = true;
 const bool debug = true;

// Configure RGB Strip
int const gOutPin = 10;
int const rOutPin = 9;
int const bOutPin = 6;

// Configure TV Sensor
#define TV_SENSOR A4
unsigned long prevTvCheckMillis = 0;
unsigned long delayTvCheckMillis = 3000;

// Configure DHT Sensor
#define DHTPIN 5
DHT dht(DHTPIN, DHTTYPE);
unsigned long prevDhtCheckMillis = 0;
unsigned long delayDhtCheckMillis = 30000;

// Keypad Setup
byte const ROWS = 4;
byte const COLS = 3;
char charKeys[ROWS][COLS] = {
  { 1, 2, 3 },
  { 4, 5, 6 },
  { 7, 8, 9 },
  { 10, 11, 12 }
};
byte rowPins[ROWS] = {A0, A1, A2, A3};
byte colPins[COLS] = {A4, A5, 2};
Keypad kpd = Keypad(makeKeymap(charKeys), rowPins, colPins, ROWS, COLS);

// RF24 Radio Setup
RF24 radio(7,8);
RF24Network network(radio);
const uint16_t pi_node = 00;
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

// Initialize Global Variables
int rVal = 0;
int gVal = 0;
int bVal = 255;
int mode = 3;
int shift;
bool prevTvStatus;
int breatheSpeed = 30;
int previousTemperature;
int previousHumidity;
unsigned long currentMillis;

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
  network.begin(/*channel*/ 90, /*node address*/ this_node);
  if (has_dht) { dht.begin(); }
  powerOnSelfTest();

  debugPrinter("My node id is: ", this_node, 0);
  debugPrinter("Have DHT: ", has_dht, 0);
  debugPrinter("Have TV: ", has_tv, 0);
  debugPrinter("Have Key: ", has_key, 1);
  debugPrinter("Arduino Ready!", 1);
}

void loop() {
  inputWatcher();
  inputManager(mode);
}

void inputWatcher() {
  Watchdog.reset();
  currentMillis = millis();
  if (has_dht) { dhtMonitor(); }
  if (has_tv) { checkTvStatus(); }

  network.update();
  if (network.available()) { networkInputProcessor(); }
  if (Serial.available()) { serialInputProcessor(); }
  if (has_key && kpd.getKeys()) { keypadInputProcessor(); }
}

void dhtMonitor() {
  if (currentMillis - prevDhtCheckMillis >= delayDhtCheckMillis) {
    prevDhtCheckMillis = currentMillis;
    int f = dht.readTemperature(true);
    int h = dht.readHumidity();

    // Check if any reads failed and exit early (to try again).
    if (isnan(f) || isnan(h)) {
      debugPrinter("Failed to read from DHT sensor!", 1);
      return;
    }

    if (f != previousTemperature || h != previousHumidity) {
      debugPrinter("Temperature(F): ", f, 0);
      debugPrinter("Humidity(%): ", h, 1);

      previousTemperature = f;
      previousHumidity = h;
      sendSensorData(1, f, h);
    } else {
      debugPrinter("Temp/Humidity unchanged", 1);
    }
  }
}

void checkTvStatus() {
  if (currentMillis - prevTvCheckMillis >= delayTvCheckMillis) {
    prevTvCheckMillis = currentMillis;

    int tv_sensor_level = analogRead(TV_SENSOR);
    if (tv_sensor_level > 700 && prevTvStatus == false) {
      debugPrinter("TV Sensor: ", tv_sensor_level, 1);
    }

    bool currentTvStatus = (tv_sensor_level == 1023);
    if (prevTvStatus != currentTvStatus) {
      prevTvStatus = currentTvStatus;
      debugPrinter("TV is: ", currentTvStatus, 0);
      sendSensorData(2, currentTvStatus);
      if(currentTvStatus) {
        inputManager(25);
      } else {
        biasFadeOutMode();
      }
    }
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

      debugPrinter("Mode: ", mode, 0);
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

    debugPrinter("Mode: ", mode, 0);
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

  modeManager(mode);
}
