#include <WiFi.h>
#define LED_PIN 2   // onboard LED (common for ESP32)
void setup() {
  Serial.begin(115200);

  pinMode(LED_PIN, OUTPUT);

  WiFi.mode(WIFI_AP);  // Access Point mode only

  WiFi.softAP("ESP32_TEST", "12345678");  // SSID, password (min 8 chars)

  Serial.println("WiFi AP started");
  Serial.print("IP Address: ");
  Serial.println(WiFi.softAPIP());
}

void loop() {
  digitalWrite(LED_PIN, HIGH);  // LED ON
  delay(500);
  digitalWrite(LED_PIN, LOW);   // LED OFF
  delay(500);

}
