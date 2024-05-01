#include "AzureSetup.h"
#include "Wire.h"
#include "PZEM004Tv30.h"
#include "ArduinoJson.h"
#include "LiquidCrystal.h"

#define RX 17
#define Tx 16

PZEM004Tv30 pzme(Serial2,17, 16, PZEM_DEFAULT_ADDR); // RX, TX

const int rs = 19, en = 23, d4 = 18, d5 = 5, d6 = 4, d7 = 15;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
const int lcdSwitch=25;
volatile bool lcdOn = false;

float voltage;
float current ;
float energy ;
float powerFactor;
static void sendTelemetry();
static void generateTelemetryPayload();
static void establishConnection();
void getPzPowerReadings();
void lcdDisplay(float voltage, float current, float energy, float powerFactor);
void ARDUINO_ISR_ATTR isr();
void setup() 
{
   attachInterrupt(lcdSwitch, isr, FALLING);
   establishConnection();
   Serial.begin(115200);
   Serial2.begin(9600, SERIAL_8N1, 17, 16);
  lcd.begin( 4,  16);
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

  lcdDisplay(voltage, current, energy, powerFactor);
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
JsonDocument jsonTelemetry_payload;
jsonTelemetry_payload["Message Count"]= telemetry_send_count++;
jsonTelemetry_payload["Voltage"] =voltage;
jsonTelemetry_payload["Current"] = current;
jsonTelemetry_payload["Energy"]= energy;
jsonTelemetry_payload["Power Factor"]= powerFactor;

serializeJson(jsonTelemetry_payload, telemetry_payload);
 
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

void lcdDisplay(float voltage, float current, float energy, float powerFactor)
{
  lcd.println("Voltage:"+String(voltage));
  lcd.println("Current:"+String(current));
  lcd.println("Power Factor:"+String(powerFactor));
  lcd.println("Energy:"+String(energy));

}


void getPzPowerReadings()
{
  voltage= pzme.voltage();
  current = pzme.current();
  energy = pzme.energy();
  powerFactor=  pzme.pf();

}

void ARDUINO_ISR_ATTR isr()
{
  lcdOn=!lcdOn;
  if(lcdOn==true)
  {
    // Turn on the display:
    lcd.display();
  }else
  {
    //turn off lcd 
    lcd.noDisplay();
  }
}