#include <Bluepad32.h>
#include <WiFi.h>
#include <PubSubClient.h>

const char* ssid = "my_technology";
const char* password = "35k4nu54marina";
const char* mqtt_server = "192.168.0.104";
const int mqttPort = 1883;
const char* mqttUser = "Antasena";
const char* mqttPassword = "12345678";

WiFiClient espClient;
PubSubClient client(espClient);

GamepadPtr myGamepad = nullptr;

// Callback saat gamepad terhubung
void onConnectedGamepad(GamepadPtr gp) {
    Serial.println("Gamepad connected!");
    myGamepad = gp;
}

// Callback saat gamepad terputus
void onDisconnectedGamepad(GamepadPtr gp) {
    Serial.println("Gamepad disconnected!");
    myGamepad = nullptr;
}

void setup() {
    Serial.begin(115200);

    Serial.print("Connecting to Wi-Fi...");
    WiFi.begin(ssid, password);
    int attempt = 0;
    while (WiFi.status() != WL_CONNECTED && attempt < 20) {
        delay(500);
        Serial.print(".");
        attempt++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWi-Fi Connected!");
    } else {
        Serial.println("\nWi-Fi Connection Failed!");
        return;
    }

    client.setServer(mqtt_server, mqttPort);
    reconnect();

    BP32.setup(&onConnectedGamepad, &onDisconnectedGamepad);
    BP32.forgetBluetoothKeys();
}

void reconnect() {
    while (!client.connected()) {
        Serial.print("Connecting to MQTT...");
        if (client.connect("ESP32_Gamepad", mqttUser, mqttPassword)) {
            Serial.println("Connected!");
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

    BP32.update();

    if (myGamepad && myGamepad->isConnected()) {
        int xLeft = myGamepad->axisX();
        int yLeft = -myGamepad->axisY();
        int z = myGamepad->axisRX();
        int btnA = myGamepad->a() ? 1 : 0;
        int btnB = myGamepad->b() ? 1 : 0;

        if (client.connected()) {
            char payload[100];
            snprintf(payload, sizeof(payload), 
                     "{\"xLeft\":%d, \"yLeft\":%d, \"z\":%d, \"A\":%d, \"B\":%d}", 
                     xLeft, yLeft, z, btnA, btnB);
            
            client.publish("joystick/data", payload);
            Serial.println(payload);
        }
    }

    delay(100);
}
