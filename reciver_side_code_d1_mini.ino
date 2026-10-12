#include <ESP8266WiFi.h>
#include <espnow.h>

extern "C" {
  #include <user_interface.h>
}

// Most relay modules are ACTIVE-LOW (LOW = ON). Set to false if yours is active-high.
#define RELAY_ACTIVE_LOW true

const uint8_t relayPins[4] = {D2, D5, D6, D7};
const uint8_t WIFI_CHANNEL = 1;

typedef struct __attribute__((packed)) {
  uint8_t relay[4];
} Packet;

Packet pkt;

void applyRelays() {
  for (int i = 0; i < 4; i++) {
    bool on = pkt.relay[i];
    digitalWrite(relayPins[i], RELAY_ACTIVE_LOW ? !on : on);
  }
}

void onDataRecv(uint8_t *mac, uint8_t *data, uint8_t len) {
  Serial.printf("Packet from %02X:%02X:%02X:%02X:%02X:%02X, len=%d\n",
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], len);
  if (len != sizeof(Packet)) return;
  memcpy(&pkt, data, sizeof(pkt));
  applyRelays();
  Serial.printf("Relays [%d %d %d %d]\n",
                pkt.relay[0], pkt.relay[1], pkt.relay[2], pkt.relay[3]);
}

void setup() {
  Serial.begin(115200);
  delay(500);

  // Relays OFF before anything else
  for (int i = 0; i < 4; i++) {
    pkt.relay[i] = 0;
    pinMode(relayPins[i], OUTPUT);
  }
  applyRelays();

  WiFi.mode(WIFI_AP_STA);
  WiFi.disconnect();
  wifi_set_channel(WIFI_CHANNEL);

  Serial.println();
  Serial.print("STA MAC:    "); Serial.println(WiFi.macAddress());
  Serial.print("SoftAP MAC: "); Serial.println(WiFi.softAPmacAddress());
  Serial.print("Channel:    "); Serial.println(wifi_get_channel());

  if (esp_now_init() != 0) {
    Serial.println("ESP-NOW init failed");
    while (true) delay(1000);
  }

  esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
  esp_now_register_recv_cb(onDataRecv);
  Serial.println("Waiting for data...");
}

void loop() {
}
