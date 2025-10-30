#define _DISABLE_TLS_
#define THINGER_SERIAL_DEBUG

#include <ThingerESP32.h>
#include "secrets.h"

#define THINGER_PORT 1883

ThingerESP32 thing(USERNAME, DEVICE_ID, DEVICE_CREDENTIAL);

static const char ROOT_CA[] PROGMEM = R"EOF(-----BEGIN CERTIFICATE-----
MIIFEDCCA/igAwIBAgISBeXZNax1zlf7nyOBvtmbVOeXMA0GCSqGSIb3DQEBCwUA
MDMxCzAJBgNVBAYTAlVTMRYwFAYDVQQKEw1MZXQncyBFbmNyeXB0MQwwCgYDVQQD
EwNSMTMwHhcNMjUwOTI1MTQwNTIxWhcNMjUxMjI0MTQwNTIwWjAlMSMwIQYDVQQD
ExptYWlzb25uZXV2ZS5hd3MudGhpbmdlci5pbzCCASIwDQYJKoZIhvcNAQEBBQAD
ggEPADCCAQoCggEBAK3JQw1fHkTFuFySuNfTsEKM3+dPggaMIRhESeJ9TxoXcWjC
NNB46eCcSoI5G7q8zfTzv4ixJ0mlO8ip7Ugkgw8A6vsQZW9HcbvL2xx/jMUx+837
IqHTT/I3NA2bUXBc6JaJdu145I2XvY3nHi6ORXX8d4BQ1sATMicPnQK7MrvAQnnX
3qAv8+LojEpqR93E113jRjjUrGg2N5AgkWijxt3w/H5ENFpEzBy/VjZ1zBZTF7Wm
nFVfGplh7kDoHfacD9O3yhyUBU6tljQUcrxIGMe+CZRENKY+QSdcceZ/xrq3I3Oj
Ejoeu+FWgAcM1U8YniIhhHuI/kZp3S48o6LbocECAwEAAaOCAiowggImMA4GA1Ud
DwEB/wQEAwIFoDAdBgNVHSUEFjAUBggrBgEFBQcDAQYIKwYBBQUHAwIwDAYDVR0T
AQH/BAIwADAdBgNVHQ4EFgQUqzo9Wj9HPEk30W2652AI/wVjM0swHwYDVR0jBBgw
FoAU56ufDywzoFPTXk94yLKEDjvWkjMwMwYIKwYBBQUHAQEEJzAlMCMGCCsGAQUF
BzAChhdodHRwOi8vcjEzLmkubGVuY3Iub3JnLzAlBgNVHREEHjAcghptYWlzb25u
ZXV2ZS5hd3MudGhpbmdlci5pbzATBgNVHSAEDDAKMAgGBmeBDAECATAuBgNVHR8E
JzAlMCOgIaAfhh1odHRwOi8vcjEzLmMubGVuY3Iub3JnLzI0LmNybDCCAQQGCisG
AQQB1nkCBAIEgfUEgfIA8AB2ABLxTjS9U3JMhAYZw48/ehP457Vih4icbTAFhOvl
hiY6AAABmYFm808AAAQDAEcwRQIhALsaUlV3syLlopq7Ztdq7vLy299mclaH3fSo
U1vruKMiAiBtMNtD3zriGNff6CLAARDXrgq2o/1uIVrPnNYY6TStkAB2ABmG1Mco
qm/+ugNveCpNAZGqzi1yMQ+uzl1wQS0lTMfUAAABmYFm9F0AAAQDAEcwRQIhAMAa
r7lxSE8SJNILj48X1l/rAmji7BDPEbYp7X4tmmTVAiAH+W2G2bC8kMCVef25VSIl
je0Et4ylABnDIhEAKaO91zANBgkqhkiG9w0BAQsFAAOCAQEAlYXL6ArlCmL+Q9Ld
rxeRLRRURZjBzEWWOoZtr+EiqDQP+DUSMCMO4yWZfB+TrR1wVoxzbY7eeqsIuvSe
+iADaWIwl5tKp9nwfQG2hj9Uqm0MLKF335JM01/aV8tisjCIHBtPjSDnSaZJkAks
9Qyps7dxExd3ivsTYV3+MlNbQaq99NwS+sI4QQK8CsldGGjAbrcl23CiiVio+EQ7
AAFRA8rd6dhVGLbEDP7/m8StE6H+5EoXjNtPlDkWMEG6WKFUjfXvhXKfZ417qafQ
atXz2VouxS3N/GzE21fs9dBqj/cKPERUtY+tZfHWSZG2oz2TxirjDZ9n4LQlxFYb
ezVUpQ==
-----END CERTIFICATE-----)EOF";

static void syncTime(){
  // Fuseau EST/EDT auto
  configTzTime("EST5EDT,M3.2.0/2,M11.1.0/2", "pool.ntp.org", "time.nist.gov");
  struct tm tm_info;
  // Attendre une horloge valide pour éviter l’échec TLS
  for(int i=0;i<50;i++){
    if(getLocalTime(&tm_info, 1000)) return;
    delay(200);
  }
}


void setup() {
  // open serial for debugging
  Serial.begin(115200);

  pinMode(21, OUTPUT);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(200);
  }
  
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());


  thing.set_root_ca(ROOT_CA);
  thing.set_host("maisonneuve.aws.thinger.io");
  thing.add_wifi(WIFI_SSID, WIFI_PASSWORD);
  
  syncTime();

  // digital pin control example (i.e. turning on/off a light, a relay, configuring a parameter, etc)
  thing["GPIO_21"] << digitalPin(21);

  // resource output example (i.e. reading a sensor value)
  thing["millis"] >> outputValue(millis());

  // more details at http://docs.thinger.io/arduino/
}

void loop() {
  thing.handle();
}