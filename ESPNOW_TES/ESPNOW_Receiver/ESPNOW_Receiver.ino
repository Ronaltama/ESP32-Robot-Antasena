#include <WiFi.h>
#include <esp_now.h>

// Definisi pin motor
#define RPWM_PIN1 18
#define LPWM_PIN1 19
#define RPWM_PIN2 21
#define LPWM_PIN2 22
#define RPWM_PIN3 23
#define LPWM_PIN3 25
#define RPWM_PIN4 26
#define LPWM_PIN4 27

// Definisi channel LEDC untuk masing-masing motor (maju dan mundur)
#define MOTOR1_FORWARD_CHANNEL 0
#define MOTOR1_REVERSE_CHANNEL 1
#define MOTOR2_FORWARD_CHANNEL 2
#define MOTOR2_REVERSE_CHANNEL 3
#define MOTOR3_FORWARD_CHANNEL 4
#define MOTOR3_REVERSE_CHANNEL 5
#define MOTOR4_FORWARD_CHANNEL 6
#define MOTOR4_REVERSE_CHANNEL 7

const int pwmFreq = 5000;      // Frekuensi PWM 5 kHz
const int pwmResolution = 8;   // Resolusi 8-bit (0–255)

// Struktur paket kontrol (format biner) – sama dengan pengirim
typedef struct __attribute__((packed)) {
  int16_t xLeft;
  int16_t yLeft;
  int16_t z;
  uint8_t btnA;
  uint8_t btnB;
} ControlData;

// Variabel global untuk menyimpan data yang diterima
volatile ControlData controlData;
volatile bool newDataReceived = false;

// Callback ESP-NOW untuk menerima data
void onReceiveData(const uint8_t *mac, const uint8_t *incomingData, int len) {
  if (len != sizeof(ControlData)) {
    Serial.println("Packet size mismatch.");
    return;
  }
  memcpy((uint8_t *)&controlData, incomingData, sizeof(ControlData));
  newDataReceived = true;
}

// Fungsi untuk mengatur kecepatan motor menggunakan LEDC PWM
void setMotorSpeed(int forwardChannel, int reverseChannel, int speed, const char* motorName) {
  int pwmValue = abs(speed);
  if (pwmValue > 255) pwmValue = 255; // Pastikan nilai tidak melebihi batas
  
  if (speed == 0) {
    ledcWrite(forwardChannel, 0);
    ledcWrite(reverseChannel, 0);
    Serial.printf("%s: STOP\n", motorName);
  } else if (speed > 0) {
    ledcWrite(forwardChannel, pwmValue);
    ledcWrite(reverseChannel, 0);
    Serial.printf("%s: MAJU dengan PWM %d\n", motorName, pwmValue);
  } else {
    ledcWrite(forwardChannel, 0);
    ledcWrite(reverseChannel, pwmValue);
    Serial.printf("%s: MUNDUR dengan PWM %d\n", motorName, pwmValue);
  }
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  esp_now_register_recv_cb(onReceiveData);
  
  // Inisialisasi LEDC PWM untuk tiap motor
  // Motor 1
  ledcSetup(MOTOR1_FORWARD_CHANNEL, pwmFreq, pwmResolution);
  ledcSetup(MOTOR1_REVERSE_CHANNEL, pwmFreq, pwmResolution);
  ledcAttachPin(RPWM_PIN1, MOTOR1_FORWARD_CHANNEL);
  ledcAttachPin(LPWM_PIN1, MOTOR1_REVERSE_CHANNEL);
  
  // Motor 2
  ledcSetup(MOTOR2_FORWARD_CHANNEL, pwmFreq, pwmResolution);
  ledcSetup(MOTOR2_REVERSE_CHANNEL, pwmFreq, pwmResolution);
  ledcAttachPin(RPWM_PIN2, MOTOR2_FORWARD_CHANNEL);
  ledcAttachPin(LPWM_PIN2, MOTOR2_REVERSE_CHANNEL);
  
  // Motor 3
  ledcSetup(MOTOR3_FORWARD_CHANNEL, pwmFreq, pwmResolution);
  ledcSetup(MOTOR3_REVERSE_CHANNEL, pwmFreq, pwmResolution);
  ledcAttachPin(RPWM_PIN3, MOTOR3_FORWARD_CHANNEL);
  ledcAttachPin(LPWM_PIN3, MOTOR3_REVERSE_CHANNEL);
  
  // Motor 4
  ledcSetup(MOTOR4_FORWARD_CHANNEL, pwmFreq, pwmResolution);
  ledcSetup(MOTOR4_REVERSE_CHANNEL, pwmFreq, pwmResolution);
  ledcAttachPin(RPWM_PIN4, MOTOR4_FORWARD_CHANNEL);
  ledcAttachPin(LPWM_PIN4, MOTOR4_REVERSE_CHANNEL);
}

void loop() {
  static unsigned long lastUpdateTime = 0;
  const unsigned long updateInterval = 10; // Update motor setiap 10 ms
  
  if (millis() - lastUpdateTime >= updateInterval) {
    lastUpdateTime = millis();
    
    if (newDataReceived) {
      // Salin data secara atomik
      ControlData data;
      noInterrupts();
      data = controlData;
      newDataReceived = false;
      interrupts();
      
      // Hitung kecepatan motor dengan kinematika sederhana
      int m1 = data.xLeft + data.yLeft + data.z;
      int m2 = data.xLeft - data.yLeft + data.z;
      int m3 = -data.xLeft + data.yLeft + data.z;
      int m4 = -data.xLeft - data.yLeft + data.z;
      
      m1 = constrain(m1, -255, 255);
      m2 = constrain(m2, -255, 255);
      m3 = constrain(m3, -255, 255);
      m4 = constrain(m4, -255, 255);
      
      // Update motor menggunakan LEDC PWM
      setMotorSpeed(MOTOR1_FORWARD_CHANNEL, MOTOR1_REVERSE_CHANNEL, m1, "M1");
      setMotorSpeed(MOTOR2_FORWARD_CHANNEL, MOTOR2_REVERSE_CHANNEL, m2, "M2");
      setMotorSpeed(MOTOR3_FORWARD_CHANNEL, MOTOR3_REVERSE_CHANNEL, m3, "M3");
      setMotorSpeed(MOTOR4_FORWARD_CHANNEL, MOTOR4_REVERSE_CHANNEL, m4, "M4");
      
      // Debug: Tampilkan data yang diterima
      Serial.printf("Received: xLeft=%d, yLeft=%d, z=%d, btnA=%d, btnB=%d\n", 
                    data.xLeft, data.yLeft, data.z, data.btnA, data.btnB);
    }
  }
}
