#if defined(ESP32)
#include <WiFiMulti.h>
WiFiMulti wifiMulti;
#define DEVICE "ESP32-board-4"
#elif defined(ESP8266)
#include <ESP8266WiFiMulti.h>
ESP8266WiFiMulti wifiMulti;
#define DEVICE "ESP8266"
#endif

#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>

#include "secrets.h"

#define INFLUXDB_URL "https://us-east-1-1.aws.cloud2.influxdata.com"
#define INFLUXDB_TOKEN "wFeWxhMhh-iSmxm_6v93xcEYiJomrUZ0elwZDWr6R8JJ_lcxthpGO58MX1z2a6cm9cN0ZGPWwXl6ym95zl-iLQ=="
#define INFLUXDB_ORG "fd834ecbef2de6e5"
#define INFLUXDB_BUCKET "arduino"

// Time zone info
#define TZ_INFO "UTC-4"

// Declare InfluxDB client instance with preconfigured InfluxCloud certificate
InfluxDBClient client(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN, InfluxDbCloud2CACert);

// Declare Data point
Point sensor("wifi_status");

String translateEncryptionType(wifi_auth_mode_t encryptionType)
{

  switch (encryptionType)
  {
  case (WIFI_AUTH_OPEN):
    return "Open";
  case (WIFI_AUTH_WEP):
    return "WEP";
  case (WIFI_AUTH_WPA_PSK):
    return "WPA_PSK";
  case (WIFI_AUTH_WPA2_PSK):
    return "WPA2_PSK";
  case (WIFI_AUTH_WPA_WPA2_PSK):
    return "WPA_WPA2_PSK";
  case (WIFI_AUTH_WPA2_ENTERPRISE):
    return "WPA2_ENTERPRISE";
  default:  
    return "UNKNOWN";
  }
  return "UNKNOWN";
}

void setup()
{
  Serial.begin(115200);

  Serial.println("ESP Information:");
  Serial.println("-----------------");
  Serial.printf("ESPChipModel: %s\r\n", ESP.getChipModel());
  Serial.printf("ESPChipRevision: %s\r\n", String(ESP.getChipRevision()));
  Serial.printf("ESPCPUFrequency: %s\r\n", String(ESP.getCpuFreqMHz()));
  Serial.printf("ESPChipCores: %s\r\n", String(ESP.getChipCores()));
  Serial.printf("ESPSdkVersion: %s\r\n", ESP.getSdkVersion());
  Serial.printf("MAC Address: %s\r\n", WiFi.BSSIDstr());
  Serial.println("-----------------");
  
  // Setup wifi
  WiFi.mode(WIFI_STA);

  int numberOfNetworks = WiFi.scanNetworks();

  Serial.print("Number of networks found: ");
  Serial.println(numberOfNetworks);

  for (int i = 0; i < numberOfNetworks; i++)
  {

    Serial.print("Network name: ");
    Serial.println(WiFi.SSID(i));

    Serial.print("Signal strength: ");
    Serial.println(WiFi.RSSI(i));

    Serial.print("MAC address: ");
    Serial.println(WiFi.BSSIDstr(i));

    Serial.print("Encryption type: ");
    String encryptionTypeDescription = translateEncryptionType(WiFi.encryptionType(i));
    Serial.println(encryptionTypeDescription);
    Serial.println("-----------------------");
  }

  wifiMulti.addAP(WIFI_SSID, WIFI_PASSWORD);

  Serial.println("Connecting to wifi");
  while (wifiMulti.run() != WL_CONNECTED)
  {
    wifiMulti.addAP(WIFI_SSID, WIFI_PASSWORD);

    Serial.printf("Status: %d\r\n", wifiMulti.run());
    WiFi.printDiag(Serial);
    Serial.printf("TxPower: %d\r\n", WiFi.getTxPower());
    
    Serial.print(".");
    delay(100);
  }

  Serial.printf("Local IP address: %s\r\n", String(WiFi.localIP()));

  Serial.println();

  // Accurate time is necessary for certificate validation and writing in batches
  // We use the NTP servers in your area as provided by: https://www.pool.ntp.org/zone/
  // Syncing progress and the time will be printed to Serial.
  timeSync(TZ_INFO, "pool.ntp.org", "time.nis.gov");

  // Check server connection
  if (client.validateConnection())
  {
    Serial.print("Connected to InfluxDB: ");
    Serial.println(client.getServerUrl());
  }
  else
  {
    Serial.print("InfluxDB connection failed: ");
    Serial.println(client.getLastErrorMessage());
  }

  sensor.addTag("device", DEVICE);
  sensor.addTag("SSID", WiFi.SSID());

  sensor.addTag("ESPChipModel", ESP.getChipModel());
  sensor.addTag("ESPChipRevision", String(ESP.getChipRevision()));
  sensor.addTag("ESPCPUFrequency", String(ESP.getCpuFreqMHz()));
  sensor.addTag("ESPChipCores", String(ESP.getChipCores()));
  sensor.addTag("ESPSdkVersion", ESP.getSdkVersion());

  sensor.addTag("MACAddress", String(WiFi.macAddress()));
  sensor.addTag("LocalIP", WiFi.localIP());

  sensor.clearFields();
  sensor.addField("restart", 1);

  Serial.print("Writing: ");
  Serial.println(sensor.toLineProtocol());

  if (!client.writePoint(sensor))
  {
    Serial.print("InfluxDB write failed: ");
    Serial.println(client.getLastErrorMessage());
  }
}
void loop()
{
  // Clear fields for reusing the point. Tags will remain the same as set above.
  sensor.clearFields();

  // Store measured value into point
  // Report RSSI of currently connected network
  sensor.addField("rssi", WiFi.RSSI());

  // Print what are we exactly writing
  Serial.print("Writing: ");
  Serial.println(sensor.toLineProtocol());

  // Check WiFi connection and reconnect if needed
  if (wifiMulti.run() != WL_CONNECTED)
  {
    Serial.println("Wifi connection lost");
    delay(10000);
    ESP.restart();
  }

  // Write point
  if (!client.writePoint(sensor))
  {
    Serial.print("InfluxDB write failed: ");
    Serial.println(client.getLastErrorMessage());
  }

  Serial.println("Waiting 1 second");
  delay(1000);
}