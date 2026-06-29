#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <Wire.h>

const int GSR = A3;

int sensorValue = 0;
int gsr_average = 0;
int moving_avg = 0;

int moving_average_array[10] = {0};
int indexPos = 0;

#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9A"
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9A"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9A"

BLEServer *pServer = nullptr;
BLECharacteristic *pTxCharacteristic = nullptr;

bool deviceConnected = false;
bool oldDeviceConnected = false;

class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *pServer) {
    deviceConnected = true;
    Serial.println("Client connected");
  }

  void onDisconnect(BLEServer *pServer) {
    deviceConnected = false;
    Serial.println("Client disconnected");
  }
};

class MyRxCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    String rxValue = pCharacteristic->getValue();

    if (rxValue.length() > 0) {
      Serial.print("Received from client: ");
      Serial.println(rxValue);
    }
  }
};

void setup() {
  Serial.begin(115200);
  pinMode(GSR, INPUT);
  pinMode(D7, OUTPUT);

  Wire.begin();

  BLEDevice::init("PaShield");

  Serial.print("BLE MAC: ");
  Serial.println(BLEDevice::getAddress().toString().c_str());

  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);

  pTxCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID_TX,
    BLECharacteristic::PROPERTY_NOTIFY
  );
  pTxCharacteristic->addDescriptor(new BLE2902());

  BLECharacteristic *pRxCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID_RX,
    BLECharacteristic::PROPERTY_WRITE
  );
  pRxCharacteristic->setCallbacks(new MyRxCallbacks());

  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->start();

  Serial.println("BLE server started. Waiting for client...");
}

void loop() {
  if (deviceConnected) {
    long sum = 0;

    for (int i = 0; i < 10; i++) {
      sensorValue = analogRead(GSR);
      sum += sensorValue;
      delay(5);
    }

    gsr_average = sum / 10;

    moving_average_array[indexPos] = gsr_average;
    indexPos++;

    if (indexPos >= 10) {
      indexPos = 0;
    }

    long sum2 = 0;
    for (int i = 0; i < 10; i++) {
      sum2 += moving_average_array[i];
    }

    moving_avg = sum2 / 10;

    int diff = abs(moving_avg - gsr_average);
    digitalWrite(D7, LOW);

    String message = "GSR=" + String(gsr_average) +
                     ", AVG=" + String(moving_avg) +
                     ", DIFF=" + String(diff);

    Serial.println(message);
    //Serial.println(gsr_average);

    if (diff > 100 && gsr_average < 2000 && gsr_average > 1000) {
      Serial.println("Significant change detected. Vibrating.");

      digitalWrite(D7, HIGH);
      
      message = "Vibrating...";
      delay(500);
    }


    pTxCharacteristic->setValue(message.c_str());
    pTxCharacteristic->notify();

    delay(100);
  }

  if (!deviceConnected && oldDeviceConnected) {
    delay(500);
    BLEDevice::startAdvertising();
    Serial.println("Restart advertising...");
    oldDeviceConnected = false;
  }

  if (deviceConnected && !oldDeviceConnected) {
    oldDeviceConnected = true;
  }
}
