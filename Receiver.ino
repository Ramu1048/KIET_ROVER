#include <esp_now.h>
#include <WiFi.h>

// ================= MOTOR PINS =================
#define FL_RPWM 25
#define FL_LPWM 26
#define FR_RPWM 32
#define FR_LPWM 33

#define RL_RPWM 18
#define RL_LPWM 19
#define RR_RPWM 23
#define RR_LPWM 5

#define FL_REN 27
#define FL_LEN 14
#define FR_REN 12
#define FR_LEN 13
#define RL_REN 21
#define RL_LEN 22
#define RR_REN 16
#define RR_LEN 17

// ================= DATA STRUCT =================
typedef struct struct_message {
  int x;
  int y;
  int btn;
} struct_message;

struct_message data;

// ================= MOTOR CONTROL FUNCTION =================

// 🔥 Handles forward + backward + stop automatically
void setMotor(int rpwm, int lpwm, int speed) {
  if (speed > 0) {
    analogWrite(rpwm, 0);
    analogWrite(lpwm, speed);
  } 
  else if (speed < 0) {
    analogWrite(rpwm, -speed);
    analogWrite(lpwm, 0);
  } 
  else {
    analogWrite(rpwm, 0);
    analogWrite(lpwm, 0);
  }
}

// ================= ESP-NOW RECEIVE =================

void onReceive(const esp_now_recv_info *info, const uint8_t *incomingData, int len) {
  memcpy(&data, incomingData, sizeof(data));

  int center = 2048;
  int deadZone = 200;

  // Convert joystick values
  int x = data.x - center;
  int y = center - data.y;  // forward positive

  // Apply dead zone
  if (abs(x) < deadZone) x = 0;
  if (abs(y) < deadZone) y = 0;

  // Map to PWM range (-255 to 255)
  x = map(x, -2048, 2048, -255, 255);
  y = map(y, -2048, 2048, -255, 255);

  // 🎮 Differential drive (8-direction control)
  int leftSpeed  = y + x;
  int rightSpeed = y - x;

  // Limit values
  leftSpeed  = constrain(leftSpeed, -255, 255);
  rightSpeed = constrain(rightSpeed, -255, 255);

  // Apply speeds to motors
  // LEFT SIDE
  setMotor(FL_RPWM, FL_LPWM, leftSpeed);
  setMotor(RL_RPWM, RL_LPWM, leftSpeed);

  // RIGHT SIDE
  setMotor(FR_RPWM, FR_LPWM, rightSpeed);
  setMotor(RR_RPWM, RR_LPWM, rightSpeed);

  // Debug
  Serial.print("X: "); Serial.print(data.x);
  Serial.print(" | Y: "); Serial.print(data.y);
  Serial.print(" | Left: "); Serial.print(leftSpeed);
  Serial.print(" | Right: "); Serial.println(rightSpeed);
}

// ================= SETUP =================

void setup() {
  Serial.begin(115200);

  // Enable pins
  pinMode(FL_REN, OUTPUT); pinMode(FL_LEN, OUTPUT);
  pinMode(FR_REN, OUTPUT); pinMode(FR_LEN, OUTPUT);
  pinMode(RL_REN, OUTPUT); pinMode(RL_LEN, OUTPUT);
  pinMode(RR_REN, OUTPUT); pinMode(RR_LEN, OUTPUT);

  digitalWrite(FL_REN, HIGH); digitalWrite(FL_LEN, HIGH);
  digitalWrite(FR_REN, HIGH); digitalWrite(FR_LEN, HIGH);
  digitalWrite(RL_REN, HIGH); digitalWrite(RL_LEN, HIGH);
  digitalWrite(RR_REN, HIGH); digitalWrite(RR_LEN, HIGH);

  // PWM pins
  pinMode(FL_RPWM, OUTPUT); pinMode(FL_LPWM, OUTPUT);
  pinMode(FR_RPWM, OUTPUT); pinMode(FR_LPWM, OUTPUT);
  pinMode(RL_RPWM, OUTPUT); pinMode(RL_LPWM, OUTPUT);
  pinMode(RR_RPWM, OUTPUT); pinMode(RR_LPWM, OUTPUT);

  // ESP-NOW Setup
  WiFi.mode(WIFI_STA);

  Serial.print("Receiver MAC Address: ");
  Serial.println(WiFi.macAddress());

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW Init Failed");
    return;
  }

  esp_now_register_recv_cb(onReceive);
}

// ================= LOOP =================

void loop() {
  // Nothing needed
}
