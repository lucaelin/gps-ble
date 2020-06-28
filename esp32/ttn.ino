#include <TTN_esp32.h>
#include <SPI.h>
#include "data.h"
#include "ttn.config.h"

#define UNUSED_GPIO     TTN_esp32_LMIC::HalPinmap_t::UNUSED_PIN
#define SCK_GPIO        5
#define MISO_GPIO       19
#define MOSI_GPIO       27
#define NSS_GPIO        18
#define RESET_GPIO      23
#define DIO0_GPIO       26
#define DIO1_GPIO       33
#define DIO2_GPIO       32

const TTN_esp32_LMIC::HalPinmap_t pPinmap = {
  .nss = NSS_GPIO,
  .rxtx = UNUSED_GPIO,
  .rst = RESET_GPIO,
  .dio = {DIO0_GPIO, DIO1_GPIO, DIO2_GPIO},
};

TTN_esp32 ttn ;

void message(const uint8_t* payload, size_t size, int rssi)
{
    Serial.println("-- MESSAGE");
    Serial.print("Received " + String(size) + " bytes RSSI= " + String(rssi) + "dB");

    for (int i = 0; i < size; i++)
    {
        Serial.print(" " + String(payload[i]));
        // Serial.write(payload[i]);
    }

    Serial.println();
}

void setupTTN()
{
    Serial.println("Starting TTN");
    SPI.begin(SCK_GPIO, MISO_GPIO, MOSI_GPIO, NSS_GPIO);
    ttn.begin(&pPinmap);
    ttn.setDataRate(0); // SF12
    ttn.onMessage(message); // declare callback function when is downlink from server
    ttn.personalize(devAddr, nwkSKey, appSKey);
    ttn.setDataRate(0);
    ttn.showStatus();
}

int sendTTN(GpsData data) {
  Serial.print("Sending ");
  Serial.print((int) sizeof(GpsData));
  Serial.print(" bytes to ttn... ");
  int status = ttn.sendBytes((uint8_t*)&data, sizeof(GpsData));
  if (status) {
    Serial.println("done.");
  } else {
    Serial.println("error.");
  }
  return status;
}
