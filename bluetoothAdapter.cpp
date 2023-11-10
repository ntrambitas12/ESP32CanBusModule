#include "bluetoothAdapter.h"
#include "Arduino.h"
#define BLE_SERVER_NAME "Your Vehicle"
#define SERVICE_UUID "91bad492-b950-4226-aa2b-4ede9fa42f59"
// Two characteristics are created. Reading can be done insecurely, writing is a secured operation
#define W_SECURE_CHARACTERISTIC "cba1d466-344c-4be3-ab3f-189f80dd7518"
#define R_CHARACTERISTIC "c8ca6af0-5c97-11ee-9b23-8b8ec8fa712a"

#include "bluetoothAdapter.h"

bool bluetoothAdapter::isDeviceConnected = false;
std::queue<std::string> bluetoothAdapter::commandsQueue;
SemaphoreHandle_t bluetoothAdapter::queueSemaphore;
StaticSemaphore_t bluetoothAdapter::queueSemaphoreBuffer;
\
void bluetoothAdapter::MyServerCallbacks::onConnect(BLEServer* pServer) {
    isDeviceConnected = true;
     BLEAdvertising* pAdvertising = pServer->getAdvertising();
     pAdvertising->stop(); // Stop advertising when connected
}

void bluetoothAdapter::MyServerCallbacks::onDisconnect(BLEServer* pServer) {
    isDeviceConnected = false;
    BLEAdvertising* pAdvertising = pServer->getAdvertising();
     pAdvertising->start(); // Start advertising again
}

void bluetoothAdapter::MyCharacteristicReadCallbacks::onRead(BLECharacteristic* carDataCharacteristics) {
    // Callback for when the client requests to read data
   
}

void bluetoothAdapter::MyCharacteristicWriteCallbacks::onWrite(BLECharacteristic* carDataCharacteristics) {
    // Callback for when the client writes data
    xSemaphoreTake(queueSemaphore, portMAX_DELAY); 
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
        BLECharacteristic::PROPERTY_READ
    );

    // Set security and encryption for characteristic
    pCarLinkWriteCharacteristic->setAccessPermissions(ESP_GATT_PERM_WRITE_ENCRYPTED);
    pCarLinkWriteCharacteristic->setCallbacks(new MyCharacteristicWriteCallbacks());
    // Configure the read characterisics settings
    pCarLinkReadCharacteristic->setCallbacks(new MyCharacteristicReadCallbacks());
    BLEDescriptor* pCccDescriptor = new BLEDescriptor(BLEUUID((uint16_t)0x2902));
    pCarLinkReadCharacteristic->addDescriptor(pCccDescriptor);

    // Intialize semaphore
    queueSemaphore = xSemaphoreCreateBinaryStatic(&queueSemaphoreBuffer);
    // Initialize the semaphore to the "not taken" state
    xSemaphoreGive(queueSemaphore); 
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
  pCarLinkReadCharacteristic->setValue(std::string(state));
}
