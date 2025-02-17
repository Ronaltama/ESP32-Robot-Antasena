#include <WiFi.h>
#include <esp_now.h>

// Pin motor
const int motorPins[8] = {0, 1, 2, 3, 4, 5, 6, 7};
const int pwmChannels[8] = {0, 1, 2, 3, 4, 5, 6, 7};

// Struktur data joystick (binary)
typedef struct {
    int xLeft;
    int yLeft;
    int z;
    int btnA;
    int btnB;
} JoystickData;

void setMotorSpeed(int pin_r, int pin_l, int speed, const char* motorName) {
    int pwmValue = abs(speed);
    bool maju = (speed > 0);

    if (speed == 0) {
        ledcWrite(pin_r, 0);
        ledcWrite(pin_l, 0);
        Serial.printf("%s: STOP\n", motorName);
    } else {
        ledcWrite(pin_r, maju ? pwmValue : 0);
        ledcWrite(pin_l, maju ? 0 : pwmValue);
        Serial.printf("%s: %s dengan PWM %d\n", motorName, maju ? "MAJU" : "MUNDUR", pwmValue);
    }
}

void onReceiveData(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
    Serial.print("Data received from: ");
    for (int i = 0; i < 6; i++) {
        Serial.printf("%02X", info->src_addr[i]);
        if (i < 5) Serial.print(":");
    }
    Serial.println();
    
    if(len != sizeof(JoystickData)){
        Serial.println("Data length mismatch!");
        return;
    }
    
    JoystickData jd;
    memcpy(&jd, data, sizeof(jd));
    
    Serial.printf("xLeft: %d | yLeft: %d | z: %d | A: %d | B: %d\n", jd.xLeft, jd.yLeft, jd.z, jd.btnA, jd.btnB);
    
    // Hitung kecepatan motor
    int m1 = jd.xLeft + jd.yLeft + jd.z;
    int m2 = jd.xLeft - jd.yLeft + jd.z;
    int m3 = -jd.xLeft + jd.yLeft + jd.z;
    int m4 = -jd.xLeft - jd.yLeft + jd.z;

    m1 = constrain(m1, -255, 255);
    m2 = constrain(m2, -255, 255);
    m3 = constrain(m3, -255, 255);
    m4 = constrain(m4, -255, 255);

    Serial.printf("M1: %d | M2: %d | M3: %d | M4: %d\n", m1, m2, m3, m4);

    setMotorSpeed(motorPins[0], motorPins[1], m1, "M1");
    setMotorSpeed(motorPins[2], motorPins[3], m2, "M2");
    setMotorSpeed(motorPins[4], motorPins[5], m3, "M3");
    setMotorSpeed(motorPins[6], motorPins[7], m4, "M4");
}

void setup() {
    Serial.begin(115200);
    
    WiFi.mode(WIFI_STA);
    if (esp_now_init() != ESP_OK) {
        Serial.println("Error initializing ESP-NOW");
        return;
    }
    esp_now_register_recv_cb(onReceiveData);
}

void loop() {
}
