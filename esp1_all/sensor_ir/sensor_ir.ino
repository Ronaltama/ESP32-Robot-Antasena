const int sensorPin = 15; // Pin sensor

bool pulseReady = true;
unsigned long lastTime = 0;
unsigned long currentTime;
float rpm = 0;

void setup() {
  pinMode(sensorPin, INPUT);
  Serial.begin(115200);
  delay(1000);
  Serial.println("=== RPM MONITOR (Rumus: 1/RPM = ms/1 rotasi) ===");
  Serial.println("Waktu(ms) | Delta(ms) | RPM");
  Serial.println("--------------------------------");
}

void loop() {
  bool sensorState = digitalRead(sensorPin);

  if (sensorState == HIGH && pulseReady) {
    currentTime = millis();
    unsigned long deltaTime = currentTime - lastTime;

    if (deltaTime > 0) {
      // Menggunakan rumus yang kamu sebut: 1/RPM = ms/1 → RPM = 60000 / deltaTime
      rpm = 60000.0 / deltaTime;
    }

    Serial.print(currentTime);
    Serial.print(" ms | ");

    Serial.print(deltaTime);
    Serial.print(" ms | ");

    Serial.print(rpm, 2);
    Serial.println(" RPM");

    lastTime = currentTime;
    pulseReady = false;
  }

  if (sensorState == LOW) {
    pulseReady = true;
  }
}
