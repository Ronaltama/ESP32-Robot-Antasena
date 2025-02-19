#include <WiFi.h>
#include <esp_now.h>
#include <ArduinoJson.h>

#define RPWM_PIN1 18
#define LPWM_PIN1 19
#define RPWM_PIN2 21
#define LPWM_PIN2 22
#define RPWM_PIN3 23
#define LPWM_PIN3 25
#define RPWM_PIN4 26
#define LPWM_PIN4 27

unsigned long previousMillis = 0;  // Menyimpan waktu terakhir motor diperbarui
const long updateInterval = 10;    // Interval pembaruan motor dalam ms

int m1 = 0, m2 = 0, m3 = 0, m4 = 0;

void setMotorSpeed(int pin_r, int pin_l, int speed, const char* motorName) {
    int pwmValue = abs(speed);
    bool maju = (speed > 0);

    if (speed == 0) {
        analogWrite(pin_r, 0);
        analogWrite(pin_l, 0);
        Serial.printf("%s: STOP\n", motorName);
    } else {
        analogWrite(pin_r, maju ? pwmValue : 0);
        analogWrite(pin_l, maju ? 0 : pwmValue);
        Serial.printf("%s: %s dengan PWM %d\n", motorName, maju ? "MAJU" : "MUNDUR", pwmValue);
    }
}

// Callback saat menerima data
void onReceiveData(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
    Serial.print("Data received from: ");
    for (int i = 0; i < 6; i++) {
        Serial.printf("%02X", info->src_addr[i]);
        if (i < 5) Serial.print(":");
    }
    Serial.println();

    // Parsing JSON
    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, data, len);
    if (error) {
        Serial.print("JSON Parsing failed: ");
        Serial.println(error.c_str());
        return;
    }

    int x = doc["xLeft"];
    int xLeft = x * -1;
    int yLeft = doc["yLeft"];
    int z = doc["z"];
    int btnA = doc["A"];
    int btnB = doc["B"];

    Serial.printf("xLeft: %d | yLeft: %d | z: %d | A: %d | B: %d\n", xLeft, yLeft, z, btnA, btnB);

    // Hitung kecepatan motor
    m1 = xLeft + yLeft + z;
    m2 = xLeft - yLeft + z;
    m3 = -xLeft + yLeft + z;
    m4 = -xLeft - yLeft + z;

    m1 = constrain(m1, -255, 255);
    m2 = constrain(m2, -255, 255);
    m3 = constrain(m3, -255, 255);
    m4 = constrain(m4, -255, 255);

    Serial.printf("M1: %d | M2: %d | M3: %d | M4: %d\n", m1, m2, m3, m4);
}

void setup() {
    Serial.begin(115200);

    pinMode(RPWM_PIN1, OUTPUT);
    pinMode(LPWM_PIN1, OUTPUT);
    pinMode(RPWM_PIN2, OUTPUT);
    pinMode(LPWM_PIN2, OUTPUT);
    pinMode(RPWM_PIN3, OUTPUT);
    pinMode(LPWM_PIN3, OUTPUT);
    pinMode(RPWM_PIN4, OUTPUT);
    pinMode(LPWM_PIN4, OUTPUT);

    WiFi.mode(WIFI_STA);
    if (esp_now_init() != ESP_OK) {
        Serial.println("Error initializing ESP-NOW");
        return;
    }
    esp_now_register_recv_cb(onReceiveData);
}

void loop() {
    unsigned long currentMillis = millis();  // Ambil waktu sekarang

    // Update kecepatan motor setiap `updateInterval` ms
    if (currentMillis - previousMillis >= updateInterval) {
        previousMillis = currentMillis;  // Simpan waktu terakhir update

        setMotorSpeed(RPWM_PIN1, LPWM_PIN1, m1, "M1");
        setMotorSpeed(RPWM_PIN2, LPWM_PIN2, m2, "M2");
        setMotorSpeed(RPWM_PIN3, LPWM_PIN3, m3, "M3");
        setMotorSpeed(RPWM_PIN4, LPWM_PIN4, m4, "M4");
    }
}
