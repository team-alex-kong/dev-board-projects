#include "configs.h"
#define sensorSerial Serial1

#define SERIAL_BAUD_RATE 115200

// Change the communication baud rate here, if necessary
// #define LD2410_BAUD_RATE 256000
#include <ArduinoJson.h>
#include <MyLD2410.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <Wire.h>

#ifdef DEBUG_MODE
MyLD2410 sensor(sensorSerial, true);
#else
MyLD2410 sensor(sensorSerial);
#endif

uint32_t last_publish_at = 0;
bool has_published = false;
bool is_prev_positive = false;
bool sensor_ready = false;
uint32_t last_sensor_attempt_at = 0;
uint32_t last_sensor_data_at = 0;

void printValue(const byte &val) {
  Serial.print(' ');
  Serial.print(val);
}

void printData() {
  Serial.print(sensor.statusString());
  if (sensor.presenceDetected()) {
    Serial.print(", distance: ");
    Serial.print(sensor.detectedDistance());
    Serial.print("cm");
  }
  Serial.println();
  if (sensor.movingTargetDetected()) {
    Serial.print(" MOVING    = ");
    Serial.print(sensor.movingTargetSignal());
    Serial.print("@");
    Serial.print(sensor.movingTargetDistance());
    Serial.print("cm ");
    if (sensor.inEnhancedMode()) {
      Serial.print("\n signals->[");
      sensor.getMovingSignals().forEach(printValue);
      Serial.print(" ] thresholds:[");
      sensor.getMovingThresholds().forEach(printValue);
      Serial.print(" ]");
    }
    Serial.println();
  }
  if (sensor.stationaryTargetDetected()) {
    Serial.print(" STATIONARY= ");
    Serial.print(sensor.stationaryTargetSignal());
    Serial.print("@");
    Serial.print(sensor.stationaryTargetDistance());
    Serial.print("cm ");
    if (sensor.inEnhancedMode()) {
      Serial.print("\n signals->[");
      sensor.getStationarySignals().forEach(printValue);
      Serial.print(" ] thresholds:[");
      sensor.getStationaryThresholds().forEach(printValue);
      Serial.print(" ]");
    }
    Serial.println();
  }

  if (sensor.inEnhancedMode() && (sensor.getFirmwareMajor() > 1)) {
    Serial.print("Light level: ");
    Serial.println(sensor.getLightLevel());
    Serial.print("Output level: ");
    Serial.println((sensor.getOutLevel()) ? "HIGH" : "LOW");
  }

  Serial.println();
}

WiFiClientSecure wifi_client;
PubSubClient mqtts_client(wifi_client);
uint32_t last_wifi_attempt_at = 0;
uint32_t last_mqtt_attempt_at = 0;
bool mqtt_attempted = false;

void setup_wifi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  last_wifi_attempt_at = millis();
  Serial.println("Connecting to WiFi [" WIFI_SSID "]");
}

void reconnect_mqtts() {
  const auto t_ms = millis();
  if (WiFi.status() != WL_CONNECTED) {
    mqtt_attempted = false;
    if (mqtts_client.connected())
      mqtts_client.disconnect();
    if (uint32_t(t_ms - last_wifi_attempt_at) >= 60000UL)
      setup_wifi();
    return;
  }
  if (mqtts_client.connected() ||
      (mqtt_attempted && uint32_t(t_ms - last_mqtt_attempt_at) < 10000UL))
    return;

  mqtt_attempted = true;
  Serial.printf("Connecting to MQTT broker " MQTT_SERVER
                ":%d with MQTT_CLIENT_ID: " MQTT_CLIENT_ID "\n",
                MQTT_PORT);
  if (mqtts_client.connect(MQTT_CLIENT_ID, MQTT_USERNAME, MQTT_PASSWORD)) {
    has_published = false;
    Serial.println("connected");
  } else {
    Serial.printf("mqtts_client.connect() failed, state = %d; retrying in 10 seconds\n",
                  mqtts_client.state());
  }
  last_mqtt_attempt_at = millis();
}

