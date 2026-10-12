#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

// 0 = D1 mini STA MAC, 1 = D1 mini SoftAP MAC, 2 = broadcast
uint8_t targets[3][6] = {
  {0xE8, 0x9F, 0x6D, 0x92, 0x00, 0x46},
  {0xEA, 0x9F, 0x6D, 0x92, 0x00, 0x46},
  {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}
};
const char *targetNames[3] = {"STA MAC", "SoftAP MAC", "broadcast"};

const uint8_t btnPins[4] = {5, 6, 7, 8};
const uint8_t WIFI_CHANNEL = 1;
const unsigned long DEBOUNCE_MS = 30;
const unsigned long HEARTBEAT_MS = 2000;
const int TEST_SHOTS = 10;

typedef struct __attribute__((packed)) {
  uint8_t relay[4];
} Packet;

Packet pkt = {{0, 0, 0, 0}};

volatile int currentTarget = 2;
volatile int successCount[3] = {0, 0, 0};
volatile int failCount[3] = {0, 0, 0};
int activeTarget = 2;

bool lastReading[4] = {HIGH, HIGH, HIGH, HIGH};
bool stableState[4] = {HIGH, HIGH, HIGH, HIGH};
unsigned long lastChange[4] = {0, 0, 0, 0};
unsigned long lastSend = 0;
bool testing = true;

#if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 3, 0)
void onSent(const esp_now_send_info_t *info, esp_now_send_status_t status) {
#else
void onSent(const uint8_t *mac, esp_now_send_status_t status) {
#endif
  if (status == ESP_NOW_SEND_SUCCESS) successCount[currentTarget]++;
  else failCount[currentTarget]++;
  if (!testing) {
    Serial.printf("   delivery to %s: %s\n", targetNames[currentTarget],
                  status == ESP_NOW_SEND_SUCCESS ? "SUCCESS" : "FAIL");
  }
}

void sendTo(int t) {
  currentTarget = t;
  esp_err_t r = esp_now_send(targets[t], (uint8_t *)&pkt, sizeof(pkt));
  if (!testing) {
    Serial.printf("Sent [%d %d %d %d] to %s -> %s\n",
                  pkt.relay[0], pkt.relay[1], pkt.relay[2], pkt.relay[3],
                  targetNames[t], r == ESP_OK ? "queued" : "ERROR");
  }
  lastSend = millis();
}

void runLinkTest() {
  Serial.println("\n=== LINK TEST (10 shots per target) ===");
  for (int t = 0; t < 2; t++) {            // unicast targets only
    for (int s = 0; s < TEST_SHOTS; s++) {
      sendTo(t);
      delay(80);
    }
  }
  delay(200);
  for (int t = 0; t < 2; t++) {
    Serial.printf("%-10s : %d SUCCESS, %d FAIL\n",
                  targetNames[t], successCount[t], failCount[t]);
  }

  if (successCount[0] > 0)      activeTarget = 0;
  else if (successCount[1] > 0) activeTarget = 1;
  else                          activeTarget = 2;

  Serial.printf("=> Using: %s\n", targetNames[activeTarget]);
  if (activeTarget == 2) {
    Serial.println("=> No unicast ACK at all: C3 is not reaching the D1 mini.");
    Serial.println("   Check distance, channel, TX power, antenna.");
  }
  Serial.println("=======================================\n");
  testing = false;
}

void setup() {
  Serial.begin(115200);
  delay(1500);

  for (int i = 0; i < 4; i++) pinMode(btnPins[i], INPUT_PULLUP);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  WiFi.setTxPower(WIFI_POWER_8_5dBm);      // helps many C3 Super Mini boards
  esp_wifi_set_channel(WIFI_CHANNEL, WIFI_SECOND_CHAN_NONE);

  uint8_t ch; wifi_second_chan_t sc;
  esp_wifi_get_channel(&ch, &sc);
  Serial.print("Sender MAC: "); Serial.println(WiFi.macAddress());
  Serial.printf("Sender channel: %d\n", ch);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    while (true) delay(1000);
  }
  esp_now_register_send_cb(onSent);

  for (int t = 0; t < 3; t++) {
    esp_now_peer_info_t peer = {};
    memcpy(peer.peer_addr, targets[t], 6);
    peer.channel = WIFI_CHANNEL;
    peer.ifidx = WIFI_IF_STA;
    peer.encrypt = false;
    esp_err_t r = esp_now_add_peer(&peer);
    if (r != ESP_OK) Serial.printf("add_peer failed for %s (err %d)\n", targetNames[t], r);
  }

  runLinkTest();
  sendTo(activeTarget);
}

void loop() {
  for (int i = 0; i < 4; i++) {
    bool reading = digitalRead(btnPins[i]);

    if (reading != lastReading[i]) {
      lastChange[i] = millis();
      lastReading[i] = reading;
    }

    if ((millis() - lastChange[i]) > DEBOUNCE_MS && reading != stableState[i]) {
      stableState[i] = reading;
      if (reading == LOW) {
        pkt.relay[i] ^= 1;
        sendTo(activeTarget);
      }
    }
  }

  if (millis() - lastSend > HEARTBEAT_MS) sendTo(activeTarget);
}
