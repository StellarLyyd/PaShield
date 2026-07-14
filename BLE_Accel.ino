#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <iostream>
#include <cmath>

using namespace std;

Adafruit_MPU6050 mpu;

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
  pinMode(D7, OUTPUT);

  // Try to initialize!
  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
    while (1) {
      delay(10);
    }
  }

  mpu.setAccelerometerRange(MPU6050_RANGE_16_G);
  mpu.setGyroRange(MPU6050_RANGE_250_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
  Serial.println("");
  delay(100);

  // SERVER SETUP

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

    // Get new sensor events with the readings
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);
    digitalWrite(D7, LOW);

    String message = "AccelX: " + String(a.acceleration.x) +
                    ", AccelY: " + String(a.acceleration.y) +
                    ", AccelZ: " + String(a.acceleration.z) +
                    ", Total Accel: " + String(abs(a.acceleration.x) + abs(a.acceleration.y) + abs(a.acceleration.z));
    // ignoring gyroscope measurements for now
   
    Serial.println(message);
    
    if (abs(a.acceleration.x) + abs(a.acceleration.y) + abs(a.acceleration.z) > 30.0) {
      Serial.println("Vigorous motion detected. Vibrating.");

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
