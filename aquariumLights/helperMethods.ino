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

bool sendSensorData(int type, int param1) {
  sendSensorData(type, param1, 0, 0);
}

bool sendSensorData(int type, int param1, int param2) {
  sendSensorData(type, param1, param2, 0);
}

bool sendSensorData(int type, int param1, int param2, int param3) {
  payload_environment sensorData;

  sensorData = (payload_environment){ type, param1, param2, param3 };

  RF24NetworkHeader header(master_node);

  bool ok = network.write(header, &sensorData, sizeof(sensorData));
  bool sent;
  if (ok) {
    debugPrinter("Sensor data sent!", 1);
  } else {
    debugPrinter("Sensor data send failed!", 1);
  }
}
