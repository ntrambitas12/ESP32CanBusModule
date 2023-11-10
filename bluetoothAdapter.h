#ifndef BLUETOOTH_ADAPTER_H
#define BLUETOOTH_ADAPTER_H

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <queue>
#include <string>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

class bluetoothAdapter {
private:
    BLEServer* pServer;
    BLEService* pService;
    BLECharacteristic* pCarLinkWriteCharacteristic;
    BLECharacteristic* pCarLinkReadCharacteristic;
    static std::queue<std::string> commandsQueue;
    static StaticSemaphore_t queueSemaphoreBuffer;
    static SemaphoreHandle_t queueSemaphore; // mutex to from queue being incorrect


    class MyServerCallbacks : public BLEServerCallbacks {
        void onConnect(BLEServer* pServer);
        void onDisconnect(BLEServer* pServer);

    };

    class MyCharacteristicWriteCallbacks : public BLECharacteristicCallbacks {
        void onWrite(BLECharacteristic* carDataCharacteristics);
    };

     class MyCharacteristicReadCallbacks : public BLECharacteristicCallbacks {
        void onRead(BLECharacteristic* carDataCharacteristics);
    };

public:
    bluetoothAdapter();
    static bool isDeviceConnected;
    std::string getCommand();
    void updateState(const char* state);
};

#endif  // BLUETOOTH_ADAPTER_H
