// Robot Antasena - Penerima (Tanpa Millis) - Error lebih sedikit daripada pakai Millis 
// Esp 2 menerima input, inputnya dikirim ke driver motor

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

// Fungsi untuk mengatur kecepatan motor menggunakan ledcWrite
void setMotorSpeed(int pin_r, int pin_l, int speed) {
    int pwmValue = abs(speed);
    bool maju = (speed > 0);

    ledcWrite(pin_r, maju ? pwmValue : 0);
    ledcWrite(pin_l, maju ? 0 : pwmValue);
}

// Variabel global untuk mengatur frekuensi pencetakan serial
volatile int serialPrintCounter = 0;

// Callback saat menerima data
void onReceiveData(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
    StaticJsonDocument<128> doc;
    if (deserializeJson(doc, data, len)) return;

    int xLeft = -doc["xLeft"];
    int yLeft = doc["yLeft"];
    int z = doc["z"];
    int btnA = doc["A"];
    int btnB = doc["B"];

    // Hitung kecepatan motor berdasarkan data joystick
    int m1 = constrain(xLeft + yLeft + z, -80, 80);
    int m2 = constrain(xLeft - yLeft + z, -80, 80);
    int m3 = constrain(-xLeft + yLeft + z, -80, 80);
    int m4 = constrain(-xLeft - yLeft + z, -80, 80);

    // Set motor berdasarkan nilai yang dihitung
    setMotorSpeed(RPWM_PIN1, LPWM_PIN1, m1);
    setMotorSpeed(RPWM_PIN2, LPWM_PIN2, m2);
    setMotorSpeed(RPWM_PIN3, LPWM_PIN3, m3);
    setMotorSpeed(RPWM_PIN4, LPWM_PIN4, m4);

    // Cetak data hanya setiap 10 paket untuk mengurangi beban serial
    if (serialPrintCounter++ % 10 == 0) {
        Serial.printf("xLeft: %d | yLeft: %d | z: %d | A: %d | B: %d\n", xLeft, yLeft, z, btnA, btnB);
        Serial.printf("M1: %d | M2: %d | M3: %d | M4: %d\n", m1, m2, m3, m4);
        Serial.flush(); // Memastikan buffer serial dikosongkan lebih cepat
    }
}

void setup() {
    Serial.begin(115200);

    // Inisialisasi PWM untuk motor
    for (int pin : {RPWM_PIN1, LPWM_PIN1, RPWM_PIN2, LPWM_PIN2, RPWM_PIN3, LPWM_PIN3, RPWM_PIN4, LPWM_PIN4}) {
        ledcAttachPin(pin, pin);
        ledcSetup(pin, 5000, 8);
    }

    // Inisialisasi ESP-NOW
    WiFi.mode(WIFI_STA);
    if (esp_now_init() != ESP_OK) {
        Serial.println("ESP-NOW Init Failed!");
        return;
    }
    esp_now_register_recv_cb(onReceiveData);
}

void loop() {
    // Loop kosong karena semua dijalankan di callback onReceiveData()
}
