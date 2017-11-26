void modeManager(int request) {
  modeManager(request, 0);
}

void modeManager(int request, int param1) {
  switch (request) {
    // First Row
    case 1:
      setMode(request);
      solidColorMode();
      break;
    case 2:
      sendButtonPress(242, true);
      break;
    case 3:
      setMode(request);
      allOff();
      break;
    // Second Row
    case 4:
      setMode(request);
      breatheMode();
      break;
    case 5:
      setMode(request);
      rgbBreatheMode();
      break;
    case 6:
      setMode(request);
      rgbFadeMode();
      break;
    // Third Row
    case 7:
      adjustColor('r', 'u');
      break;
    case 8:
      adjustColor('g', 'u');
      break;
    case 9:
      adjustColor('b', 'u');
      break;
    // Fourth Row
    case 10:
      adjustColor('r', 'd');
      break;
    case 11:
      adjustColor('g', 'd');
      break;
    case 12:
      adjustColor('b', 'd');
      break;
    // Shifted First Row
    case 13:
      setMode(request);
      solidWhiteMode();
      break;
    case 14:
      sendButtonPress(243, true);
      break;
    case 15:
      sendButtonPress(247, true);
      break;
    // Shifted Second Row
    case 16:
      sendButtonPress(244, false);
      break;
    case 17:
      sendButtonPress(245, false);
      break;
    case 18:
      sendButtonPress(246, false);
      break;
    // Shifted Third Row
    case 19:
      adjustColor('r', 'x');
      break;
    case 20:
      adjustColor('g', 'x');
      break;
    case 21:
      adjustColor('b', 'x');
      break;
    // Shifted Fourth Row
    case 22:
      adjustColor('r', 'n');
      break;
    case 23:
      adjustColor('g', 'n');
      break;
    case 24:
      adjustColor('b', 'n');
      break;
    // Other Modes
    case 25:
      setMode(request);
      sunriseMode(2, 5);
      break;
    case 26:
      setMode(request);
      solidBlueMode();
      break;
    case 27:
      setMode(request);
      nightVisionMode();
      break;
    case 28:
      setMode(request);
      danceMode();
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
