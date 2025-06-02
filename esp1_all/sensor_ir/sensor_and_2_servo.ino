#include <ESP32Servo.h>

const int sensorPin = 15; // Pin sensor

Servo servo1;
Servo servo2;

#define SERVO1_PIN 18
#define SERVO2_PIN 19

int pos1 = 90;

bool pulseReady = true;
unsigned long lastTime = 0;
unsigned long currentTime;
float rpm = 0;

void setup() {
  pinMode(sensorPin, INPUT);
  Serial.begin(115200);

  servo1.attach(SERVO1_PIN);
  servo2.attach(21);
  servo1.write(90);
  servo2.write(90);
  Serial.println("Servo diset ke posisi 90° (tengah)");
  delay(1000);
}

void loop() {
  bool sensorState = digitalRead(sensorPin);

  if (sensorState == HIGH && pulseReady) {
    pulseReady = false;
    Serial.println("Sensor ke detect");
    servo1.write(180);
    servo2.write(0);
    delay(1000);
  }

  if (sensorState == LOW) {
    pulseReady = true;
    servo1.write(90);
    servo2.write(90);
  }
}
