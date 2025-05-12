// Import semua library
#include <WiFi.h>
#include <Bluepad32.h>
#include <Wire.h>
#include <ESP32Servo.h>

// Definisi Pin Motor (Bawah)
#define RPWM_PIN1 18
#define LPWM_PIN1 19
#define RPWM_PIN2 32
#define LPWM_PIN2 33
#define RPWM_PIN3 23
#define LPWM_PIN3 25
#define RPWM_PIN4 26
#define LPWM_PIN4 27

// Definisi Channel LEDC untuk PWM (Motor Bawah)
#define LEDC_CHANNEL_1A  0  
#define LEDC_CHANNEL_1B  1  
#define LEDC_CHANNEL_2A  2  
#define LEDC_CHANNEL_2B  3  
#define LEDC_CHANNEL_3A  4  
#define LEDC_CHANNEL_3B  5  
#define LEDC_CHANNEL_4A  6  
#define LEDC_CHANNEL_4B  7  


// Pin Relay
#define RELAY1 14
#define RELAY2 15

// Create servo
Servo servo1;
Servo servo2;

// Pelontar
#define RPWM_PIN5 21
#define LPWM_PIN6 22


// Definisi konstanta untuk LEDC (Motor Bawah)
#define LEDC_FREQ        5000  
#define LEDC_RESOLUTION  8



// Buat Gamepad
GamepadPtr myGamepad = nullptr;

// Variabel akselerasi & deadzone joystick
float current_acc = 1.0; // Akselerasi motor bawah (1 = normal, 2.5 = Cepet, 0 = Stop)
const float ACC_STEP = 0.1;  // Langkah perubahan akselerasi (Biar ga langsung berhenti)
#define DEADZONE 10  // Deadzone pada gamepad (kalau kurang dari 10 maka dianggep 0)

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

// Fungsi untuk mengontrol motor
void setMotorSpeed(int channel_r, int channel_l, int speed) {
    int pwmValue = abs(speed);
    bool maju = (speed > 0);
    ledcWrite(channel_r, maju ? pwmValue : 0);
    ledcWrite(channel_l, maju ? 0 : pwmValue);
}

// Fungsi untuk menerapkan deadzone
int applyDeadzone(int value) {
    return (abs(value) < DEADZONE) ? 0 : value;
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

    // Setup relay
    pinMode(RELAY1, OUTPUT);
    pinMode(RELAY2, OUTPUT);


    // Setup servo
    servo1.attach(4); 
    servo2.attach(13);

    // Inisialisasi Bluetooth Gamepad
    BP32.setup(&onConnectedGamepad, &onDisconnectedGamepad);

    // Motor Pelontar
    pinMode(RPWM_PIN5, OUTPUT);
    pinMode(LPWM_PIN6, OUTPUT);
}

