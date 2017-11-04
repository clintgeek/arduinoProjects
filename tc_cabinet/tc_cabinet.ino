#include <Servo.h>
#include <Key.h>
#include <Keypad.h>

/////////////////////////////
// PIN LIST - Arduino Nano
/////////////////////////////
//  0: RX
//  1: TX
//  2:
// *3:
//  4:
// *5:
// *6:
//  7:
//  8:
// *9: servo
//*10: buzzer
//*11:
// 12: drawer sensor (reed switch)
// 13:
// A0: battery monitor (voltage divider)
// A1: keypad (Common)
// A2: keypad (input 2)
// A3: keypad (input 4)
// A4: keypad (input 3)
// A5: keypad (input 1)
// A6:
// A7:
// Ground: power supply (-)
// 5V:
// 3v3:
// vin: power supply (+)
// *PWM
/////////////////////////////

// enable debug output
const bool debug = false;

// declare global variables
int secret = 1234; // set default password here, power cycle will reset it to this
unsigned long currentMillis;
unsigned long lastKeypress;
unsigned long unlockedAt;
unsigned int input[4];
bool updatePassword;
int inputCount;
bool isInLockedState;
int shift;

// configure timeouts
unsigned long unlockedAlarmTimeout = 60000; //in millis
unsigned long keypressTimeout = 10000; // in millis
unsigned long unlockDuration = 5000; // in millis

// configure keypad
/* 1x4 keypad ribbon pointing up
	Ribbon | Arduino | Color | key value
	1 A1 Black common
	2 A2 White input 2
	3 A3 Blue  input 4
	4 A4 Red   input 3
	5 A5 Green input 1 // don't ask...
*/
const byte rows = 1;
const byte cols = 4;
char keys[rows][cols] = {
	{'1', '2', '3', '4'},
};
byte rowPins[rows] = {A1};
byte colPins[cols] = {A5, A2, A4, A3};
Keypad kpd = Keypad( makeKeymap(keys), rowPins, colPins, rows, cols );

// configure servo
const int servoPin = 9; // needs PWM
const int unlockedPosition = 10;
const int lockedPosition = 45;
Servo myservo;

// configure drawer sensor (reed switch)
const int drawerSensorPin = 12;

// configure buzzer
const int buzzerPin = 10; // needs PWM
bool buzzer = true;
#define CHANGE_PASS_BEEPS  2
#define KEY_PRESS_BEEPS 1
#define GOOD_PASS_BEEPS 3
#define BAD_PASS_BEEPS 6
#define KEY_TIMEOUT_BEEPS 2
#define DRAWER_LOCK_BEEPS 2
#define DEBUG_ALERT_BEEPS 4

// configure battery monitor 410 / 3.44
const int batteryPin = A0;

float setLowVoltage = 7.1; // in full voltage across full pack
float lowVoltageConversion = (setLowVoltage * 3.44);
int lowVoltageAlarmLevel = (int)lowVoltageConversion;

void setup() {
	if (debug) {
		Serial.begin(115200);
		beep(DEBUG_ALERT_BEEPS);
	}
	debugPrinter("Arduino Ready!", 1);
	digitalWrite(drawerSensorPin, HIGH);
	pinMode(drawerSensorPin, INPUT);
	digitalWrite(drawerSensorPin, HIGH);
	pinMode(batteryPin, INPUT);
	myservo.attach(servoPin);
	setServoInSetup();
}

void loop() {
	currentMillis = millis();

	shouldLock();
	if (kpd.getKeys()) {
		keypadInput();
	}
	checkKeypressTimeout();
	if (debug) {
		serialInput();
	}
}

void keypadInput() {
	for (int i = 0; i < LIST_MAX; i++) { // Scan the whole key list.
		if (kpd.key[i].stateChanged) { // Only find keys that have changed state.
			unsigned int keycode;
			switch (kpd.key[i].kstate) { // Report active key state : IDLE, PRESSED, HOLD, or RELEASED
				case PRESSED:
					shift = 1;
					debugPrinter("PRESSED: ", kpd.key[i].kcode + shift, 0);
					beep(KEY_PRESS_BEEPS);
					break;
				case HOLD:
					shift = 5;
					debugPrinter("HELD: ", kpd.key[i].kcode + shift, 0);
					beep(CHANGE_PASS_BEEPS);
					break;
				case RELEASED:
					lastKeypress = currentMillis;
					keycode = kpd.key[i].kcode + shift;
					if (keycode == 6) { // hold button 2 to toggle buzzer
						debugPrinter("Buzzer Toggled: ", !buzzer, 0);
						toggleBuzzer();
					} else if (keycode == 8) { // hold button 4 to update password
						debugPrinter("Password Update Requested", 0);
						requestPassUpdate();
					} else {
						debugPrinter("RELEASED: ", keycode, 0);
						collectKeypadInput(keycode);
					}
					break;
				case IDLE:
					break;
				default:
					break;
			}
		}
	}
}

void serialInput() { // allows for testing/debug via serial monitor
	while (Serial.available()) {
		int i = 0;
		char serialInput[5];

		while (true) {
			char inChar = Serial.read();

			if (isDigit(inChar)) {
				serialInput[i] = inChar;
				i++;
			}

			if (inChar == '\n') {
				Serial.flush();
				debugPrinter("serialInput: ", serialInput, 0);
				break;
			}
		}

		int serialPassAsInt = atoi(serialInput);

		authPassword(serialPassAsInt);
	}
}

