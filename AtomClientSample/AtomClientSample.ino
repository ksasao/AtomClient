#include <M5Atom.h>
#include "AtomClient.h"
#include "Arduino_JSON.h"

char* ssid = "your-ssid";
char* password = "your-password";
char* server = "192.168.x.x";

AtomClient bc;

void setup() {
  M5.begin(true, false, true);

  // Client ID
  bc.setup("SampleClient01", ssid, password, server);

  // Subscribe topic
  bc.subscribe("Direction", topicReceived);
  Serial.println();
  Serial.print("MQTT Client ID: ");
  Serial.println(bc.getClientId());
}

void topicReceived(char* topic, byte* payload, unsigned int length) {
  payload[length] = '\0';

  JSONVar json = JSON.parse((char*)payload);
  int val = (double)json["Value"];

  Serial.print(topic);
  Serial.print(' ');
  Serial.print((char*)payload);
  Serial.print(" ");
  Serial.println(val);
}

void loop(){
  bc.update();

  // Publish topic
  int dir = 7;
  String id = bc.getName();
  String topicW = "Direction";
  String dataW = "{\"Id\":\"" + id + "\",\"Value\":" + String(dir) + "}";
  bc.publish(topicW,dataW);

  delay(1000);
  M5.update();
}
