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

void solidWhiteMode() {
  rgb(255, 150, 125);
}

void biasLightMode() {
  rgb(0, 0, 80);
}

void biasFadeInMode() {
  breatheIn(2, mode, 80, 5);
  modeManager(28);
}

void biasFadeOutMode() {
  breatheOut(2, mode, 80, 5);
  modeManager(3);
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
  breatheIn(primaryColor(), checkMode);
  breatheOut(primaryColor(), checkMode);
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

void pinkTurquoiseBreatheMode() {
  int checkMode = mode;
  breathePink(checkMode);
  breatheTurquoise(checkMode);
}

void breathePink(int checkMode) {
  int brightness[3] = {0, 0, 0};

  for (int i = 0; (i < 50) && (checkMode == mode); i++) {
    brightness[0] += 4;
    brightness[1] += 0;
    brightness[2] += 1;

    rgb(brightness[0], brightness[1], brightness[2]);
    threadSafeDelay(40);
  }

  for (int i = 0; (i < 50) && (checkMode == mode); i++) {
    brightness[0] -= 4;
    brightness[1] -= 0;
    brightness[2] -= 1;

    rgb(brightness[0], brightness[1], brightness[2]);
    threadSafeDelay(40);
  }
}

void breatheTurquoise(int checkMode) {
  int brightness[3] = {0, 0, 0};

  for (int i = 0; (i < 50) && (checkMode == mode); i++) {
    brightness[0] += 0;
    brightness[1] += 4;
    brightness[2] += 4;

    rgb(brightness[0], brightness[1], brightness[2]);
    threadSafeDelay(40);
  }

  for (int i = 0; (i < 50) && (checkMode == mode); i++) {
    brightness[0] -= 0;
    brightness[1] -= 4;
    brightness[2] -= 4;

    rgb(brightness[0], brightness[1], brightness[2]);
    threadSafeDelay(40);
  }
}

void tvMode() {
  int rBrightness = random(0, 255);
  int gBrightness = random(0, 255);
  int bBrightness = random(0, 255);

  rgb(rBrightness, gBrightness, bBrightness);
  threadSafeDelay(250, 1750);
}

void rgbBreatheMode() {
  int checkMode = mode;
  int color;
  for (color = 0; color < 3; color++) {
    breatheIn(color, checkMode);
    breatheOut(color, checkMode);
  }
}

void sunriseMode(int color, int duration) {
  int checkMode = mode;
  long fadeInMillis = (duration * 60L * 1000L);
  int delayDuration = fadeInMillis / 255L;

  breatheIn(color, checkMode, 255, delayDuration);
  modeManager(26);
}

void sunsetMode(int color, int duration) {
  int checkMode = mode;
  long fadeInMillis = (duration * 60L * 1000L);
  int delayDuration = fadeInMillis / 255;

  breatheOut(color, checkMode, 255, delayDuration);
  modeManager(3);
}

void danceMode() {
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
