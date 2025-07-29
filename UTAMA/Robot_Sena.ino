#include <Bluepad32.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

// =================== Pin dan Konstanta ===================

// Motor Roda (Omni/Mecanum)
#define RPWM_PIN1 18
#define LPWM_PIN1 19
#define RPWM_PIN2 32
#define LPWM_PIN2 33
#define RPWM_PIN3 23
#define LPWM_PIN3 25
#define RPWM_PIN4 26
#define LPWM_PIN4 27

// Channel PWM Roda
#define LEDC_CHANNEL_1A  0
#define LEDC_CHANNEL_1B  1
#define LEDC_CHANNEL_2A  2
#define LEDC_CHANNEL_2B  3
#define LEDC_CHANNEL_3A  4
#define LEDC_CHANNEL_3B  5
#define LEDC_CHANNEL_4A  6
#define LEDC_CHANNEL_4B  7

// Pin untuk Sistem Passing
#define RELAY_PASSING 17
#define PASSING_MOTOR_1_PIN 0 // Pin pada PWM Extender untuk motor passing 1
#define PASSING_MOTOR_2_PIN 1 // Pin pada PWM Extender untuk motor passing 2

// Pengaturan PWM dan Gamepad
#define LEDC_FREQ        5000
#define LEDC_RESOLUTION  8
#define DEADZONE 40

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();
GamepadPtr myGamepad = nullptr;

// Variabel Akselerasi
float current_acc = 1.0;
const float ACC_STEP = 0.1;

// Variabel untuk State Machine Passing (Non-Blocking)
bool isPassing = false;
unsigned long passStartTime = 0;
int passState = 0; // 0:idle, 1:motor nyala, 2:solenoid dorong, 3:solenoid kembali & motor mati

// =================== Callback Gamepad ===================
void onConnectedGamepad(GamepadPtr gp) {
    myGamepad = gp;
    Serial.println("Gamepad Terhubung!");
}

void onDisconnectedGamepad(GamepadPtr gp) {
    myGamepad = nullptr;
    Serial.println("Gamepad Terputus!");
}

// =================== Utilitas Motor ===================
void setMotorSpeed(int channel_r, int channel_l, int speed) {
    int pwmValue = abs(speed);
    bool maju = (speed > 0);
    ledcWrite(channel_r, maju ? pwmValue : 0);
    ledcWrite(channel_l, maju ? 0 : pwmValue);
}

int applyDeadzone(int value) {
    return (abs(value) < DEADZONE) ? 0 : value;
}

void stopAllBaseMotors() {
    for (int i = 0; i < 8; i++) ledcWrite(i, 0);
}

void stopPassingSystem() {
    digitalWrite(RELAY_PASSING, HIGH); // Pastikan relay mati
    pwm.setPWM(PASSING_MOTOR_1_PIN, 0, 0);
    pwm.setPWM(PASSING_MOTOR_2_PIN, 0, 0);
}

// =================== Setup ===================
void setup() {
    Serial.begin(115200);

    for (int i = 0; i < 8; i++) ledcSetup(i, LEDC_FREQ, LEDC_RESOLUTION);

    ledcAttachPin(RPWM_PIN1, LEDC_CHANNEL_1A);
    ledcAttachPin(LPWM_PIN1, LEDC_CHANNEL_1B);
    ledcAttachPin(RPWM_PIN2, LEDC_CHANNEL_2A);
    ledcAttachPin(LPWM_PIN2, LEDC_CHANNEL_2B);
    ledcAttachPin(RPWM_PIN3, LEDC_CHANNEL_3A);
    ledcAttachPin(LPWM_PIN3, LEDC_CHANNEL_3B);
    ledcAttachPin(RPWM_PIN4, LEDC_CHANNEL_4A);
    ledcAttachPin(LPWM_PIN4, LEDC_CHANNEL_4B);

    pinMode(RELAY_PASSING, OUTPUT);
    digitalWrite(RELAY_PASSING, HIGH); // Default relay mati

    BP32.setup(&onConnectedGamepad, &onDisconnectedGamepad);
    Wire.begin(21, 22);
    pwm.begin();
    pwm.setPWMFreq(60); // Frekuensi standar untuk ESC/motor
    delay(10);
}

