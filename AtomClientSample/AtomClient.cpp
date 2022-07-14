#include <ArduinoUniqueID.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "AtomClient.h"

typedef enum
{
  OFF,
  BOOTING,
  CONNECTING,
  CONNECTED,
  DISCONNECTED,
  SENDING,
  RECEIVING,
  ERROR,
  SAVING
} BROWNIE_STATUS;

const char hex[17]="0123456789ABCDEF";
char id[100];
String name;
char clientIdChar[50];
bool wifiConnected = false;


#define BROWNIE_CLIENT_BUFFER_SIZE 4096
char msgBuffer[BROWNIE_CLIENT_BUFFER_SIZE];
char topicBuffer[BROWNIE_CLIENT_BUFFER_SIZE];

char* _ssid;
char* _password;
char* _server;

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
std::function<void(char*, uint8_t*, unsigned int)> receivedCallback;

const char mqttUserName[] = "";
const char mqttPass[] = "";

void reConnect(bool toReboot);

// private methods

#if defined(_M5ATOM_H_)
void set_led(uint8_t r, uint8_t g, uint8_t b){
  int color = (g << 16) | (r << 8) | b;
  M5.dis.drawpix(0, color);
  delay(30);
}
void blink_led(uint8_t r, uint8_t g, uint8_t b, int duration, int count){
  for(int i=0; i< count; i++){
    set_led(r,g,b);
    delay(duration>>1);
    set_led(0,0,0);
    delay(duration>>1);
  }
}
#endif

void setStatus(BROWNIE_STATUS status){
#if defined(_M5ATOM_H_)
  switch(status){
    case OFF:
      set_led(0,0,0);
      break;
    case BOOTING:
      set_led(255,255,255);
      break;
    case CONNECTING:
      blink_led(255,128,0,500,1);
      break;
    case CONNECTED:
      set_led(0,0,255);
      break;
    case DISCONNECTED:
      blink_led(255,255,0,100,2);
      break;
    case SENDING:
      set_led(0,255,0);
      break;
    case RECEIVING:
      set_led(255,128,0);
      break;
    case ERROR:
      blink_led(255,0,0,100,10);
      break;
    case SAVING:
      break;
    default:
      set_led(0,0,0);
      break;
  }
#else
  switch(status){
    case CONNECTING:
      delay(500);
      break;
    default:
      break;
  }
#endif
}

void createClientId(String header){
  setStatus(BOOTING);
  id[UniqueIDsize*2]='\0';
  for(size_t i = 0; i < UniqueIDsize; i++){
      id[i*2] = hex[UniqueID[i] >> 4];
      id[i*2+1] = hex[UniqueID[i] & 0xF];
  }
  name = header;
  String clientId = name + "-" + String((char*) id);
  clientId.toCharArray(clientIdChar,clientId.length()+1);
}

// MQTT Subscribe
void callbackHook(char* topic, byte* payload, unsigned int length){
  setStatus(RECEIVING);
  receivedCallback(topic, payload,length);
  setStatus(CONNECTED);
}

void mqttSubscribe(String topic, MQTT_CALLBACK_SIGNATURE){
  mqttClient.subscribe(topic.c_str(),0); // QOS = 0
  receivedCallback = callback;
  mqttClient.setCallback(callbackHook);
}

// MQTT Publish
void mqttPublish(String topic, String data) {
  setStatus(SENDING);

  int length = data.length();
  data.toCharArray(msgBuffer,length+1);

  length = topic.length();
  topic.toCharArray(topicBuffer, length+1);
  mqttClient.publish(topicBuffer, msgBuffer);
  Serial.print(topic);
  Serial.print(" ");
  Serial.println(data);

  setStatus(CONNECTED);
}

void reboot(){
  Serial.println("Rebooting...");
  setStatus(BOOTING);
  delay(5 * 1000);
  ESP.restart();
}

void initWiFi(){
  // WiFi connection
  WiFi.begin(_ssid, _password);
  int count = 0;
  wifiConnected = true;
  while( WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    setStatus(CONNECTING);
    count++;
    if(count > 20){
      setStatus(ERROR);
      wifiConnected = false;
      break;
    }
  }
  if(wifiConnected){
    Serial.println("Connected.");
    // setBufferSize (PubSubClient >= 2.8.0)
    if(mqttClient.setBufferSize(BROWNIE_CLIENT_BUFFER_SIZE)){
      Serial.print("Max send/receive buffer size: ");
      Serial.println(BROWNIE_CLIENT_BUFFER_SIZE);
    }else{
      Serial.println("BROWNIE_CLIENT_BUFFER_SIZE is too large.");
    }
    mqttClient.setServer(_server, 1883); 
    reConnect(false);
    setStatus(CONNECTED);
  }else{
    Serial.println();
    Serial.println("Connection error. Check WiFi settings.");
    setStatus(DISCONNECTED);
    Serial.println("Wait for 1 min. to reboot");
    delay(60 * 1000);
    reboot();
  }
}

void reConnect(bool toReboot) {
  int resetCounter = 0;
  while (!mqttClient.connected()) {
    setStatus(CONNECTING);
    Serial.print("Attempting MQTT connection...");

    if (mqttClient.connect(clientIdChar,mqttUserName,mqttPass)) {
      Serial.print("Connected with Client ID:  ");
      Serial.print(clientIdChar);
      Serial.print(", Username: ");
      Serial.print(mqttUserName);
      Serial.print(" , Passwword: ");
      int i=0;
      while(mqttPass[i++]!='\0'){
        Serial.print("*");
      }
    }
    else {
      setStatus(ERROR);
      // http://pubsubclient.knolleary.net/api.html#state
      Serial.print("failed, state=");
      Serial.println(mqttClient.state());
      Serial.println("try again in 5 seconds");
      delay(5000);
    }
    if(resetCounter++ >= 10){
      if(toReboot){
        reboot();
      }else{
        wifiConnected = false;
        return;
      }
    }
  }
  setStatus(CONNECTED);
}

// public methods
AtomClient::AtomClient()
{
}

void AtomClient::setup(String name)
{
  createClientId(name);
}
void AtomClient::setup(String name, char* ssid, char* password, char* server)
{
  _ssid = ssid;
  _password = password;
  _server = server;
  createClientId(name);
  initWiFi();
}

String AtomClient::getName(void)
{
  return name;
}
char* AtomClient::getClientId(void)
{
  return clientIdChar;
}

void AtomClient::update(void){
  if(!wifiConnected){
    return;
  }
  reConnect(true);
  mqttClient.loop();
}
void AtomClient::publish(String topic, String body){
  if(!wifiConnected){
    setStatus(DISCONNECTED);
    return;
  }
  mqttPublish(topic,body);
}

void AtomClient::subscribe(String topic, MQTT_CALLBACK_SIGNATURE){
  mqttSubscribe(topic, callback);
}
void AtomClient::reboot(void){
  reboot();
}
void AtomClient::loop(){
  mqttClient.loop();
}
