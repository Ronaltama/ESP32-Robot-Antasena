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

// Definisi Channel LEDC untuk PWM
#define LEDC_CHANNEL_1A  0  
#define LEDC_CHANNEL_1B  1  
#define LEDC_CHANNEL_2A  2  
#define LEDC_CHANNEL_2B  3  
#define LEDC_CHANNEL_3A  4  
#define LEDC_CHANNEL_3B  5  
#define LEDC_CHANNEL_4A  6  
#define LEDC_CHANNEL_4B  7  

// Konfigurasi PWM
#define LEDC_FREQ        5000  
#define LEDC_RESOLUTION  8


// Fungsi untuk mengatur kecepatan motor menggunakan ledcWrite
void setMotorSpeed(int channel_r, int channel_l, int speed) {
    int pwmValue = abs(speed);
    bool maju = (speed > 0);

    // Mengatur motor kanan (kanan maju atau mundur)
    ledcWrite(channel_r, maju ? pwmValue : 0);
    
    // Mengatur motor kiri (kiri maju atau mundur)
    ledcWrite(channel_l, maju ? 0 : pwmValue);
}

// Variabel global untuk mengatur frekuensi pencetakan serial
volatile int serialPrintCounter = 0;

// Callback saat menerima data
void onReceiveData(const uint8_t *mac_addr, const uint8_t *data, int len) {
    StaticJsonDocument<128> doc;
    DeserializationError error = deserializeJson(doc, data, len);
    
    if (error) {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.c_str());
        return;
    }

    // Mengambil nilai dari JSON dan mengkonversi ke int
    int xLeft = -(doc["xLeft"].as<int>());
    int yLeft = doc["yLeft"].as<int>();
    int z = doc["z"].as<int>();
    int btnA = doc["A"].as<int>();
    int btnB = doc["B"].as<int>();

    // Hitung kecepatan motor berdasarkan data joystick
    int m1 = constrain(xLeft + yLeft + z, -80, 80);
    int m2 = constrain(xLeft - yLeft + z, -80, 80);
    int m3 = constrain(-xLeft + yLeft + z, -80, 80);
    int m4 = constrain(-xLeft - yLeft + z, -80, 80);

    // Set motor berdasarkan nilai yang dihitung
    setMotorSpeed(LEDC_CHANNEL_1A, LEDC_CHANNEL_1B, m1);
    setMotorSpeed(LEDC_CHANNEL_2A, LEDC_CHANNEL_2B, m2);
    setMotorSpeed(LEDC_CHANNEL_3A, LEDC_CHANNEL_3B, m3);
    setMotorSpeed(LEDC_CHANNEL_4A, LEDC_CHANNEL_4B, m4);

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
    // Inisialisasi LEDC untuk setiap motor
    ledcSetup(LEDC_CHANNEL_1A, LEDC_FREQ, LEDC_RESOLUTION);
    ledcSetup(LEDC_CHANNEL_1B, LEDC_FREQ, LEDC_RESOLUTION);
    ledcSetup(LEDC_CHANNEL_2A, LEDC_FREQ, LEDC_RESOLUTION);
    ledcSetup(LEDC_CHANNEL_2B, LEDC_FREQ, LEDC_RESOLUTION);
    ledcSetup(LEDC_CHANNEL_3A, LEDC_FREQ, LEDC_RESOLUTION);
    ledcSetup(LEDC_CHANNEL_3B, LEDC_FREQ, LEDC_RESOLUTION);
    ledcSetup(LEDC_CHANNEL_4A, LEDC_FREQ, LEDC_RESOLUTION);
    ledcSetup(LEDC_CHANNEL_4B, LEDC_FREQ, LEDC_RESOLUTION);

    // Attach GPIO ke channel LEDC
    ledcAttachPin(RPWM_PIN1, LEDC_CHANNEL_1A);
    ledcAttachPin(LPWM_PIN1, LEDC_CHANNEL_1B);
    ledcAttachPin(RPWM_PIN2, LEDC_CHANNEL_2A);
    ledcAttachPin(LPWM_PIN2, LEDC_CHANNEL_2B);
    ledcAttachPin(RPWM_PIN3, LEDC_CHANNEL_3A);
    ledcAttachPin(LPWM_PIN3, LEDC_CHANNEL_3B);
    ledcAttachPin(RPWM_PIN4, LEDC_CHANNEL_4A);
    ledcAttachPin(LPWM_PIN4, LEDC_CHANNEL_4B);

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
