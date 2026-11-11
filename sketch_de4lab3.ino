
#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// ================= WIFI + MQTT =================
const char* ssid = "Wokwi-GUEST";
const char* password = "";

const char* mqtt_server = "broker.emqx.io";
const int mqtt_port = 1883;

const char* topic_data = "aquaculture/data";
const char* topic_command = "aquaculture/command";
const char* topic_status = "aquaculture/status";

// ================= GPIO =================
#define TRIG_PIN 5
#define ECHO_PIN 18
#define DS_PIN 15
#define BOM_PIN 25
#define VAN_PIN 26

// ================= SENSOR =================
OneWire oneWire(DS_PIN);
DallasTemperature ds18b20(&oneWire);

WiFiClient espClient;
PubSubClient mqtt(espClient);

// ================= DATA =================
struct SensorData {
  float distance;
  float waterLevel;
  float temperature;
};

struct AlertData {
  float waterLevel;
};

QueueHandle_t dataQueue;
QueueHandle_t alertQueue;
SemaphoreHandle_t controlMutex;

// ================= CONTROL =================
bool autoMode = true;
bool bomState = false;
bool vanState = false;
int bomPWM = 0;

const float CHIEU_CAO_BE = 250.0; // cm

// ================= MQTT CALLBACK =================
void mqttCallback(char* topic, byte* payload,
                  unsigned int length) {
  String cmd = "";

  for (int i = 0; i < length; i++) {
    cmd += (char)payload[i];
  }

  cmd.trim();

  xSemaphoreTake(controlMutex, portMAX_DELAY);

  if (cmd == "auto") {
    autoMode = true;
  }
  else if (cmd == "manual") {
    autoMode = false;
    bomState = false;
    vanState = false;
    bomPWM = 0;
  }
  else if (!autoMode) {
    if (cmd == "bom_on") {
      bomState = true;
      bomPWM = 255;
    }
    else if (cmd == "bom_off") {
      bomState = false;
      bomPWM = 0;
    }
    else if (cmd == "van_on") {
      vanState = true;
    }
    else if (cmd == "van_off") {
      vanState = false;
    }
  }

  xSemaphoreGive(controlMutex);
}

// ================= MQTT CONNECT =================
void reconnectMQTT() {
  while (!mqtt.connected()) {
    String clientID = "ESP32-Aqua-";
    clientID += String(random(0xffff), HEX);

    if (mqtt.connect(clientID.c_str())) {
      mqtt.subscribe(topic_command);
    } else {
      vTaskDelay(pdMS_TO_TICKS(2000));
    }
  }
}

// ================= READ HC-SR04 =================
float docKhoangCach() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);

  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);

  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000);

  if (duration == 0) return -1;

  return duration * 0.0343 / 2.0;
}

// ================= TASK DOC SENSOR =================
void Task_DocData(void* pvParameters) {
  while (1) {
    SensorData data;

    data.distance = docKhoangCach();

    if (data.distance >= 0 &&
        data.distance <= CHIEU_CAO_BE) {
      data.waterLevel =
          CHIEU_CAO_BE - data.distance;
    } else {
      data.waterLevel = -1;
    }

    ds18b20.requestTemperatures();
    data.temperature =
        ds18b20.getTempCByIndex(0);

    // Serial
    Serial.print("Khoang cach: ");
    Serial.print(data.distance);
    Serial.print(" cm | Muc nuoc: ");
    Serial.print(data.waterLevel);
    Serial.print(" cm | Nhiet do: ");
    Serial.print(data.temperature);
    Serial.println(" C");

    // Gui data Queue
    xQueueOverwrite(dataQueue, &data);

    // Gui alert Queue
    AlertData alert;
    alert.waterLevel = data.waterLevel;
    xQueueOverwrite(alertQueue, &alert);

    vTaskDelay(pdMS_TO_TICKS(2000));
  }
}

