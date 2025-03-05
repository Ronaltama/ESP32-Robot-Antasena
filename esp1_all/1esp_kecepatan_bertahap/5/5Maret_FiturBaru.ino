#include <WiFi.h>
#include <Bluepad32.h>

// Definisi Pin Motor
#define RPWM_PIN1 23
#define LPWM_PIN1 25
#define RPWM_PIN2 18
#define LPWM_PIN2 19
#define RPWM_PIN3 14
#define LPWM_PIN3 12
#define RPWM_PIN4 21
#define LPWM_PIN4 22

// Definisi Channel LEDC untuk PWM
#define LEDC_CHANNEL_1A  0  
#define LEDC_CHANNEL_1B  1  
#define LEDC_CHANNEL_2A  2  
#define LEDC_CHANNEL_2B  3  
#define LEDC_CHANNEL_3A  4  
#define LEDC_CHANNEL_3B  5  
#define LEDC_CHANNEL_4A  6  
#define LEDC_CHANNEL_4B  7  

#define LEDC_FREQ        5000  
#define LEDC_RESOLUTION  8

GamepadPtr myGamepad = nullptr;

// Variabel untuk smoothing akselerasi
float current_acc = 1.0;
const float ACC_STEP = 0.05;  // Langkah perubahan akselerasi

// Callback saat gamepad terhubung
void onConnectedGamepad(GamepadPtr gp) {
    myGamepad = gp;
    Serial.println("Gamepad Connected!");
}

// Callback saat gamepad terputus
void onDisconnectedGamepad(GamepadPtr gp) {
    myGamepad = nullptr;
    Serial.println("Gamepad Disconnected!");
}

void setMotorSpeed(int channel_r, int channel_l, int speed) {
    int pwmValue = abs(speed);
    bool maju = (speed > 0);
    ledcWrite(channel_r, maju ? pwmValue : 0);
    ledcWrite(channel_l, maju ? 0 : pwmValue);
}

void setup() {
    Serial.begin(115200);

    // Inisialisasi PWM
    for (int i = 0; i < 8; i++) {
        ledcSetup(i, LEDC_FREQ, LEDC_RESOLUTION);
    }

    // Attach GPIO ke channel LEDC
    ledcAttachPin(RPWM_PIN1, LEDC_CHANNEL_1A);
    ledcAttachPin(LPWM_PIN1, LEDC_CHANNEL_1B);
    ledcAttachPin(RPWM_PIN2, LEDC_CHANNEL_2A);
    ledcAttachPin(LPWM_PIN2, LEDC_CHANNEL_2B);
    ledcAttachPin(RPWM_PIN3, LEDC_CHANNEL_3A);
    ledcAttachPin(LPWM_PIN3, LEDC_CHANNEL_3B);
    ledcAttachPin(RPWM_PIN4, LEDC_CHANNEL_4A);
    ledcAttachPin(LPWM_PIN4, LEDC_CHANNEL_4B);

    // Inisialisasi Bluetooth Gamepad
    BP32.setup(&onConnectedGamepad, &onDisconnectedGamepad);
}

void loop() {
    BP32.update();

    if (myGamepad && myGamepad->isConnected()) {
        // Jika tombol L1 ditekan, lakukan rem mendadak (stop instan)
        if (myGamepad->l1()) {
            current_acc = 0;  // Langsung set kecepatan ke 0
            setMotorSpeed(LEDC_CHANNEL_1A, LEDC_CHANNEL_1B, 0);
            setMotorSpeed(LEDC_CHANNEL_2A, LEDC_CHANNEL_2B, 0);
            setMotorSpeed(LEDC_CHANNEL_3A, LEDC_CHANNEL_3B, 0);
            setMotorSpeed(LEDC_CHANNEL_4A, LEDC_CHANNEL_4B, 0);
            
        }
        else {
            // Jika tombol R1 ditekan, set faktor kecepatan menjadi 0.5 (setengah)
            float speedFactor = 1.0;
            if (myGamepad->r1()) {
                speedFactor = 0.5;
            }

            int xLeft = -myGamepad->axisX();
            int yLeft = -myGamepad->axisY() + 4;
            int z = myGamepad->axisRX();
            
            float r2 = (double)myGamepad->brake() / 1020; 
            float l2 = (double)myGamepad->throttle() / 680;

            // Hitung target akselerasi
            float target_acc = 1 - r2 + l2; 
            target_acc = constrain(target_acc, 0, 2.5);  // Batas percepatan agar tidak terlalu ekstrem

            // Smoothing akselerasi (ubah ke target_acc secara bertahap)
            if (target_acc > current_acc) {
                current_acc += ACC_STEP;
                if (current_acc > target_acc)
                    current_acc = target_acc;
            } else if (target_acc < current_acc) {
                current_acc -= ACC_STEP;
                if (current_acc <= target_acc)
                    current_acc = target_acc;  // perbaiki supaya tidak langsung 0
            }

            // Hitung kecepatan motor dengan menggunakan current_acc dan faktor kecepatan
            int m1 = constrain(xLeft + yLeft + z, -125, 125) * current_acc * speedFactor;
            int m2 = constrain(xLeft - yLeft + z, -125, 125) * current_acc * speedFactor;
            int m3 = constrain(-xLeft + yLeft + z, -125, 125) * current_acc * speedFactor;
            int m4 = constrain(-xLeft - yLeft + z, -125, 125) * current_acc * speedFactor;

            // Set motor berdasarkan nilai yang dihitung
            setMotorSpeed(LEDC_CHANNEL_1A, LEDC_CHANNEL_1B, m1);
            setMotorSpeed(LEDC_CHANNEL_2A, LEDC_CHANNEL_2B, m2);
            setMotorSpeed(LEDC_CHANNEL_3A, LEDC_CHANNEL_3B, m3);
            setMotorSpeed(LEDC_CHANNEL_4A, LEDC_CHANNEL_4B, m4);

            // Debug output
            Serial.printf("xLeft: %d | yLeft: %d | z: %d\n", xLeft, yLeft, z);
            Serial.printf("M1: %d | M2: %d | M3: %d | M4: %d\n", m1, m2, m3, m4);
            Serial.printf("target_acc: %.2f | current_acc: %.2f | speedFactor: %.2f\n", target_acc, current_acc, speedFactor);
        }
    }
}
