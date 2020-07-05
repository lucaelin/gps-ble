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
    client.println("POST /upload?device_id=tbeam1 HTTP/1.0");
    client.println("Content-Type: application/octet-stream");
    client.print("Content-Length: "); client.println((int) history.size());
    
    client.println();

    history.seek(0);
    while(history.available()) {
      client.write(history.read());
    }
    history.close();
    client.flush();
    delay(1000);
    
    while(client.available()) {
      Serial.write(client.read());
    }
    
    client.stop();
    
    delay(100);
  } else {
    Serial.println("WiFi connection failed!");
  }
  
  WiFi.mode(WIFI_OFF);
}
