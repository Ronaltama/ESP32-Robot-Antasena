#include <esp_now.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <Bluepad32.h>

// MAC Address ESP Penerima
uint8_t receiverMAC[] = {0xEC, 0x62, 0x60, 0x33, 0xFA, 0xD0};

GamepadPtr myGamepad = nullptr;

// Variabel untuk mengatur interval pengiriman data
unsigned long previousMillis = 0;
const long sendInterval = 10; // Interval pengiriman dalam ms

// Callback saat gamepad terhubung
void onConnectedGamepad(GamepadPtr gp) {
    Serial.println("Gamepad Connected!");
    myGamepad = gp;
}

// Callback saat gamepad terputus
void onDisconnectedGamepad(GamepadPtr gp) {
    Serial.println("Gamepad Disconnected!");
    myGamepad = nullptr;
}

// Callback saat ESP-NOW mengirim data
void onSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
    Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Sent Successfully" : "Send Failed");
}

void setup() {
    Serial.begin(115200);
    WiFi.mode(WIFI_STA);

    if (esp_now_init() != ESP_OK) {
        Serial.println("ESP-NOW Init Failed!");
        return;
    }
    esp_now_register_send_cb(onSent);

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

    BP32.setup(&onConnectedGamepad, &onDisconnectedGamepad);
}

void loop() {
    BP32.update();
    unsigned long currentMillis = millis();  // Ambil waktu sekarang

    // Kirim data hanya jika sudah mencapai interval yang ditentukan
    if (myGamepad && myGamepad->isConnected() && (currentMillis - previousMillis >= sendInterval)) {
        previousMillis = currentMillis;  // Simpan waktu terakhir pengiriman

        // Ambil data joystick
        int xLeft = myGamepad->axisX();
        int yLeft = -myGamepad->axisY() + 4;
        int z = myGamepad->axisRX();
        int btnA = myGamepad->a() ? 1 : 0;
        int btnB = myGamepad->b() ? 1 : 0;

        // Serialize data ke JSON
        StaticJsonDocument<200> doc;
        doc["xLeft"] = xLeft;
        doc["yLeft"] = yLeft;
        doc["z"] = z;
        doc["A"] = btnA;
        doc["B"] = btnB;

        char jsonBuffer[256];
        size_t jsonLen = serializeJson(doc, jsonBuffer, sizeof(jsonBuffer));

        esp_err_t result = esp_now_send(receiverMAC, (uint8_t *)jsonBuffer, jsonLen);
        if (result != ESP_OK) {
            Serial.println("Error sending data");
        }

        Serial.printf("Sent JSON: %s\n", jsonBuffer);
    }
}
