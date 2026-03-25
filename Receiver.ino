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

// ================= DATA =================
typedef struct struct_message {
  int x;
  int y;
  int btn;
} struct_message;

struct_message data;

// ================= SETTINGS =================
int deadZone = 150;
float expo = 1.8;        // 🔥 sensitivity curve (1 = linear, 2 = smoother)
float smoothFactor = 0.3; // 🔥 smoothing (0.1–0.3 best)

// current speeds (for smoothing)
float leftCurrent = 0;
float rightCurrent = 0;

// ✅ FAILSAFE TIMER
unsigned long lastReceiveTime = 0;
unsigned long timeout = 400; // ms (300–500 best)

// ================= MOTOR =================
void setMotor(int rpwm, int lpwm, int speed) {
  if (speed > 0) {
    analogWrite(rpwm, 0);
    analogWrite(lpwm, speed);
  } else if (speed < 0) {
    analogWrite(rpwm, -speed);
    analogWrite(lpwm, 0);
  } else {
    analogWrite(rpwm, 0);
    analogWrite(lpwm, 0);
  }
}

// 🛑 STOP FUNCTION (important)
void stopMotors() {
  setMotor(FL_RPWM, FL_LPWM, 0);
  setMotor(RL_RPWM, RL_LPWM, 0);
  setMotor(FR_RPWM, FR_LPWM, 0);
  setMotor(RR_RPWM, RR_LPWM, 0);

  leftCurrent = 0;
  rightCurrent = 0;
}


// ================= RECEIVE =================
void onReceive(const esp_now_recv_info *info, const uint8_t *incomingData, int len) {
  memcpy(&data, incomingData, sizeof(data));

   // ✅ Update last signal time
  lastReceiveTime = millis();

  int center = 2048;

  float x = data.x - center;
  float y = center - data.y;

  // Dead zone
  if (abs(x) < deadZone) x = 0;
  if (abs(y) < deadZone) y = 0;

  // Normalize (-1 to 1)
  x /= 2048.0;
  y /= 2048.0;

  // 🔥 EXPO CURVE (real joystick feel)
  x = pow(abs(x), expo) * (x >= 0 ? 1 : -1);
  y = pow(abs(y), expo) * (y >= 0 ? 1 : -1);

  // Arcade drive
  float leftTarget  = y + x;
  float rightTarget = y - x;

  // Clamp
  leftTarget  = constrain(leftTarget, -1, 1);
  rightTarget = constrain(rightTarget, -1, 1);

  // Convert to PWM
  leftTarget  *= 255;
  rightTarget *= 255;

  // 🔥 SMOOTHING (important)
  leftCurrent  = leftCurrent + (leftTarget - leftCurrent) * smoothFactor;
  rightCurrent = rightCurrent + (rightTarget - rightCurrent) * smoothFactor;

  // Apply motors
  setMotor(FL_RPWM, FL_LPWM, leftCurrent);
  setMotor(RL_RPWM, RL_LPWM, leftCurrent);

  setMotor(FR_RPWM, FR_LPWM, rightCurrent);
  setMotor(RR_RPWM, RR_LPWM, rightCurrent);

  // Debug
  Serial.print("L: "); Serial.print(leftCurrent);
  Serial.print(" | R: "); Serial.println(rightCurrent);
}

// ================= SETUP =================
void setup() {
  Serial.begin(115200);

  pinMode(FL_REN, OUTPUT); pinMode(FL_LEN, OUTPUT);
  pinMode(FR_REN, OUTPUT); pinMode(FR_LEN, OUTPUT);
  pinMode(RL_REN, OUTPUT); pinMode(RL_LEN, OUTPUT);
  pinMode(RR_REN, OUTPUT); pinMode(RR_LEN, OUTPUT);

  digitalWrite(FL_REN, HIGH); digitalWrite(FL_LEN, HIGH);
  digitalWrite(FR_REN, HIGH); digitalWrite(FR_LEN, HIGH);
  digitalWrite(RL_REN, HIGH); digitalWrite(RL_LEN, HIGH);
  digitalWrite(RR_REN, HIGH); digitalWrite(RR_LEN, HIGH);

  pinMode(FL_RPWM, OUTPUT); pinMode(FL_LPWM, OUTPUT);
  pinMode(FR_RPWM, OUTPUT); pinMode(FR_LPWM, OUTPUT);
  pinMode(RL_RPWM, OUTPUT); pinMode(RL_LPWM, OUTPUT);
  pinMode(RR_RPWM, OUTPUT); pinMode(RR_LPWM, OUTPUT);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  Serial.print("Receiver MAC: ");
  Serial.println(WiFi.macAddress());

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW Failed");
    return;
  }

  esp_now_register_recv_cb(onReceive);

  // 🛑 ensure stopped at boot
  stopMotors();
}

// ================= LOOP =================
void loop() {
   // 🛑 FAILSAFE CHECK
  if (millis() - lastReceiveTime > timeout) {
    stopMotors();
  }
}
