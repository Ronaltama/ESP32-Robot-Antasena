// Robot Antasena - Pengirim (Tanpa Millis) - Error Lebih Sedikit daripada pakai Millis 
// Dari controller (joystick) ke esp 1, dikirim ke esp 2

#include <esp_now.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <Bluepad32.h>

// MAC Address ESP Penerima
uint8_t receiverMAC[] = {0xEC, 0x62, 0x60, 0x33, 0xFA, 0xD0};

GamepadPtr myGamepad = nullptr;

// Callback saat gamepad terhubung
void onConnectedGamepad(GamepadPtr gp) {
    myGamepad = gp;
}

// Callback saat gamepad terputus
void onDisconnectedGamepad(GamepadPtr gp) {
    myGamepad = nullptr;
}

// Callback saat ESP-NOW mengirim data
void onSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
    if (memcmp(mac_addr, receiverMAC, 6) == 0) { // Pastikan MAC address cocok dengan penerima
        Serial.print("Data sent to: ");
        for (int i = 0; i < 6; i++) {
            Serial.printf("%02X", mac_addr[i]);
            if (i < 5) Serial.print(":");
        }
        Serial.println(status == ESP_NOW_SEND_SUCCESS ? " - Sent Successfully" : " - Send Failed");
    }
}

void setup() {
    Serial.begin(115200);
    WiFi.mode(WIFI_STA);

    if (esp_now_init() != ESP_OK) {
        Serial.println("ESP-NOW Init Failed!");
        return;
    }
    esp_now_register_send_cb(onSent);

    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, receiverMAC, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;
    peerInfo.ifidx = WIFI_IF_STA;

    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Serial.println("Failed to add peer");
        return;
    }

    BP32.setup(&onConnectedGamepad, &onDisconnectedGamepad);
}

void loop() {
    BP32.update();

    if (myGamepad && myGamepad->isConnected()) {
        StaticJsonDocument<128> doc;
        doc["xLeft"] = myGamepad->axisX();
        doc["yLeft"] = -myGamepad->axisY() + 4;
        doc["z"] = myGamepad->axisRX();
        doc["A"] = myGamepad->a();
        doc["B"] = myGamepad->b();

        char jsonBuffer[128];
        size_t jsonLen = serializeJson(doc, jsonBuffer);
        esp_now_send(receiverMAC, (uint8_t *)jsonBuffer, jsonLen);
    }

    delay(0);
}
