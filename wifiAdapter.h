#ifndef WIFI_ADAPTER_H
#define WIFI_ADAPTER_H
#include <queue>
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <mutex>

struct WifiCredentials {
        char* ssid;
        char* password;
    };

class wifiAdapter {
private:
    TaskHandle_t fetchCommandHandle = NULL;
    TaskHandle_t sendStatusHandle = NULL;
     // These fields are static so that the helper tasks can access them.
    // There shouldn't be any issues at runtime but these need to be used with caution
    static unsigned long wifiConnectionstartTime;
    static char* serverAddress;
    static char* VIN;
    static bool isConnected;
    static bool connectionInProgress;
    static void connectToWifiTask(void* param);
    static void fetchCommand(void* params);
    static void sendStatus(void* params);
    static std::mutex commandsQueueMutex;
    static std::mutex statusQueueMutex;
    static TaskHandle_t connectToWifiTaskHandle; // Handle to dynamically control wifi connection loop
    static WifiCredentials* ClassCredentials; // Class variable to be able to access wifi credentials
    static bool autoReconnectFlag;
    static std::mutex connectionStateMutex; // mutex to protect network connection vars

public:
    static std::queue<String> commandsQueue; // received commands from server
    static std::queue<String> statusQueue; // status to send back to server
    wifiAdapter(char* serverAddress);
    ~wifiAdapter();
    void connectToNetwork(char* ssid, char* password);
    void disconnectFromNetwork();
    String getCommandWifi();
    void updateState(const char* state);
    bool isDeviceConnected();
    void setVIN(char* VIN);
};


#endif // WIFI_ADAPTER_H