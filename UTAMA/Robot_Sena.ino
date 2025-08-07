//////////////////////////////////////////////
//        RemoteXY include library          //
//////////////////////////////////////////////

// you can enable debug logging to Serial at 115200
//#define REMOTEXY__DEBUGLOG    

// RemoteXY select connection mode and include library 
#define REMOTEXY_MODE__ESP32CORE_BLUETOOTH
#include <BluetoothSerial.h>

// RemoteXY connection settings 
#define REMOTEXY_BLUETOOTH_NAME "Antasena"
#include <RemoteXY.h>

#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

// Definisi Pin Motor Bawah
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
#define RELAY1 16  // DRIBBLING
#define RELAY2 17  // PASSING

#define LEDC_FREQ        5000  
#define LEDC_RESOLUTION  8

#define DEADZONE 10

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(); // default address 0x40

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

// RemoteXY GUI configuration  
#pragma pack(push, 1)  
uint8_t RemoteXY_CONF[] =   // 111 bytes
  { 255,6,0,4,0,104,0,19,0,0,0,65,110,116,97,115,101,110,97,0,
  179,2,106,200,200,80,1,1,5,0,5,205,48,150,150,8,36,41,41,32,
  2,26,1,5,27,35,150,150,150,37,41,41,32,2,26,31,1,22,23,60,
  60,33,5,19,19,0,2,31,80,101,108,111,110,116,97,114,0,1,52,20,
  60,60,7,5,19,19,0,2,31,72,105,100,114,111,108,105,107,0,67,4,
  15,21,25,86,5,28,11,109,2,26,2 };
  
// this structure defines all the variables and events of your control interface 
struct {

    // input variables
  int8_t joystick_x; // from -100 to 100
  int8_t joystick_y; // from -100 to 100
  int8_t joystick_z; // from -100 to 100
  int8_t joystick_a; // from -100 to 100
  uint8_t button_pelontar; // =1 if button pressed, else =0
  uint8_t button_relay; // =1 if button pressed, else =0

    // output variables
  float kecepatan;

    // other variable
  uint8_t connect_flag;  // =1 if wire connected, else =0

} RemoteXY;   
#pragma pack(pop)
 
/////////////////////////////////////////////
//           END RemoteXY include          //
/////////////////////////////////////////////

void setup() 
{
  RemoteXY_Init(); 
  Serial.begin(115200);  // Mulai Serial Monitor

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

  digitalWrite(RELAY1, HIGH);
  digitalWrite(RELAY2, HIGH);

  Wire.begin(21, 22); // SDA, SCL ... UNTUK EXTENDER PWM

  pwm.begin();
  pwm.setPWMFreq(50);  // Servo pakai 50Hz

  Serial.println("Antasena siap tempur! 🚀");
  delay(10);
}

void loop() 
{ 
  RemoteXY_Handler();
  
  // Baca nilai joystick
  int x = applyDeadzone(RemoteXY.joystick_x);
  int y = applyDeadzone(RemoteXY.joystick_y);
  int z = applyDeadzone(RemoteXY.joystick_z) / 2;
  int a = applyDeadzone(RemoteXY.joystick_a);

  // Baca tombol
  int tombolPelontar = RemoteXY.button_pelontar;
  int tombolRelay = RemoteXY.button_relay;

  int m1 = constrain(x + y + z, -100, 100);
  int m2 = constrain(-x + y + z, -100, 100);
  int m3 = constrain(x - y + z, -100, 100);
  int m4 = constrain(x + y - z, -100, 100);

  int pulseSpeed1;
  int pulseSpeed2;

  if(tombolPelontar == 1){
    pulseSpeed1 = pwmMotorToPulse(250); // 0–255
    pulseSpeed2 = pwmMotorToPulse(0);
  } else {
    pulseSpeed1 = pwmMotorToPulse(0);
  }

  if(tombolRelay == 1){
    digitalWrite(RELAY1, LOW);
    digitalWrite(RELAY2, LOW);
    pulseSpeed1 = pwmMotorToPulse(0); // 0–255
    pulseSpeed2 = pwmMotorToPulse(250);
  } else {
    digitalWrite(RELAY1, HIGH);
    digitalWrite(RELAY2, HIGH);
    pulseSpeed2 = pwmMotorToPulse(0);
  }

  setMotorSpeed(LEDC_CHANNEL_1A, LEDC_CHANNEL_1B, m1);
  setMotorSpeed(LEDC_CHANNEL_2A, LEDC_CHANNEL_2B, m2);
  setMotorSpeed(LEDC_CHANNEL_3A, LEDC_CHANNEL_3B, m3);
  setMotorSpeed(LEDC_CHANNEL_4A, LEDC_CHANNEL_4B, m4);

  pwm.setPWM(15, 0, pulseSpeed1); // RPWM motor 1 , mati
  pwm.setPWM(12, 0, pulseSpeed2); // LPWM motor 1 , mati

  // Hitung rata-rata kecepatan (nilai absolut jika ada negatif)
  int totalSpeed = abs(m1) + abs(m2) + abs(m3) + abs(m4);
  int averageSpeed = totalSpeed / 4;
  RemoteXY.kecepatan = averageSpeed;

  // Serial.print("x : ");
  // Serial.println(x);
  // Serial.print("y : ");
  // Serial.println(y);
  // Serial.print("z : ");
  // Serial.println(z);
  Serial.print("Pelontar : ");
  Serial.println(tombolPelontar);
  Serial.print("Hidrolik : ");
  Serial.println(tombolRelay);
  Serial.printf("M1: %d | M2: %d | M3: %d | M4: %d\n", m1, m2, m3, m4);
  Serial.printf("RPWM = %d | LPWM = %d \n", pulseSpeed1, pulseSpeed2);
  Serial.println("================================================");

  // Tambahkan delay agar gak spam
  RemoteXY_delay(200);
}
