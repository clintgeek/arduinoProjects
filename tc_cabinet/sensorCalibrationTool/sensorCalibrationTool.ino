// configure hardware
const int drawerSensorPin = 12;
const int buzzerPin = 10;

void setup() {
  pinMode(drawerSensorPin, INPUT);
  digitalWrite(drawerSensorPin, HIGH);
}

void loop() {
  alarmIfClosed();
}

void alarmIfClosed() {
  bool drawerOpen = digitalRead(drawerSensorPin);
  if (drawerOpen) {
    noTone(buzzerPin);
  } else {
    tone(buzzerPin, 2000);
  }
}

