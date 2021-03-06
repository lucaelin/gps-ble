#include "wifi.h"

#include "file.h"
#include <WiFi.h>
#include <WiFiMulti.h>
#include <WiFiClient.h>
#include "wifi.config.h"
#include "esp_wifi.h"

WiFiMulti wifiMulti;
WiFiClient client;
boolean didUpload = false;

void setupWIFI() {
  wifiMulti.addAP(ap1, pw1);
  wifiMulti.addAP(ap2, pw2);
  wifiMulti.addAP(ap3, pw3);
}

void uploadWIFI() {
  Serial.println("Connecting wifi...");

  uint32_t count = 0;
  while (count < 100 && wifiMulti.run() != WL_CONNECTED) {
    delay(100);
    count++;
  }

  if(wifiMulti.run() == WL_CONNECTED) {
    Serial.println("WiFi upload");
    //Serial.println("IP address: ");
    //Serial.println(WiFi.localIP());

    client.connect(server, port);
    
    uint32_t gpsLogSize = getGPSLogSize();
    Serial.print("history upload with ");
    Serial.print(gpsLogSize);
    Serial.println(" bytes");

    // TODO dynamic device id
    client.println("POST /upload?source=wifi&dev_id=tbeam1 HTTP/1.1");
    client.print("Host: "); client.print(server); client.print(":"); client.println(String(port));
    client.println("Content-Type: application/octet-stream");
    client.print("Content-Length: "); client.println(gpsLogSize);

    client.println();

    while(client.available()) {
      client.read();
    }

    readGPSLog(&client);
    client.flush();

    Serial.println("Awaiting response...");

    uint32_t count = 0;
    while(count < 200 && client.available()<=16) {
      delay(100);
      count++;
    }

    if(!client.available()) {
      Serial.println("No response from server!");
      return;
    }

    char* response = "HTTP/1.1 200 OK\r\n";
    for(uint8_t i = 0; i<=16; i++) {
      char c = (char) client.read();
      Serial.print(c);
      if (response[i] != c) {
        while (client.available()) Serial.write(client.read());
        Serial.println("\nUnexpected HTTP response!");
        return;
      }
    }
    while (client.available()) Serial.write(client.read());
    Serial.println("\nUpload success!");

    while (deleteOldestFile()) ;
    Serial.println("\nDeleted old files!");

    delay(100);
  } else {
    Serial.println("WiFi connection failed!");
  }
}
