#include <Arduino.h>
#include "BLEDevice.h"

#define LED_PIN D7

static BLEUUID serviceUUID("6E400001-B5A3-F393-E0A9-E50E24DCCA9A");
static BLEUUID notifyUUID("6E400003-B5A3-F393-E0A9-E50E24DCCA9A");

static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;

static BLEAdvertisedDevice* myDevice;
static BLERemoteCharacteristic* pRemoteCharacteristic;

static void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify
) {
  String message = "";

  for (size_t i = 0; i < length; i++) {
    message += (char)pData[i];
  }

  Serial.print("Received from server: ");
  Serial.println(message);

  if (message == "Vibrating...") {
    digitalWrite(LED_PIN, HIGH);
    Serial.println("Vibrating");
    delay(500);
  } else {
    digitalWrite(LED_PIN, LOW);
  }
}

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
    Serial.println("Client connected");
  }

  void onDisconnect(BLEClient* pclient) {
    connected = false;
    Serial.println("Client disconnected");
    doScan = true;
  }
};

bool connectToServer() {
  Serial.print("Connecting to ");
  Serial.println(myDevice->getAddress().toString().c_str());

  BLEClient* pClient = BLEDevice::createClient();
  pClient->setClientCallbacks(new MyClientCallback());

  if (!pClient->connect(myDevice)) {
    Serial.println("Failed to connect");
    return false;
  }

  Serial.println("Connected to server");

  BLERemoteService* pRemoteService = pClient->getService(serviceUUID);

  if (pRemoteService == nullptr) {
    Serial.println("Failed to find service");
    pClient->disconnect();
    return false;
  }

  Serial.println("Found service");

  pRemoteCharacteristic = pRemoteService->getCharacteristic(notifyUUID);

  if (pRemoteCharacteristic == nullptr) {
    Serial.println("Failed to find notify characteristic");
    pClient->disconnect();
    return false;
  }

  Serial.println("Found notify characteristic");

  if (pRemoteCharacteristic->canNotify()) {
    pRemoteCharacteristic->registerForNotify(notifyCallback);
    Serial.println("Registered for notifications");
  }

  connected = true;
  return true;
}

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    Serial.print("Found: ");
    Serial.println(advertisedDevice.toString().c_str());

    if (advertisedDevice.haveName() &&
        advertisedDevice.getName() == "PaShield") {

      Serial.println("Found PaShield!");

      BLEDevice::getScan()->stop();

      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
      doScan = false;
    }
  }
};

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  Serial.println("Starting BLE client...");

  BLEDevice::init("");

  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks(), false);
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);
  pBLEScan->setActiveScan(true);

  Serial.println("Scanning...");
  pBLEScan->start(10, false);
}

void loop() {
  if (doConnect) {
    doConnect = false;

    if (connectToServer()) {
      Serial.println("Connected and waiting for notifications...");
    } else {
      Serial.println("Connection failed");
      doScan = true;
    }
  }

  if (!connected && doScan) {
    Serial.println("Restarting scan...");
    BLEDevice::getScan()->start(10, false);
    doScan = false;
  }

  delay(100);
}
