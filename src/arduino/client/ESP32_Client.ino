#include <esp_now.h>
#include <esp_wifi.h>
#include <WiFi.h>

#define ledPin  2
#define trigPin 33
#define echoPin 32

uint8_t broadcastAddress[]                    = {0xb0, 0xa7, 0x32, 0x2b, 0x23, 0x84};

const int esp32ClientID                       = 1; // ID ESP32 Client
const int wifiApChannel                       = 8; // Channel yang digunakan ESP32 Server

unsigned long previousSendDataDistanceMillis  = 0;
const long sendDataDistanceInterval           = 500;

unsigned long previousBlinkLEDMillis          = 0;
const long blinkLEDInterval                   = 100;
bool isDeliverySuccess                        = false;

typedef struct struct_message {
  char message[20];
} struct_message;

struct_message myData;

esp_now_peer_info_t peerInfo;

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");

  if (status == ESP_NOW_SEND_SUCCESS) {
    Serial.print("Delivery Success");
    isDeliverySuccess = true;
  } else {
    Serial.print("Delivery Fail");
    isDeliverySuccess = false;
  }
}

void initWiFi() {
  WiFi.mode(WIFI_STA);
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(wifiApChannel, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(false);
}

void initESPNow() {
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  esp_now_register_send_cb(OnDataSent);

  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }
}

void blinkLED() {
  unsigned long currentMillis = millis();

  if (isDeliverySuccess) {
    if (currentMillis - previousBlinkLEDMillis >= blinkLEDInterval) {
      previousBlinkLEDMillis = currentMillis;
      int ledState = digitalRead(ledPin);
      digitalWrite(ledPin, !ledState);
    }
  } else {
    digitalWrite(ledPin, HIGH);
  }
}

void sendDataDistance() {
  unsigned long currentMillis = millis();

  if (currentMillis - previousSendDataDistanceMillis >= sendDataDistanceInterval) {
    previousSendDataDistanceMillis = currentMillis;

    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);

    int currentDistance = pulseIn(echoPin, HIGH, 30000) * 0.034 / 2;
    
    if (currentDistance <= 40) {
      currentDistance = 1;
    } else {
      currentDistance = 0;
    }

    // Mengubah format agar menjadi "101" atau "100" tanpa spasi
    sprintf(myData.message, "%d %d", esp32ClientID, currentDistance);
    esp_now_send(broadcastAddress, (uint8_t *)&myData, sizeof(myData));
  }
}


void setup() {
  Serial.begin(115200);

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  pinMode(ledPin, OUTPUT);

  initWiFi();
  initESPNow();
}

void loop() {
  sendDataDistance();
  blinkLED();
}
