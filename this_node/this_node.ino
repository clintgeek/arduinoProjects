#include <MQTTClient.h>
// MUST USE < v2.0 of MQTTClient
#include <ESP8266WiFi.h>
#include <RF24Network.h>
#include <RF24Network_config.h>
#include <Sync.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <RF24_config.h>

// WiFi Parameters
const char *ssid = "WiFiNetwork001";
const char *pass = "unicorn7799";

// Setup radios and networks
const unsigned long interval = 1000;
WiFiClient net;
MQTTClient client;
RF24 radio(4,15);
RF24Network network(radio);
uint16_t this_node = 00; // rf24 address
// payload to send to action nodes
struct message_action {
  unsigned char mode;
  unsigned char param1;
  unsigned char param2;
  unsigned char param3;
};
// payload sent from sensor nodes
struct message_sensor {
  uint16_t sensor_type;
  uint16_t param1;
  uint16_t param2;
  uint16_t param3;
};

// --------------------------
const bool debug = true;
// --------------------------


void setup() {
  if (debug) { Serial.begin(115200); }
  debugPrinter("NodeMCU Booting...", 0);
  WiFi.begin(ssid, pass);
  client.begin("192.168.1.17", net); // mqtt
  connect(); // wifi and mqtt
  radio.begin(); // rf24
  network.begin(/*channel*/ 90, /*node address*/ this_node); // rf24
  debugPrinter("NodeMCU Ready!", 1);
}

void connect() {
  debugPrinter("Checking WiFi...", 0);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  debugPrinter("Connecting MQTT...", 0);
  while (!client.connect("arduino", "try", "try")) {
    delay(500);
  }
  // Initialize MQTT subscriptions
  debugPrinter("Subscribing to home/rf24/commands/#", 0);
  client.subscribe("home/rf24/commands/#");

  debugPrinter("Connected to WiFi and MQTT!", 0);
}

void loop() {
  client.loop();
  delay(10); // <- fixes some issues with WiFi stability
  if (!client.connected()) {
    connect();
  }

  network.update();
  if (network.available()) { networkProcessor(); } //rf24
}

void networkProcessor() {
  debugPrinter("rf24 message found!", 0);
  while (network.available()) {
    RF24NetworkHeader header;
    message_sensor payload;
    String mqtt_topic = "home/rf24/";
    String mqtt_message;

    network.read(header,&payload,sizeof(payload));

    int sensor_id = header.from_node;
    int sensor_type = payload.sensor_type;
    mqtt_topic += String(sensor_id) + "/";
    mqtt_topic += String(sensor_type);

    int param1 = payload.param1;
    mqtt_message = String(param1);

    debugPrinter("From sensor: ", sensor_id, 0);
    debugPrinter("Sensor type: ", sensor_type, 0);
    debugPrinter("Param1: ", param1, 0);

    debugPrinter("mqtt_topic: ", mqtt_topic, 0);
    debugPrinter("mqtt_message: ", mqtt_message, 0);

    bool publish = client.publish(mqtt_topic, mqtt_message);
    debugPrinter("Publish Status: ", publish, 1);
  }
}

void messageReceived(String topic, String payload, char * bytes, unsigned int length) { // incoming MQTT
  debugPrinter("Incoming Message!", 0);
  debugPrinter("Topic: ", topic, 0);
  debugPrinter("Payload: ", payload, 1);

  messageHandler(topic, payload);
}

void messageHandler(String topic, String payload) {
  debugPrinter("messageHandler() called...", 0);
  int params [4];
  int target_node;

  int targetNodeIdPosition = topic.lastIndexOf('/') + 1;
  debugPrinter("targetNodeIdPosition: ", targetNodeIdPosition, 0);
  target_node = topic.substring(targetNodeIdPosition).toInt();
  debugPrinter("target_node: ", target_node, 0);

  getParams(payload, params);
  message_action actionmessage = getActionMessage(params);

  bool messageSent = sendRf24Message(target_node, actionmessage);
}

void getParams(String payload, int (& params) [4]) {
  char messagePayload[16];
  payload.toCharArray(messagePayload, 16);

  char modeArray[4] = { messagePayload[0], messagePayload[1], messagePayload[2] };
  char paramArray1[4] = { messagePayload[3], messagePayload[4], messagePayload[5] };
  char paramArray2[4] = { messagePayload[6], messagePayload[7], messagePayload[8] };
  char paramArray3[4] = { messagePayload[9], messagePayload[10], messagePayload[11] };
  int mode = atoi(modeArray);
  int param1 = atoi(paramArray1);
  int param2 = atoi(paramArray2);
  int param3 = atoi(paramArray3);

  params[0] = mode;
  params[1] = param1;
  params[2] = param2;
  params[3] = param3;

  debugPrinter("Mode: ", mode, 0);
  debugPrinter("Param1: ", param1, 0);
  debugPrinter("Param2: ", param2, 0);
  debugPrinter("Param3: ", param3, 0);
}

message_action getActionMessage(int (& params) [4]) {

  int mode = params[0];
  int param1 = params[1];
  int param2 = params[2];
  int param3 = params[3];
  message_action actionmessage = (message_action){ mode, param1, param2, param3 };

  return actionmessage;
}

bool sendRf24Message(int target_node, message_action (& actionmessage)) {
  // send message on the RF24Network
  debugPrinter("Sending message to node: ", target_node, 0);

  RF24NetworkHeader header(target_node);
  bool ok = network.write(header, &actionmessage, sizeof(actionmessage));
  bool sent = false;

  if (ok){
    debugPrinter("Command sent to node: ", target_node, 1);
    sent = true;
  } else {
    for (int i = 0; i < 10; i++) {
      delay (50);
      bool retry = network.write(header, &actionmessage, sizeof(actionmessage));
      if (retry) {
        sent = true;
        debugPrinter("Sent via retry to node: ", target_node, 1);
        break;
      }
    }
    if (!sent) { debugPrinter("Send failed to node: ", target_node, 1); }
  }
  return sent;
}
