#include <esp_now.h>
#include <WiFi.h>
#include <Bluepad32.h>

// Ganti dengan MAC address penerima
uint8_t receiverMAC[] = {0xEC, 0x62, 0x60, 0x33, 0xFA, 0xD0};

GamepadPtr myGamepad = nullptr;

// Struktur paket kontrol (format biner)
typedef struct __attribute__((packed)) {
  int16_t xLeft;
  int16_t yLeft;
  int16_t z;
  uint8_t btnA;
  uint8_t btnB;
} ControlData;

ControlData controlData;

void onConnectedGamepad(GamepadPtr gp) {
  Serial.println("Gamepad Connected!");
  myGamepad = gp;
}

void onDisconnectedGamepad(GamepadPtr gp) {
  Serial.println("Gamepad Disconnected!");
  myGamepad = nullptr;
}

// Callback untuk mengkonfirmasi pengiriman data
void onSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Sent Successfully" : "Send Failed");
}

unsigned long lastSentTime = 0;
const unsigned long sendInterval = 10; // Interval pengiriman: 10 ms

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW Init Failed");
    return;
  }
  
  esp_now_register_send_cb(onSent);
  
  // Menambahkan peer (penerima)
  esp_now_peer_info_t peerInfo;
  memset(&peerInfo, 0, sizeof(peerInfo));
  memcpy(peerInfo.peer_addr, receiverMAC, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  peerInfo.ifidx = WIFI_IF_STA;
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }
  
  // Setup Bluepad32 untuk gamepad
  BP32.setup(&onConnectedGamepad, &onDisconnectedGamepad);
}

void loop() {
  BP32.update();
  
  if (myGamepad && myGamepad->isConnected()) {
    if (millis() - lastSentTime >= sendInterval) {
      lastSentTime = millis();
      
      // Ambil data gamepad dan masukkan ke dalam struktur
      controlData.xLeft = myGamepad->axisX();
      controlData.yLeft = -myGamepad->axisY() + 4; // Penyesuaian jika diperlukan
      controlData.z     = myGamepad->axisRX();
      controlData.btnA  = myGamepad->a() ? 1 : 0;
      controlData.btnB  = myGamepad->b() ? 1 : 0;
      
      // Kirim paket data secara biner
      esp_err_t result = esp_now_send(receiverMAC, (uint8_t *)&controlData, sizeof(ControlData));
      if (result != ESP_OK) {
        Serial.println("Error sending data");
      }
    }
  }
}
