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

// Pin Relay
#define RELAY1 32
#define RELAY2 33
#define RELAY3 27
#define RELAY4 14

//Define tombol
int tombol_a = 0; // bawah
int tombol_b = 0; // kanan
int tombol_c = 0; // atas
int tombol_d = 0; // kiri
int delayTombol = 0;

#define LEDC_FREQ        5000  
#define LEDC_RESOLUTION  8

GamepadPtr myGamepad = nullptr;

// Variabel akselerasi & deadzone joystick
float current_acc = 1.0;
const float ACC_STEP = 0.05;  // Langkah perubahan akselerasi
#define DEADZONE 10  

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
    pinMode(RELAY3, OUTPUT);
    pinMode(RELAY4, OUTPUT);

    // Inisialisasi Bluetooth Gamepad
    BP32.setup(&onConnectedGamepad, &onDisconnectedGamepad);
}

void loop() {
    BP32.update();

    if (myGamepad && myGamepad->isConnected()) {
        // Joystick kiri
        int xLeft = applyDeadzone(-myGamepad->axisX());
        int yLeft = applyDeadzone(-myGamepad->axisY() + 4);

        // Joystick kanan
        int z = applyDeadzone(myGamepad->axisRX()/2);

        // Tombol R2 dan L2
        float r2 = (double)myGamepad->brake() / 1020; 
        float l2 = (double)myGamepad->throttle() / 680;

        // Tombol R1 dan L1
        int r1 = myGamepad->r1(); // R1
        int l1 = myGamepad->l1();  // L1

        // Tombol X, Y, A, B jadi saklar (flip flop T)
        if (delayTombol <= 0){
          tombol_a += myGamepad->a(); // bawah
          tombol_b += myGamepad->b(); // kanan
          tombol_c += myGamepad->y(); // atas
          tombol_d += myGamepad->x(); // kiri
          delayTombol += (myGamepad->a() * 100) + (myGamepad->b() * 100) + (myGamepad->y() * 100) + (myGamepad->x() * 100);
        } else if (delayTombol > 0) {
          delayTombol--;
        }

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

        // Set motor berdasarkan nilai yang dihitung
        setMotorSpeed(LEDC_CHANNEL_1A, LEDC_CHANNEL_1B, m1);
        setMotorSpeed(LEDC_CHANNEL_2A, LEDC_CHANNEL_2B, m2);
        setMotorSpeed(LEDC_CHANNEL_3A, LEDC_CHANNEL_3B, m3);
        setMotorSpeed(LEDC_CHANNEL_4A, LEDC_CHANNEL_4B, m4);

        //Relay Code
        digitalWrite(RELAY1, tombol_a % 2 == 1 ? LOW : HIGH);
        digitalWrite(RELAY2, tombol_a % 2 == 1 ? LOW : HIGH);
        digitalWrite(RELAY3, tombol_b % 2 == 1 ? LOW : HIGH);
        digitalWrite(RELAY4, tombol_d % 2 == 1 ? LOW : HIGH);

        // Debug output
        Serial.printf("xLeft: %d | yLeft: %d | z: %d\n", xLeft, yLeft, z);
        Serial.printf("M1: %d | M2: %d | M3: %d | M4: %d\n", m1, m2, m3, m4);
        Serial.printf("target_acc: %.2f | current_acc: %.2f \n", target_acc, current_acc);
        Serial.printf("R1: %d | R2: %d | L1: %d | L2: %d \n", r1, r2, l1, l2);
        Serial.printf("A: %d | B: %d | C: %d | D: %d \n\n", tombol_a, tombol_b, tombol_c, tombol_d);
    } else {
        // Jika gamepad terputus, hentikan semua motor
        setMotorSpeed(LEDC_CHANNEL_1A, LEDC_CHANNEL_1B, 0);
        setMotorSpeed(LEDC_CHANNEL_2A, LEDC_CHANNEL_2B, 0);
        setMotorSpeed(LEDC_CHANNEL_3A, LEDC_CHANNEL_3B, 0);
        setMotorSpeed(LEDC_CHANNEL_4A, LEDC_CHANNEL_4B, 0);
    }
}
