#include "wifi.h"

#include <WiFi.h>
#include <WiFiMulti.h>
#include <WiFiClient.h>
#include "wifi.config.h"

WiFiMulti wifiMulti;
WiFiClient client;
boolean didUpload = false;

void setupWIFI() {
  WiFi.mode(WIFI_OFF);
  wifiMulti.addAP(ap1, pw1);
  wifiMulti.addAP(ap2, pw2);
  wifiMulti.addAP(ap3, pw3);
}

void uploadWIFI() {
  Serial.println("Connecting wifi...");
  WiFi.mode(WIFI_STA);
  delay(1000);
  uint32_t count = 0;
  while (count < 100 && wifiMulti.run() != WL_CONNECTED) {
    delay(100);
    count++;
  }
  
  if(wifiMulti.run() == WL_CONNECTED) {
    Serial.println("WiFi upload");
    //Serial.println("IP address: ");
    //Serial.println(WiFi.localIP());
    
    File history = getLatestFile();

    Serial.print("history upload with ");
    Serial.print((int) history.size());
    Serial.println(" bytes");

    client.connect(server, port);

    // TODO dynamic device id
    client.println("POST /upload?device_id=tbeam1 HTTP/1.1");
    client.print("Host: "); client.print(server); client.print(":"); client.println(String(port));
    client.println("Content-Type: application/octet-stream");
    client.print("Content-Length: "); client.println((int) history.size());
    
    client.println();

    history.seek(0);
    while(history.available()) {
      client.write(history.read());
    }
    history.close();
    client.flush();

    uint32_t count = 0;
    while(count < 100 && client.available()<18) {
      delay(100);
      count++;
    }
    
    char response[18];
    Serial.readBytes(response, 18);
    Serial.write(response);
    while(client.available()) {
      Serial.write(client.read());
    }
    client.stop();

    if (strncmp(response, "HTTP/1.1 200 OK\r\n", 18) == 0) {
      Serial.println("Upload success!");
    } else {
      Serial.println("Upload failed!");
    }
    
    delay(100);
  } else {
    Serial.println("WiFi connection failed!");
  }
  
  WiFi.mode(WIFI_OFF);
}
