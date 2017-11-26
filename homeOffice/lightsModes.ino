void powerOnSelfTest() {
  delay(1000);

  rgb(100, 0, 0);
  delay(750);

  rgb(0, 100, 0);
  delay(750);

  rgb(0, 0, 100);
  delay(750);

  rgb(0, 0, 0);
}

void verifyAlarmRequest() {
  rgb(0,0,0);
  delay(250);

  rgb(100,0,0);
  delay(250);

  rgb(0,0,0);
  delay(250);

  rgb(100,0,0);
  delay(250);

  rgb(0,0,0);
  delay(250);
}

void solidWhiteMode() {
  rgb(255, 150, 125);
}

void solidBlueMode() {
  rgb(0, 0, 255);
}

void nightVisionMode() {
  rgb(126, 0, 0);
}

void solidColorMode() {
  rgb(rVal, gVal, bVal);
}

void breatheMode() {
  int checkMode = mode;
  while (checkMode == mode) {
    breatheIn(primaryColor(), checkMode);
    breatheOut(primaryColor(), checkMode);
  }
}

void allOff() {
  rgb(0, 0, 0);
}

void rgbFadeMode() {
  int checkMode = mode;
  int rgbColor[3];

  // Start off with red.
  rgbColor[0] = 255;
  rgbColor[1] = 0;
  rgbColor[2] = 0;

  while (checkMode == mode) {
    // Choose the colors to increment and decrement.
    for (int decColor = 0; (decColor < 3) && (checkMode == mode); decColor += 1) {
      int incColor = decColor == 2 ? 0 : decColor + 1;

      // cross-fade the two colors.
      for (int i = 0; (i < 255) && (checkMode == mode); i += 1) {

        rgbColor[decColor] -= 1;
        rgbColor[incColor] += 1;

        rgb(rgbColor[0], rgbColor[1], rgbColor[2]);
        threadSafeDelay(breatheSpeed);
      }
    }
  }
}

void rgbBreatheMode() {
  int checkMode = mode;
  int color;
  while (checkMode == mode) {
    for (color = 0; color < 3; color++) {
      breatheIn(color, checkMode);
      breatheOut(color, checkMode);
    }
  }
}

void sunriseMode(int color, int duration) {
  int checkMode = mode;
  long fadeInMillis = (duration * 60L * 1000L);
  int delayDuration = fadeInMillis / 255L;

  breatheIn(color, checkMode, 255, delayDuration);
  modeManager(26);
}

void danceMode() {
  int checkMode = mode;
  while (checkMode == mode) {
    int rgbValue[3] = { 0, 0, 0 };
    int primaryColor = random(0, 3);
    rgbValue[primaryColor] = 255;

    for (int i = 0; i < 3; i++) {
      if (i != primaryColor) {
        rgbValue[i] = random(0, 85);
      }
    }

    rgb(rgbValue[0], rgbValue[1], rgbValue[2]);
    threadSafeDelay(100, 500);
  }
}
