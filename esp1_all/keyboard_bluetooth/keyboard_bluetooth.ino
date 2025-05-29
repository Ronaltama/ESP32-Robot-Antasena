#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include "BluetoothSerial.h"

// Inisialisasi objek
BluetoothSerial SerialBT;
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

// Fungsi konversi derajat ke pulse servo
int angleToPulse(int angle) {
  return map(angle, 0, 180, 150, 600); // kalibrasi sesuai kebutuhan
}

void setup() {
  Serial.begin(115200);
  
  Wire.begin(21, 22);

  pwm.begin();
  pwm.setPWMFreq(50);
  delay(10);
  
  SerialBT.begin("ESP32_ServoControl");
  Serial.println("ESP32 Servo Control Ready!");
  Serial.println("Bluetooth device name: ESP32_ServoControl");
  Serial.println("Pair your keyboard and start controlling servos!");
  
  delay(1000);
  printInstructions();
}

void loop() {
  if (SerialBT.available()) {
    char key = SerialBT.read();
    processKeyInput(key);
  }
  
  if (Serial.available()) {
    char key = Serial.read();
    processKeyInput(key);
  }
  
  delay(50);
}

void processKeyInput(char key) {
  bool moved = false;

  switch (key) {
    case 'w':
    case 'W': {
      int pulse = angleToPulse(90); // posisi tengah
      pwm.setPWM(4, 0, pulse);          // Servo1
      pwm.setPWM(5, 0, pulse);          // Servo2
      delay(200);

      Serial.println("W");
      moved = true;
      break;
    }

    case 's':
    case 'S': {
      int pulse = angleToPulse(180); // posisi tengah
      pwm.setPWM(4, 0, pulse);          // Servo1
      pwm.setPWM(5, 0, pulse);          // Servo2
      delay(200);

      Serial.println("S");
      moved = true;
      break;
    }

    default:
      // Tidak ada aksi
      break;
  }

  if (moved) {
    Serial.println("Moved");
  }
}

void printInstructions() {
  Serial.println("\n=== KONTROL SERVO ===");
  Serial.println("W/S: Servo 1 (Channel 4) - Naik/Turun");
  Serial.println("A/D: Servo 2 (Channel 5) - Kiri/Kanan"); 
  Serial.println("R: Reset kedua servo ke 90°");
  Serial.println("+/-: Ubah ukuran langkah");
  Serial.println("H: Tampilkan bantuan");
  Serial.println("==================\n");
}