void collectKeypadInput(int inputChar) {
	debugPrinter("inputChar: ", inputChar, 0);
	input[inputCount] = inputChar;
	inputCount++;

	if (inputCount == 4) {
		unsigned int inputAsInt = 0;
		inputAsInt += input[0] * 1000;
		inputAsInt += input[1] * 100;
		inputAsInt += input[2] * 10;
		inputAsInt += input[3];
		clearInputStream();

		if (updatePassword) {
			passwordUpdate(inputAsInt);
		} else {
			authPassword(inputAsInt);
		}
	}
}

void authPassword(unsigned int inputAsInt) {
	debugPrinter("Drawer Status: ", drawerReadsAsOpen(), 1);
	debugPrinter("inputAsInt: ", inputAsInt, 0);
	debugPrinter("secret: ", secret, 0);

	if (inputAsInt == secret) {
		debugPrinter("Password Authenticated", 1);
		unlockServo();
		beep(GOOD_PASS_BEEPS);
		checkBatt();
	} else {
		debugPrinter("Wrong Password!", 1);
		beep(BAD_PASS_BEEPS);
	}
}

void unlockServo() {
	unlockedAt = currentMillis;
	if (isInLockedState == true) {
		myservo.write(unlockedPosition);
		isInLockedState = false;
		debugPrinter("Drawer Unlocked!", 1);
	}
}


void requestPassUpdate() {
	clearInputStream();
	if (isInLockedState == false) {
		updatePassword = true;
	}
}

void passwordUpdate(unsigned int inputAsInt) {
	updatePassword = false;

	if (isInLockedState == false) {
		secret = inputAsInt;
		debugPrinter("Password Updated", 1);
		beep(CHANGE_PASS_BEEPS);
	}
}

void shouldLock() {
	if (isInLockedState == false) {
		bool timeExpired = ((currentMillis - unlockedAt) > unlockDuration);

		if (timeExpired && !drawerReadsAsOpen()) {
			clearInputStream();
			lockServo();
		} else {
			drawerOpenAlarm();
		}
	}
}

void lockServo() {
	if (!drawerReadsAsOpen()) {
		delay(500);
		if (!drawerReadsAsOpen()) {
			myservo.write(lockedPosition);
			isInLockedState = true;
			debugPrinter("Drawer Locked!", 1);
			beep(DRAWER_LOCK_BEEPS);
		}
	}
}

void setServoInSetup() {
	isInLockedState = !drawerReadsAsOpen();
	if(isInLockedState) {
		myservo.write(lockedPosition);
	} else {
		myservo.write(unlockedPosition);
	}
}

bool drawerReadsAsOpen() {
	bool drawerOpen = digitalRead(drawerSensorPin);
	return drawerOpen;
}

void drawerOpenAlarm() {
	bool alarmTimeout = ((currentMillis - unlockedAt) > unlockedAlarmTimeout);
	if (alarmTimeout) {
		alarmBeep();
	}
}

void checkKeypressTimeout() {
	bool keyPressTimedOut = (lastKeypress && (currentMillis - lastKeypress) > keypressTimeout);

	if (keyPressTimedOut) {
		debugPrinter("Keypad Timeout", 1);
		clearInputStream();
		beep(KEY_TIMEOUT_BEEPS);
	}
}

void clearInputStream() {
	memset(input, 0, sizeof(input)); // clear any buffered input
	updatePassword = false;
	lastKeypress = 0;
	inputCount = 0;
}

void toggleBuzzer() {
	buzzer = !buzzer;
}

void beep(unsigned int repeat) {
	for (unsigned int i = 0; i < repeat; i++) {
		debugPrinter("BEEP!", 0);
		if (buzzer) {
			tone(buzzerPin, 2000);
			delay(150);
			noTone(buzzerPin);
		}
		if ((repeat - i) > 1) {
			delay(10);
		}
	}
}
void alarmBeep() {
	if (buzzer) {
		tone(buzzerPin, 2000);
		delay(150);
		noTone(buzzerPin);
		delay(150);
	}
}
void checkBatt() {
	int voltageReading = analogRead(batteryPin);
	debugPrinter("Voltage: ", voltageReading, 1);

	if (voltageReading < lowVoltageAlarmLevel) {
		batteryAlert();
	}
}

void batteryAlert() {
	// distinct alarm pattern for low battery
	if (buzzer) {
		tone(buzzerPin, 2000);
		delay(500);
		tone(buzzerPin, 1000);
		delay(500);
		tone(buzzerPin, 2000);
		delay(500);
		tone(buzzerPin, 1000);
		delay(500);
		tone(buzzerPin, 2000);
		delay(500);
		tone(buzzerPin, 1000);
		delay(500);
		noTone(buzzerPin);
	}
}

void debugPrinter(String title, int blankLines) {
	if (debug) {
		Serial.println(title);

		for (int i = 0; i < blankLines; i++) {
			Serial.println();
		}
	}
}

void debugPrinter(String title, int value, int blankLines) {
	if (debug) {
		Serial.print(title);
		Serial.println(value);

		for (int i = 0; i < blankLines; i++) {
			Serial.println();
		}
	}
}

void debugPrinter(String title, String value, int blankLines) {
	if (debug) {
		Serial.print(title);
		Serial.print(value);

		for (int i = 0; i < blankLines; i++) {
			Serial.println();
		}
	}
}

void debugPrinter(String title, char* value, int blankLines) {
	if (debug) {
		Serial.print(title);

		int x = 0;
		while (true) {
			if (value[x] == '\n') {
				Serial.println();
				break;
			} else {
				Serial.print(value[x]);
				x++;
			}
		}

		for (int i = 0; i < blankLines; i++) {
			Serial.println();
		}
	}
}
