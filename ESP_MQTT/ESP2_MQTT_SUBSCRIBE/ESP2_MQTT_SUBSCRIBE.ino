#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

const char* ssid = "my_technology";
const char* password = "35k4nu54marina";
const char* mqtt_server = "192.168.0.104";
const int mqttPort = 1883;
const char* mqttUser = "Antasena";
const char* mqttPassword = "12345678";

WiFiClient espClient;
PubSubClient client(espClient);

// Pin motor
const int motorPins[8] = {0, 1, 2, 3, 4, 5, 6, 7};
const int pwmChannels[8] = {0, 1, 2, 3, 4, 5, 6, 7}; // Channel PWM motor

void callback(char* topic, byte* payload, unsigned int length) {
    Serial.print("Pesan diterima: ");
    Serial.println(topic);

    // Parsing JSON
    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, payload, length);
    
    if (error) {
        Serial.print("JSON Parsing failed: ");
        Serial.println(error.f_str());
        return;
    }

    int xLeft = doc["xLeft"];
    int yLeft = doc["yLeft"];
    int z = doc["z"];
    int A = doc["A"];
    int B = doc["B"];

    Serial.printf("xLeft: %d | yLeft: %d | z: %d | A: %d | B: %d\n", xLeft, yLeft, z, A, B);

    // Hitung kecepatan motor
    int m1 = xLeft + yLeft + z;
    int m2 = xLeft - yLeft + z;
    int m3 = -xLeft + yLeft + z;
    int m4 = -xLeft - yLeft + z;

    // Batasi nilai agar tetap dalam rentang -255 sampai 255
    m1 = constrain(m1, -255, 255);
    m2 = constrain(m2, -255, 255);
    m3 = constrain(m3, -255, 255);
    m4 = constrain(m4, -255, 255);

    Serial.printf("M1: %d | M2: %d | M3: %d | M4: %d\n", m1, m2, m3, m4);

    // Terapkan kecepatan ke motor
    setMotorSpeed(motorPins[0], motorPins[1], m1, "M1");
    setMotorSpeed(motorPins[2], motorPins[3], m2, "M2");
    setMotorSpeed(motorPins[4], motorPins[5], m3, "M3");
    setMotorSpeed(motorPins[6], motorPins[7], m4, "M4");
}

// Fungsi untuk mengontrol motor + debugging
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

void setup() {
    Serial.begin(115200);

    // Koneksi Wi-Fi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWi-Fi Connected!");

    // Setup MQTT
    client.setServer(mqtt_server, mqttPort);
    client.setCallback(callback);
    reconnect();

    // // Setup PWM untuk motor
    // for (int i = 0; i < 8; i++) {
    //     ledcSetup(pwmChannels[i], 1000, 8);   // Setup PWM tiap channel (1000 Hz, 8-bit)
    //     ledcAttachPin(motorPins[i], pwmChannels[i]); // Attach pin ke channel
    // }
}

void reconnect() {
    while (!client.connected()) {
        Serial.print("Connecting to MQTT...");
        if (client.connect("ESP32_Motor", mqttUser, mqttPassword)) {
            Serial.println("Connected!");
            client.subscribe("joystick/data");
        } else {
            Serial.print("Failed, rc=");
            Serial.print(client.state());
            Serial.println(" Retrying in 5 seconds...");
            delay(5000);
        }
    }
}

void loop() {
    if (!client.connected()) {
        reconnect();
    }
    client.loop();
}
