#include <WiFi.h>
#include <Bluepad32.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>


// Definisi Pin Motor
#define RPWM_PIN1 18
#define LPWM_PIN1 19
#define RPWM_PIN2 32
#define LPWM_PIN2 33
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

// Pin Relay
#define RELAY1 4
#define RELAY2 5
#define RELAY3 13
#define RELAY4 14
#define RELAY5 15
#define RELAY6 2


// Definisi IR Sensor Motor
#define IR_SENSOR_PIN 36
#define IR_SENSOR_PIN 39
#define IR_SENSOR_PIN 34
#define IR_SENSOR_PIN 35


#define LEDC_FREQ        5000  
#define LEDC_RESOLUTION  8

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(); // default address 0x40

GamepadPtr myGamepad = nullptr;

// Variabel akselerasi & deadzone joystick
float current_acc = 1.0;
const float ACC_STEP = 0.1;  // Langkah perubahan akselerasi
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

// Fungsi konversi PWM motor (0-255) ke PCA pulse (0-4095)
int pwmMotorToPulse(int pwmVal) {
  return map(pwmVal, 0, 255, 0, 4095);
}

// Fungsi konversi derajat ke pulse servo
int angleToPulse(int angle) {
  return map(angle, 0, 180, 150, 600); // kalibrasi sesuai kebutuhan
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
    pinMode(RELAY5, OUTPUT);
    pinMode(RELAY6, OUTPUT);


    // Inisialisasi Bluetooth Gamepad
    BP32.setup(&onConnectedGamepad, &onDisconnectedGamepad);

    Wire.begin(21, 22); // SDA, SCL

    pwm.begin();
    pwm.setPWMFreq(50);  // Servo pakai 50Hz
    delay(10);
}

void loop() {
    BP32.update();

    if (myGamepad && myGamepad->isConnected()) {
        // Joystick kiri
        int xLeft = applyDeadzone(-myGamepad->axisX());
        int yLeft = applyDeadzone(-myGamepad->axisY() + 4);

        // Joystick kanan
        int z = applyDeadzone(myGamepad->axisRX()/4);
 
        // Tombol R2 dan L2
        float r2 = (double)myGamepad->brake() / 1020; 
        float l2 = (double)myGamepad->throttle() / 680;

        // Tombol R1 dan L1
        int r1 = myGamepad->r1(); // R1
        int l1 = myGamepad->l1();  // L1

        // Tombol X, Y, A, B
        int tombol_a = myGamepad->a(); // bawah
        int tombol_b = myGamepad->b(); // kanan
        int tombol_c = myGamepad->y(); // atas
        int tombol_d = myGamepad->x(); // kiri

        // D pad
        int dpad = myGamepad->dpad(); // 1 atas, 2 bawah, 4 kanan, 8 kiri

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

        // Kecepatan motor jika memakai dpad
        if (dpad > 0){
          if (dpad == 1 || dpad == 9 || dpad == 5){ // atas
            m1 = 125 * current_acc;
            m2 = -125 * current_acc;
            m3 = 125 * current_acc;
            m4 = -125 * current_acc;
          } else if (dpad == 2 || dpad == 6 || dpad == 10){ // bawah
            m1 = -125 * current_acc;
            m2 = 125 * current_acc;
            m3 = -125 * current_acc;
            m4 = 125 * current_acc;
          } 
        }

      // Program Pelontar 
        if (tombol_a == 1){
          // definisi kecepatan melontar
          int motorSpeed1 = 150; // 0-255
          int motorSpeed2 = 150; // 0–255
          int pulseSpeed1 = pwmMotorToPulse(motorSpeed1);
          int pulseSpeed2 = pwmMotorToPulse(motorSpeed2);

          // pelontar nyala
          pwm.setPWM(4, 0, pulseSpeed1); // RPWM motor 1 , nyala
          pwm.setPWM(5, 0, pulseSpeed2); // LPWM motor 2 , nyala
          delay(1000);
          digitalWrite(RELAY1,HIGH);
          delay(3000);
          // pelontar mati
          pwm.setPWM(4, 0, 0); // RPWM motor 1 , mati
          pwm.setPWM(5, 0, 0); // LPWM motor 2 , mati
          digitalWrite(RELAY1,LOW);
        }

      // Program Transform 
      if (tombol_b == 1) {
        digitalWrite(RELAY2, HIGH);
      } 

      // Program Ambil Bola
      if (tombol_c) {
        int pulse = angleToPulse(90); // posisi tengah
        
        pwm.setPWM(0, 0, pulse);          // Servo1
        pwm.setPWM(1, pulse, 0);          // Servo2
        pwm.setPWM(2, 0, pulse);          // Servo3
        pwm.setPWM(3, pulse, 0);          // Servo4

        delay(500);

        digitalWrite(RELAY3,HIGH);
        digitalWrite(RELAY4,HIGH);
        digitalWrite(RELAY5,HIGH);
        digitalWrite(RELAY6,HIGH);

        delay(3000);

        pwm.setPWM(0, pulse, 0);          // Servo1
        pwm.setPWM(1, 0, pulse);          // Servo2
        pwm.setPWM(2, pulse, 0);          // Servo3
        pwm.setPWM(3, 0, pulse);          // Servo4

        delay(500);

        digitalWrite(RELAY3,LOW);
        digitalWrite(RELAY4,LOW);
        digitalWrite(RELAY5,LOW);
        digitalWrite(RELAY6,LOW);
      
      }

        // Set motor berdasarkan nilai yang dihitung
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
