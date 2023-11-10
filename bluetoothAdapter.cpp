#include "bluetoothAdapter.h"
#include "Arduino.h"
#define BLE_SERVER_NAME "Your Vehicle"
#define SERVICE_UUID "91bad492-b950-4226-aa2b-4ede9fa42f59"
// Two characteristics are created. Reading can be done insecurely, writing is a secured operation
#define W_SECURE_CHARACTERISTIC "cba1d466-344c-4be3-ab3f-189f80dd7518"
#define R_CHARACTERISTIC "c8ca6af0-5c97-11ee-9b23-8b8ec8fa712a"
#define MAX_BLE_DEVICES 1 // Support up to 1 concurrent connections ?? Dont know why data gets corrupt when multiple devices want to read carstate :(
#define WRITE_UPDATE_BREAK 900 // pause for 300 MS

#include "bluetoothAdapter.h"

bool bluetoothAdapter::isDeviceConnected = false;
std::queue<std::string> bluetoothAdapter::commandsQueue;
SemaphoreHandle_t bluetoothAdapter::queueSemaphore;
StaticSemaphore_t bluetoothAdapter::queueSemaphoreBuffer;
SemaphoreHandle_t bluetoothAdapter::readSemaphore;
StaticSemaphore_t bluetoothAdapter::readSemaphoreBuffer;
SemaphoreHandle_t bluetoothAdapter::deviceConnectionSemaphore;
StaticSemaphore_t bluetoothAdapter::deviceConnectionSemaphoreBuffer;
int bluetoothAdapter::connectedDevices = 0;
unsigned long lastWriteTime;

void bluetoothAdapter::MyServerCallbacks::onConnect(BLEServer* pServer) {
    xSemaphoreTake(deviceConnectionSemaphore, portMAX_DELAY); 
    isDeviceConnected = true;
    connectedDevices++; // Increment number of connected devices
    Serial.println("Device connected, #" + String(connectedDevices));
    // // BLE Automatically stops advertising whenever a device connects
    if (connectedDevices < MAX_BLE_DEVICES) {
        BLEDevice::startAdvertising();
    }
    xSemaphoreGive(deviceConnectionSemaphore); 
}

void bluetoothAdapter::MyServerCallbacks::onDisconnect(BLEServer* pServer) {
    xSemaphoreTake(deviceConnectionSemaphore, portMAX_DELAY); 
    connectedDevices--;
    Serial.println("Device disconnected, #" + String(connectedDevices));
    // Check if any devices are still connected
    if (connectedDevices <= 0){
        isDeviceConnected = false;
    }
    //If not advertizing and under MAX BLE threshold, start advertizing again
    if (connectedDevices < MAX_BLE_DEVICES) {
        BLEAdvertising* pAdvertising = pServer->getAdvertising();
        pAdvertising->start();
    }
    xSemaphoreGive(deviceConnectionSemaphore); 
}

void bluetoothAdapter::MyCharacteristicReadCallbacks::onRead(BLECharacteristic* carDataCharacteristics) {
    //BUG: Issues with mutliple characterisitcs read
    // Callback for when the client requests to read data
    xSemaphoreTake(readSemaphore, portMAX_DELAY);
    //  std::string value = carDataCharacteristics->getValue();
    // Serial.println("Attempted read callback" + String(value.c_str()));
    xSemaphoreGive(readSemaphore);  // allow the next device to read
}

void bluetoothAdapter::MyCharacteristicWriteCallbacks::onWrite(BLECharacteristic* carDataCharacteristics) {
    // Callback for when the client writes data
    xSemaphoreTake(queueSemaphore, portMAX_DELAY); 
    Serial.println("client wrote data");
    commandsQueue.push(carDataCharacteristics->getValue());
    xSemaphoreGive(queueSemaphore); 
}

bluetoothAdapter::bluetoothAdapter() {
    // Initialize Bluetooth (Standard ESP32 BLE initialization)
    BLEDevice::init(BLE_SERVER_NAME); 
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    // Create custom service and set up characteristic for passkey pairing
    pService = pServer->createService(SERVICE_UUID);
    pCarLinkWriteCharacteristic = pService->createCharacteristic(
        W_SECURE_CHARACTERISTIC,
        BLECharacteristic::PROPERTY_WRITE
    );
    pCarLinkReadCharacteristic = pService->createCharacteristic(
        R_CHARACTERISTIC,
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
    );

    // Set security and encryption for characteristic
    pCarLinkWriteCharacteristic->setAccessPermissions(ESP_GATT_PERM_WRITE_ENCRYPTED);
    pCarLinkWriteCharacteristic->setCallbacks(new MyCharacteristicWriteCallbacks());
    // Configure the read characterisics settings
    pCarLinkReadCharacteristic->setCallbacks(new MyCharacteristicReadCallbacks());
    // Allow for Characteristic notifications
    BLEDescriptor* pCccDescriptor = new BLEDescriptor(BLEUUID((uint16_t)0x2902));
    pCarLinkReadCharacteristic->addDescriptor(pCccDescriptor);

    // Intialize semaphore
    queueSemaphore = xSemaphoreCreateBinaryStatic(&queueSemaphoreBuffer);
    deviceConnectionSemaphore = xSemaphoreCreateBinaryStatic(&deviceConnectionSemaphoreBuffer);
    readSemaphore = xSemaphoreCreateBinaryStatic(&readSemaphoreBuffer);
    // Initialize the semaphore to the "not taken" state
    xSemaphoreGive(queueSemaphore); 
    xSemaphoreGive(deviceConnectionSemaphore);
    xSemaphoreGive(readSemaphore);
    // Start advertising
    pService->start();
    BLEAdvertising* pAdvertising = pServer->getAdvertising();
    pAdvertising->start();
}

std::string bluetoothAdapter::getCommand() {
    xSemaphoreTake(queueSemaphore, portMAX_DELAY); 
    if (!commandsQueue.empty()) {
        std::string command = commandsQueue.front();
        xSemaphoreGive(queueSemaphore); 
        commandsQueue.pop();
        return command;
    }
    xSemaphoreGive(queueSemaphore); 
    return "";
}

void bluetoothAdapter::updateState(const char* state) {
if (millis() - lastWriteTime > WRITE_UPDATE_BREAK){
  pCarLinkReadCharacteristic->setValue(state);
  lastWriteTime = millis();
  //   pCarLinkReadCharacteristic->notify();
}

}
