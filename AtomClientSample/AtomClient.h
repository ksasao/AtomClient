#ifndef __ATOM_CLIENT_H__
#define __ATOM_CLIENT_H__

//#include <M5Stack.h>
//#include <M5StickC.h>
#include <M5Atom.h>
#define USE_BROWNIE_LED
#include <PubSubClient.h>

class AtomClient {
  private:
  public:
    AtomClient();
    String getName();
    char* getClientId();
    void setup(String name, char* ssid, char* password, char* server);
    void setup(String name);
    void update(void);
    void publish(String topic, String body);
    void subscribe(String topic, MQTT_CALLBACK_SIGNATURE);
    void reboot();
    void loop();
};
#endif
