#include <WiFi.h>

void setup() {
  Serial.begin(115200);
  delay(1000);  // Beri waktu untuk inisialisasi
  WiFi.mode(WIFI_STA);  // Inisialisasi WiFi sebagai Station

  Serial.println();
  Serial.print("MAC Address: ");
  Serial.println(WiFi.macAddress());
}

void loop() {
  // Tidak diperlukan
}