void loop() {
    BP32.update();

    if (myGamepad && myGamepad->isConnected()) { // Hanya berjalan saat gamepad connect
        // Joystick kiri
        int xLeft = applyDeadzone(-myGamepad->axisX()); // Gerak kanan kiri
        int yLeft = applyDeadzone(-myGamepad->axisY() + 4); // Gerak depan belakang

        // Joystick kanan
        int z = applyDeadzone(myGamepad->axisRX()/4); // Buat rotasi
 
        // Tombol R2 dan L2
        float r2 = (double)myGamepad->brake() / 1020; // Rem
        float l2 = (double)myGamepad->throttle() / 680; // Gas

        // Tombol R1 dan L1
        int r1 = myGamepad->r1(); // Kecepatan setengah
        int l1 = myGamepad->l1();  // Berhenti langsung

        // Tombol X, Y, A, B
        int tombol_a = myGamepad->a(); // bawah         Y
        int tombol_b = myGamepad->b(); // kanan       X   B
        int tombol_c = myGamepad->y(); // atas          A
        int tombol_d = myGamepad->x(); // kiri

        // D pad
        int dpad = myGamepad->dpad(); // 1 atas, 2 bawah, 4 kanan, 8 kiri

        // Menghitung perubahan kecepatan
        float target_acc = 1 - r2 + l2; 
        target_acc = constrain(target_acc, 0, 2.04);

        // Jika L1 ditekan, langsung berhenti (current_acc = 0)
        if (l1 > 0) {
            current_acc = 0;
        }
        // Jika R1 ditekan, set current_acc ke 0.5 (kecepatan setengah)
        else if (r1 > 0) {
            current_acc = 0.5;
        }
        // Jika tidak ditekan, gunakan akselerasi normal
        else {
            if (target_acc > current_acc) {
                current_acc += ACC_STEP;
                if (current_acc > target_acc)
                    current_acc = target_acc;
            } else if (target_acc < current_acc) {
                current_acc -= ACC_STEP;
                if (current_acc <= target_acc)
                    current_acc = 0 ;
            }
        }

        // Hitung kecepatan motor dengan menggunakan current_acc
        int m1 = constrain(xLeft + yLeft + z, -125, 125) * current_acc;
        int m2 = constrain(xLeft - yLeft + z, -125, 125) * current_acc;
        int m3 = constrain(-xLeft + yLeft + z, -125, 125) * current_acc;
        int m4 = constrain(-xLeft - yLeft + z, -125, 125) * current_acc;

        // Kecepatan motor jika memakai dpad (Belum dicoba arahnya udah bener atau salah)
        if (dpad > 0){
          if (dpad == 1 || dpad == 9 || dpad == 5){ // maju
            m1 = 125 * current_acc;
            m2 = -125 * current_acc;
            m3 = 125 * current_acc;
            m4 = -125 * current_acc;
          } else if (dpad == 2 || dpad == 6 || dpad == 10){ // mundur
            m1 = -125 * current_acc;
            m2 = 125 * current_acc;
            m3 = -125 * current_acc;
            m4 = 125 * current_acc;
          } 
        }


        // Driblling
        if (tombol_a == 1){ 

        servo1.write(90);
        servo2.write(0);
        digitalWrite(RELAY1, HIGH);
        delay(100);
        digitalWrite(RELAY1, LOW);
        delay(2000);
        servo1.write(0);
        servo2.write(90);
        }

        // Passing Kenceng
        if (tombol_b == 1) {
        analogWrite(RPWM_PIN5, 255);
        analogWrite(LPWM_PIN6, 255);
        delay(500);
        digitalWrite(RELAY2, HIGH);

        delay(2000);
        digitalWrite(RELAY2, LOW);
        analogWrite(RPWM_PIN5, 0);
        analogWrite(LPWM_PIN6, 0);
        } 

        // Passing pelan
        if (tombol_c) { 
        analogWrite(RPWM_PIN5, 125);
        analogWrite(LPWM_PIN6, 125);
        delay(500);
        digitalWrite(RELAY2, HIGH);

        delay(2000);
        digitalWrite(RELAY2, LOW);
        analogWrite(RPWM_PIN5, 0);
        analogWrite(LPWM_PIN6, 0);
        }


        // Set motor bawah berdasarkan nilai yang dihitung
        setMotorSpeed(LEDC_CHANNEL_1A, LEDC_CHANNEL_1B, m1);
        setMotorSpeed(LEDC_CHANNEL_2A, LEDC_CHANNEL_2B, m2);
        setMotorSpeed(LEDC_CHANNEL_3A, LEDC_CHANNEL_3B, m3);
        setMotorSpeed(LEDC_CHANNEL_4A, LEDC_CHANNEL_4B, m4);

        // Debug output
        Serial.printf("xLeft: %d | yLeft: %d | z: %d\n", xLeft, yLeft, z);
        Serial.printf("M1: %d | M2: %d | M3: %d | M4: %d\n", m1, m2, m3, m4);
        Serial.printf("target_acc: %.2f | current_acc: %.2f \n", target_acc, current_acc);
        Serial.printf("R1: %d | R2: %d | L1: %d | L2: %d \n", r1, r2, l1, l2);
        Serial.printf("A: %d | B: %d | C: %d | D: %d \n", tombol_a % 2, tombol_b % 2, tombol_c % 2, tombol_d % 2);
        Serial.printf("DPAD: %d \n\n", dpad);
    } else {
        // Jika gamepad terputus, hentikan semua motor
        setMotorSpeed(LEDC_CHANNEL_1A, LEDC_CHANNEL_1B, 0);
        setMotorSpeed(LEDC_CHANNEL_2A, LEDC_CHANNEL_2B, 0);
        setMotorSpeed(LEDC_CHANNEL_3A, LEDC_CHANNEL_3B, 0);
        setMotorSpeed(LEDC_CHANNEL_4A, LEDC_CHANNEL_4B, 0);
    }
} 
