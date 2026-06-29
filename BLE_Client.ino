#include <Arduino.h>
#include "BLEDevice.h"

#define TARGET_NAME "PaShield"

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    Serial.println("----------");
    Serial.println(advertisedDevice.toString().c_str());

    String address = advertisedDevice.getAddress().toString().c_str();

    Serial.print("Address: ");
    Serial.println(address);

    Serial.print("RSSI: ");
    Serial.println(advertisedDevice.getRSSI());

    if (advertisedDevice.haveName()) {
      String name = advertisedDevice.getName().c_str();

      Serial.print("Name: ");
      Serial.println(name);

      if (name == TARGET_NAME) {
        Serial.println("******** FOUND PASHIELD BY NAME ********");
      }
    } else {
      Serial.println("Name: <none>");
    }

    if (advertisedDevice.haveServiceUUID()) {
      Serial.print("Service UUID: ");
      Serial.println(advertisedDevice.getServiceUUID().toString().c_str());
    } else {
      Serial.println("Service UUID: <none>");
    }
  }
};

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("BLE scanner: looking for PaShield");
  BLEDevice::init("");

  BLEScan *scan = BLEDevice::getScan();
  scan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks(), false);
  scan->setActiveScan(true);
  scan->setInterval(100);
  scan->setWindow(99);
}

void loop() {
  Serial.println("Scanning for 10 seconds...");
  BLEDevice::getScan()->start(10, false);
  BLEDevice::getScan()->clearResults();
  delay(1000);
}