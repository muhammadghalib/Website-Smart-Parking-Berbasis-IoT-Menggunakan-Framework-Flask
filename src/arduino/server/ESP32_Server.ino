#include <esp_now.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#define ledPin 2

const char* SSID      = "Home";
const char* PASSWORD  = "Geha12345";
const char* serverURL = "http://192.168.0.6:5000/update-data";

const int numEsp32Devices = 15;

bool serverConnectionSuccess = false;

int esp32ClientID[numEsp32Devices];
char esp32ClientStatus[numEsp32Devices];
int messageIndexClient[numEsp32Devices];

typedef struct struct_message {
  char message[20];
} struct_message;

struct_message incomingMessage;

void initializeWiFi() {
  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(SSID, PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
  }

  WiFi.softAP("Smart Parking", "Smart-Parking");

  Serial.print(F("\nESP32 Server [SSID]          = "));
  Serial.println(SSID);
  Serial.print(F("ESP32 Server [IP Address]    = "));
  Serial.println(WiFi.localIP());
  Serial.print(F("ESP32 Server [Wi-Fi Channel] = "));
  Serial.println(WiFi.channel());
}

void initializeEspNow() {
  if (esp_now_init() != ESP_OK) {
    Serial.println(F("Error initializing ESP-NOW"));
    return;
  }
}
    
void initializeClientData() {
  for (int i = 0; i < numEsp32Devices; i++) {
    esp32ClientID[i] = i + 1;
    esp32ClientStatus[i] = 'N';
    messageIndexClient[i] = 1;
  }
}

void processIncomingData() {
  esp_now_register_recv_cb([](const uint8_t *mac, const uint8_t *incomingData, int len) {
    memcpy(&incomingMessage, incomingData, sizeof(incomingMessage));
    Serial.print(F("ESP32 Client [Received] = "));
    Serial.print(incomingMessage.message);

    for (int i = 0; i < numEsp32Devices; i++) {
      char messageCheckOn[20], messageCheckOff[20];
      snprintf(messageCheckOn, sizeof(messageCheckOn), "%d 1", i + 1);
      snprintf(messageCheckOff, sizeof(messageCheckOff), "%d 0", i + 1);

      if (strcmp(incomingMessage.message, messageCheckOn) == 0 || strcmp(incomingMessage.message, messageCheckOff) == 0) {
        Serial.print(F(" | Index = "));
        Serial.println(messageIndexClient[i]);
        messageIndexClient[i]++;
        esp32ClientStatus[i] = (strcmp(incomingMessage.message, messageCheckOn) == 0) ? '1' : '0';
        break;
      }
    }
  });
}

void sendDeviceDataToServer(JsonDocument& jsonData) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    http.begin(serverURL);
    http.addHeader("Content-Type", "application/json");

    String output;
    serializeJson(jsonData, output);

    int httpResponseCode = http.POST(output);

    if (httpResponseCode > 0) {
      serverConnectionSuccess = true;
      String response = http.getString();
      response = "ESP32 Server [HTTP Response] = " + response + "\n";
      Serial.println(F("\nESP32 Server [HTTP Post]     = Data successfully sent to Flask"));
      Serial.println(response);
    } else {
      serverConnectionSuccess = false;
      Serial.println(F("\nESP32 Server [HTTP Post] = Error sending data\n"));
    }

    http.end();
  } else {
    Serial.println(F("WiFi not connected"));
  }
}

void taskSendDeviceDataToServer(void *parameter) {
  while (true) {
    StaticJsonDocument<256> jsonData; 

    for (int i = 0; i < numEsp32Devices; i++) {
      jsonData[String(i + 1)] = String(esp32ClientStatus[i]);
    }

    sendDeviceDataToServer(jsonData);

    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

void taskBlinkLED(void *parameter) {
  while (true) {
    if (serverConnectionSuccess) {
      int ledState = digitalRead(ledPin);
      digitalWrite(ledPin, !ledState);
      vTaskDelay(pdMS_TO_TICKS(100));
    } else {
      digitalWrite(ledPin, HIGH);
      vTaskDelay(pdMS_TO_TICKS(500));
    }
  }
}

void taskMonitorMessageIndex(void *parameter) {
  while (true) {
    static int previousMessageIndex[numEsp32Devices] = {0};

    for (int i = 0; i < numEsp32Devices; i++) {
      if (messageIndexClient[i] == previousMessageIndex[i]) {
        if (esp32ClientStatus[i] != 'N') {
          esp32ClientStatus[i] = 'N';  
        }
      }
      previousMessageIndex[i] = messageIndexClient[i];
    }

    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

void setup() {
  Serial.begin(115200);
  delay(1500);

  pinMode(ledPin, OUTPUT);

  initializeWiFi();
  initializeEspNow();
  initializeClientData();
  processIncomingData();

  xTaskCreatePinnedToCore(taskSendDeviceDataToServer, "taskSendDeviceDataToServer", 4096, NULL, 5, NULL, 1);
  xTaskCreatePinnedToCore(taskBlinkLED, "taskBlinkLED", 1024, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(taskMonitorMessageIndex, "taskMonitorMessageIndex", 2048, NULL, 2, NULL, 1);
}

void loop() {
  
}