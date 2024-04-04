#include "AzureSetup.h"
#include "Wire.h"
#include "PZEM004Tv30.h"
#include "ArduinoJson.h"

PZEM004Tv30 pzme(17, 18); // RX, TX


static void sendTelemetry();
static void generateTelemetryPayload();
static void establishConnection();
static void getPzPowerReadings();

void setup() 
{
   establishConnection();
   Serial.Begin(9600);
}

void loop()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    connectToWiFi();
  }
#ifndef IOT_CONFIG_USE_X509_CERT
  else if (sasToken.IsExpired())
  {
    Logger.Info("SAS token expired; reconnecting with a new one.");
    (void)esp_mqtt_client_destroy(mqtt_client);
    initializeMqttClient();
  }
#endif
  else if (millis() > next_telemetry_send_time_ms)
  {
    sendTelemetry();
    next_telemetry_send_time_ms = millis() + TELEMETRY_FREQUENCY_MILLISECS;
  }
}
static void generateTelemetryPayload()
{
  // You can generate the JSON using any lib you want. Here we're showing how to do it manually, for simplicity.
  // This sample shows how to generate the payload using a syntax closer to regular delevelopment for Arduino, with
  // String type instead of az_span as it might be done in other samples. Using az_span has the advantage of reusing the 
  // same char buffer instead of dynamically allocating memory each time, as it is done by using the String type below.
  
/*
 telemetry_payload = "{ \"msgCount\": " + String(telemetry_send_count++) + " }";
*/
getPzPowerReadings();
JsonDocument telemetry_payload;
telemetry_payload["Voltage"] =voltage;
telemetry_payload["Current"] = current;
telemetry_payload["Energy"]= energy;
telemetry_payload["Power Factor"]= powerFactor;
 
}

static void sendTelemetry()
{
  Logger.Info("Sending telemetry ...");

  // The topic could be obtained just once during setup,
  // however if properties are used the topic need to be generated again to reflect the
  // current values of the properties.
  if (az_result_failed(az_iot_hub_client_telemetry_get_publish_topic(
          &client, NULL, telemetry_topic, sizeof(telemetry_topic), NULL)))
  {
    Logger.Error("Failed az_iot_hub_client_telemetry_get_publish_topic");
    return;
  }

  generateTelemetryPayload();

  if (esp_mqtt_client_publish(
          mqtt_client,
          telemetry_topic,
          (const char*)telemetry_payload.c_str(),
          telemetry_payload.length(),
          MQTT_QOS1,
          DO_NOT_RETAIN_MSG)
      == 0)
  {
    Logger.Error("Failed publishing");
  }
  else
  {
    Logger.Info("Message published successfully");
  }
}

static void getPzPowerReadings()
{
  float voltage= pzme.voltage();
  float current = pzme.current();
  float energy = pzme.energy();
  float powerFactor=  pzme.pf();

}