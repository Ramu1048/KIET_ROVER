#include <esp_now.h>
#include <WiFi.h>

// ================= JOYSTICK PINS =================
#define VRX 34
#define VRY 35
#define SW  32

// ================= DATA STRUCT =================
typedef struct struct_message {
  int x;
  int y;
  int btn;
} struct_message;

struct_message data;

// ✅ YOUR RECEIVER MAC ADDRESS
uint8_t receiverMAC[] = {0x08, 0xA6, 0xF7, 0x48, 0x5D, 0x70};

esp_now_peer_info_t peerInfo;

// ================= SETUP =================
void setup() {
  Serial.begin(115200);

  // Joystick button
  pinMode(SW, INPUT_PULLUP);

  // WiFi in STA mode (MANDATORY)
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  Serial.print("Transmitter MAC: ");
  Serial.println(WiFi.macAddress());

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW Init Failed");
    return;
  }

  // Register peer
  memcpy(peerInfo.peer_addr, receiverMAC, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }

  Serial.println("ESP-NOW Ready");
}

// ================= LOOP =================
void loop() {
  // Read joystick
  data.x = analogRead(VRX);
  data.y = analogRead(VRY);
  data.btn = digitalRead(SW);

  // Send data
  esp_err_t result = esp_now_send(receiverMAC, (uint8_t *) &data, sizeof(data));

  // Debug output
  Serial.print("X: ");
  Serial.print(data.x);
  Serial.print(" | Y: ");
  Serial.print(data.y);
  Serial.print(" | Btn: ");
  Serial.print(data.btn);

  if (result == ESP_OK) {
    Serial.println(" | Sent ");
  } else {
    Serial.println(" | Error ");
  }

  delay(100);
}