// ================= TASK CANH BAO =================
void Task_CanhBao(void* pvParameters) {
  AlertData alert;

  TickType_t thoiDiemBatDau = 0;
  TickType_t thoiGianChay = 0;
  bool dangChay = false;

  while (1) {
    if (xQueueReceive(alertQueue, &alert,
                     pdMS_TO_TICKS(50)) == pdTRUE) {

      xSemaphoreTake(controlMutex, portMAX_DELAY);

      if (autoMode && !dangChay) {
        float H = alert.waterLevel / 100.0;

        // H = 1m - 1.5m
        if (H >= 1.0 && H < 1.5) {
          bomState = true;
          bomPWM = 128;
          vanState = false;

          thoiGianChay = pdMS_TO_TICKS(10000);
          thoiDiemBatDau = xTaskGetTickCount();
          dangChay = true;
        }

        // H = 1.5m - 2m
        else if (H >= 1.5 && H <= 2.0) {
          bomState = true;
          bomPWM = 255;
          vanState = false;

          thoiGianChay = pdMS_TO_TICKS(20000);
          thoiDiemBatDau = xTaskGetTickCount();
          dangChay = true;
        }

        // H < 0.5m
        else if (H >= 0 && H < 0.5) {
          bomState = false;
          bomPWM = 0;
          vanState = true;

          thoiGianChay = pdMS_TO_TICKS(10000);
          thoiDiemBatDau = xTaskGetTickCount();
          dangChay = true;
        }
      }

      xSemaphoreGive(controlMutex);
    }

    // Ket thuc chu ky
    xSemaphoreTake(controlMutex, portMAX_DELAY);

    if (dangChay &&
        (xTaskGetTickCount() - thoiDiemBatDau
         >= thoiGianChay)) {

      bomState = false;
      bomPWM = 0;
      vanState = false;
      dangChay = false;
    }

    // Neu Manual: Auto khong dieu khien
    if (!autoMode) {
      dangChay = false;
    }

    // Cap nhat GPIO
    ledcWrite(BOM_PIN, bomState ? bomPWM : 0);
    digitalWrite(VAN_PIN, vanState ? HIGH : LOW);

    xSemaphoreGive(controlMutex);

    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

// ================= TASK MQTT =================
void Task_TruyenData(void* pvParameters) {
  SensorData data;
  unsigned long lastStatus = 0;

  while (1) {
    // WiFi
    if (WiFi.status() != WL_CONNECTED) {
      WiFi.begin(ssid, password);

      while (WiFi.status() != WL_CONNECTED) {
        vTaskDelay(pdMS_TO_TICKS(500));
      }
    }

    // MQTT
    if (!mqtt.connected()) {
      reconnectMQTT();
    }

    mqtt.loop();

    // Doc data Queue
    if (xQueueReceive(dataQueue, &data,
                      pdMS_TO_TICKS(100)) == pdTRUE) {

      char json[150];

      snprintf(json, sizeof(json),
        "{\"distance\":%.2f,"
        "\"water_level\":%.2f,"
        "\"temperature\":%.2f}",
        data.distance,
        data.waterLevel,
        data.temperature);

      mqtt.publish(topic_data, json);
    }

    // Trang thai he thong
    if (millis() - lastStatus >= 1000) {
      lastStatus = millis();

      char status[100];

      xSemaphoreTake(controlMutex, portMAX_DELAY);

      snprintf(status, sizeof(status),
        "{\"mode\":\"%s\","
        "\"pump\":%d,"
        "\"valve\":%d}",
        autoMode ? "auto" : "manual",
        bomState,
        vanState);

      xSemaphoreGive(controlMutex);

      mqtt.publish(topic_status, status);
    }

    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

// ================= SETUP =================
void setup() {
  Serial.begin(115200);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(VAN_PIN, OUTPUT);

  digitalWrite(TRIG_PIN, LOW);
  digitalWrite(VAN_PIN, LOW);

  // PWM BOM
  ledcAttach(BOM_PIN, 1000, 8);
  ledcWrite(BOM_PIN, 0);

  // DS18B20
  ds18b20.begin();
  ds18b20.setResolution(12);

  // MQTT
  mqtt.setServer(mqtt_server, mqtt_port);
  mqtt.setCallback(mqttCallback);
  mqtt.setBufferSize(256);

  // Queue
  dataQueue = xQueueCreate(1, sizeof(SensorData));
  alertQueue = xQueueCreate(1, sizeof(AlertData));

  // Mutex
  controlMutex = xSemaphoreCreateMutex();

  // Task
  xTaskCreatePinnedToCore(
    Task_DocData, "DocData", 4096,
    NULL, 2, NULL, 1);

  xTaskCreatePinnedToCore(
    Task_CanhBao, "CanhBao", 4096,
    NULL, 2, NULL, 1);

  xTaskCreatePinnedToCore(
    Task_TruyenData, "TruyenData", 6144,
    NULL, 1, NULL, 0);
}

void loop() {
  vTaskDelay(pdMS_TO_TICKS(1000));
}