void setup_sensor() {
  sensor_ready = sensor.begin() && sensor.enhancedMode();
  last_sensor_attempt_at = millis();
  if (sensor_ready) {
    last_sensor_data_at = last_sensor_attempt_at;
    has_published = false;
    Serial.println("Sensor connected");
  } else {
    Serial.println("Failed to communicate with the sensor; retrying in 5 seconds");
  }
}

void setup() {
  delay(10000);
  Serial.begin(SERIAL_BAUD_RATE);
#if defined(ARDUINO_ARCH_ESP32)
  sensorSerial.setPins(0, 1); // D0, D1 on Nano ESP32
#elif defined(ARDUINO_ARCH_RP2040)
  sensorSerial.setTX(0); // GP0
  sensorSerial.setRX(1); // GP1
#else
#error "Define ARDUINO_ARCH_ESP32 or ARDUINO_ARCH_RP2040"
#endif
  sensorSerial.begin(LD2410_BAUD_RATE);

  delay(2000);
  Serial.println(__FILE__);
  setup_sensor();

  setup_wifi();
#if defined(MQTT_SSL_SET_INSECURE)
  Serial.println(
      "wifi_client.setInsecure(), SSL certificate verification skipped");
  wifi_client.setInsecure();
#else
  Serial.printf("wifi_client.setCACert(): %s\n", root_ca);
  wifi_client.setCACert(root_ca);
#endif
  mqtts_client.setServer(MQTT_SERVER, MQTT_PORT);
  mqtts_client.setSocketTimeout(2);
}

void loop() {
  mqtts_client.loop();
  reconnect_mqtts();

  if (!sensor_ready) {
    if (uint32_t(millis() - last_sensor_attempt_at) >= 5000UL)
      setup_sensor();
    return;
  }

  bool has_sensor_data = false;
  while (sensorSerial.available()) {
    if (sensor.check() == MyLD2410::Response::DATA)
      has_sensor_data = true;
  }
  const auto t_ms = millis();
  if (!has_sensor_data) {
    if (uint32_t(t_ms - last_sensor_data_at) >= 5000UL) {
      sensor_ready = false;
      Serial.println("Sensor data timed out; reconnecting");
    }
    return;
  }
  last_sensor_data_at = t_ms;
  if (!mqtts_client.connected())
    return;

  const auto moving_target_detected = sensor.movingTargetDetected();
  const auto stationary_target_detected = sensor.stationaryTargetDetected();

  const auto is_curr_positive =
      moving_target_detected || stationary_target_detected;
  const uint32_t publish_interval_ms = is_curr_positive
                                          ? MQTT_POSITIVE_PUBLISH_INTERVAL_MS
                                          : MQTT_NEGATIVE_PUBLISH_INTERVAL_MS;
  if (has_published && is_prev_positive == is_curr_positive &&
      uint32_t(t_ms - last_publish_at) < publish_interval_ms)
    return;

  JsonDocument jsonDoc;
  jsonDoc["stationary_target_detected"] = stationary_target_detected;
  if (stationary_target_detected)
    jsonDoc["stationary_target_distance_cm"] =
        sensor.stationaryTargetDistance();
  jsonDoc["moving_target_detected"] = moving_target_detected;
  if (moving_target_detected)
    jsonDoc["moving_target_distance_cm"] = sensor.movingTargetDistance();

  String output;
  serializeJson(jsonDoc, output);
  Serial.printf("Publishing payload [%s] to topic [%s]... ", output.c_str(),
                MQTT_TOPIC);

  std::string msgPackStr;
  size_t size = serializeMsgPack(jsonDoc, msgPackStr);

  if (mqtts_client.publish(
          MQTT_TOPIC, reinterpret_cast<const uint8_t *>(msgPackStr.data()),
          size)) {
    last_publish_at = millis();
    has_published = true;
    is_prev_positive = is_curr_positive;
    Serial.println("done");
  } else {
    Serial.println("failed!");
  }
}