// =================== Handler Aksi Non-Blocking ===================
void handlePassing() {
    if (!isPassing) return; // Jika tidak sedang passing, keluar dari fungsi

    unsigned long currentTime = millis();
    int passingMotorSpeed = 4095; // Kecepatan penuh (0-4095)

    // State 1: Nyalakan motor pelontar
    if (passState == 1) {
        Serial.println("Passing State 1: Motor Nyala");
        pwm.setPWM(PASSING_MOTOR_1_PIN, 0, passingMotorSpeed); // Motor 1 nyala
        pwm.setPWM(PASSING_MOTOR_2_PIN, passingMotorSpeed, 0); // Motor 2 nyala arah sebaliknya
        passState = 2; // Lanjut ke state berikutnya
    }
    // State 2: Setelah 500ms, dorong solenoid
    else if (passState == 2 && (currentTime - passStartTime > 500)) { // Tunggu 500ms untuk motor stabil
        Serial.println("Passing State 2: Solenoid Mendorong");
        digitalWrite(RELAY_PASSING, LOW); // Aktifkan relay
        passState = 3; // Lanjut ke state berikutnya
    }
    // State 3: Setelah 200ms mendorong, matikan semuanya
    else if (passState == 3 && (currentTime - passStartTime > 700)) { // Total waktu 500ms + 200ms
        Serial.println("Passing State 3: Selesai");
        stopPassingSystem();
        isPassing = false; // Selesai
        passState = 0;     // Reset state
    }
}

// =================== Loop Utama ===================
void loop() {
    BP32.update();

    if (myGamepad && myGamepad->isConnected()) {
        // Panggil handler aksi di setiap loop
        handlePassing();

        // Baca semua input gamepad
        int xLeft = applyDeadzone(-myGamepad->axisX());
        int yLeft = applyDeadzone(-myGamepad->axisY() + 4);
        int z = applyDeadzone(myGamepad->axisRX() / 4);
        
        float r2 = (float)myGamepad->brake() / 1023.0;
        float l2 = (float)myGamepad->throttle() / 1023.0;

        bool tombol_a = myGamepad->a();

        // Mulai proses passing jika tombol A ditekan dan tidak sedang passing
        if (tombol_a && !isPassing) {
            isPassing = true;
            passState = 1;
            passStartTime = millis();
        }

        // Hitung akselerasi/kecepatan
        float target_acc = constrain(1.0 - r2 + l2, 0.0, 2.0);
        if (target_acc > current_acc) {
            current_acc = min(target_acc, current_acc + ACC_STEP);
        } else if (target_acc < current_acc) {
            current_acc = max(0.0f, current_acc - ACC_STEP);
        }

        // Hitung kecepatan motor roda (omni/mecanum)
        int m1 = constrain(xLeft + yLeft + z, -127, 127) * current_acc;
        int m2 = constrain(-xLeft + yLeft + z, -127, 127) * current_acc;
        int m3 = constrain(xLeft - yLeft + z, -127, 127) * current_acc;
        int m4 = constrain(-xLeft + yLeft - z, -127, 127) * current_acc;

        // Atur kecepatan motor roda
        setMotorSpeed(LEDC_CHANNEL_1A, LEDC_CHANNEL_1B, m1);
        setMotorSpeed(LEDC_CHANNEL_2A, LEDC_CHANNEL_2B, m2);
        setMotorSpeed(LEDC_CHANNEL_3A, LEDC_CHANNEL_3B, m3);
        setMotorSpeed(LEDC_CHANNEL_4A, LEDC_CHANNEL_4B, m4);

    } else {
        // Jika gamepad terputus, matikan semuanya
        stopAllBaseMotors();
        stopPassingSystem();
        isPassing = false; // Pastikan state juga di-reset
        passState = 0;
    }
}
