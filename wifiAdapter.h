#ifndef WIFI_ADAPTER_H
#define WIFI_ADAPTER_H
#include <queue>
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define MAX_SSID_LENGTH 32
#define MAX_PASSWORD_LENGTH 100
#define MAX_WRITE_BUFF_SIZE 5000

struct WifiCredentials {
        char ssid[MAX_SSID_LENGTH];
        char password[MAX_PASSWORD_LENGTH];
    };

class wifiAdapter {
private:
    TaskHandle_t fetchCommandHandle = NULL;
    TaskHandle_t sendStatusHandle = NULL;
    static TaskHandle_t connectToWifiTaskHandle;// Handle to dynamically control wifi connection loop
    static StaticSemaphore_t connectionSemaphoreBuffer;
    static StaticSemaphore_t APIVarsSemaphoreBuffer;
    static SemaphoreHandle_t connectionSemaphore; // mutex to protect WIFI connection status
    static SemaphoreHandle_t APIVarsSemaphore; // mutex to protect API (VIN and serverAddress) variables
    // These fields are static so that the helper tasks can access them.
    // There shouldn't be any issues at runtime but these need to be used with caution
    static unsigned long wifiConnectionstartTime;
    static char* serverAddress;
    static char* VIN;
    static char writeBuffer[MAX_WRITE_BUFF_SIZE]; // buffer to update status of car
    static bool isConnected;
    static bool connectionInProgress;
    static void connectToWifiTask(void* param);
    static void fetchCommand(void* params);
    static void sendStatus(void* params);


public:
    static std::queue<std::string> commandsQueue; // received commands from server
    wifiAdapter(const char* serverAddress);
    ~wifiAdapter();
    void connectToNetwork(const char* ssid, const char* password);
    void disconnectFromNetwork();
    String getCommandWifi();
    void updateState(const char* state);
    bool isDeviceConnected();
    void setVIN(char* VIN);
};


#endif // WIFI_ADAPTER_H