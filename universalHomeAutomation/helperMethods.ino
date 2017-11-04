void debugPrinter(String title, int blankLines) {
  if (debug) {
    Serial.println(title);

    for (int i=0; i < blankLines; i++) {
      Serial.println();
    }
  }
}

void debugPrinter(String title, int value, int blankLines) {
  if (debug) {
    Serial.print(title);
    Serial.println(value);

    for (int i=0; i < blankLines; i++) {
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

    for (int i=0; i < blankLines; i++) {
      Serial.println();
    }
  }
}

void writeSingleColor(int colorIndex, int brightness) {
  int outPin;

  switch (colorIndex) {
    case 0:
      outPin = rOutPin;
      break;
    case 1:
      outPin = gOutPin;
      break;
    case 2:
      outPin = bOutPin;
      break;
  }

  analogWrite(outPin, brightness);
}

void rgb(int r, int g, int b) {
  writeSingleColor(0, r);
  writeSingleColor(1, g);
  writeSingleColor(2, b);
}

void setSingleColor(int colorIndex, int brightness) {
  switch (colorIndex) {
    case 0:
      rVal = brightness;
      break;
    case 1:
      gVal = brightness;
      break;
    case 2:
      bVal = brightness;
      break;
  }
}

void setRgb(int r, int g, int b) {
  setSingleColor(0, r);
  setSingleColor(1, g);
  setSingleColor(2, b);
}

void setMode(int modeToSet) {
  if (modeToSet != mode) {
    mode = modeToSet;
    sendSensorData(6, mode);
  }
}

void threadSafeDelay(int min, int max) {
  int totalDelay = random(min, max);
  threadSafeDelay(totalDelay);
}

void threadSafeDelay(int duration) {
  for (int delayCounter = 0; delayCounter < duration; delayCounter++) {
    inputWatcher();
    delay(1);
  }
}

void adjustColor(char color, char direction) {
  switch (color) {
    case 'r':
      rVal = adjustBrightness(rVal, direction);
      break;
    case 'g':
      gVal = adjustBrightness(gVal, direction);
      break;
    case 'b':
      bVal = adjustBrightness(bVal, direction);
      break;
  }
}

int adjustBrightness(int brightness, char direction) {
  switch (direction) {
    case 'u':
      if (brightness <= 245) {
        brightness = brightness + 10;
      } else {
        brightness = 255;
      }
      break;
    case 'd':
      if (brightness >= 10) {
        brightness = brightness - 10;
      } else {
        brightness = 0;
      }
      break;
    case 'x':
      brightness = 255;
      break;
    case 'n':
      brightness = 0;
      break;
  }

  return brightness;
}

int primaryColor() {
  int primaryColor;
  if (rVal > gVal && rVal > bVal) {
    primaryColor = 0;
  }
  else if (gVal > rVal && gVal > bVal) {
    primaryColor = 1;
  }
  else primaryColor = 2;

  return primaryColor;
}

void breatheIn(int color, int checkMode) {
  breatheIn(color, checkMode, 255, breatheSpeed);
}
void breatheIn(int color, int checkMode, int endBrightness, int transitionSpeed) {
  int rgbColor[3];
  rgbColor[0] = 0;
  rgbColor[1] = 0;
  rgbColor[2] = 0;

  for (int brightness = 0; (brightness <= endBrightness) && (checkMode == mode) ; brightness++) {
    rgbColor[color] = brightness;
    rgb(rgbColor[0], rgbColor[1], rgbColor[2]);
    threadSafeDelay(transitionSpeed);
  }
}

void breatheOut(int color, int checkMode) {
  breatheOut(color, checkMode, 255, breatheSpeed);
}
void breatheOut(int color, int checkMode, int startBrightness, int transitionSpeed) {
  int rgbColor[3];
  rgbColor[0] = 0;
  rgbColor[1] = 0;
  rgbColor[2] = 0;
  rgbColor[color] = startBrightness;

  for (int brightness = startBrightness; (brightness >= 1) && (checkMode == mode); brightness--) {
    rgbColor[color] = brightness;
    rgb(rgbColor[0], rgbColor[1], rgbColor[2]);
    threadSafeDelay(transitionSpeed);
  }
}

void sendSensorData(int type, int param1) {
  sendSensorData(type, param1, 0, 0);
}

void sendSensorData(int type, int param1, int param2) {
  sendSensorData(type, param1, param2, 0);
}

void sendSensorData(int type, int param1, int param2, int param3) {
  payload_environment sensorData;

  sensorData = (payload_environment){ type, param1, param2, param3 };

  RF24NetworkHeader header(pi_node);

  bool ok = network.write(header, &sensorData, sizeof(sensorData));
  if (ok) {
    debugPrinter("Sensor data sent!", 1);
  } else {
    debugPrinter("Sensor data send failed!", 1);
  }
}
