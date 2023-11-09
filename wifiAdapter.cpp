#include "wifiAdapter.h"
#include <WiFi.h>
#include <HTTPClient.h>

TaskHandle_t wifiAdapter::connectToWifiTaskHandle = NULL; // Initial state of wifiTask
WifiCredentials* wifiAdapter::ClassCredentials = new WifiCredentials;
bool wifiAdapter::autoReconnectFlag = false; // Set autoReconnectFlag to false initially

// Helper thread to connect to WiFi without blocking
void wifiAdapter::connectToWifiTask(void* param) {
    std::unique_lock<std::mutex> lock(connectionStateMutex, std::defer_lock); // using mutex to ensure that these variables remain consistent
    // Reset deviceConnected flag to false
    wifiAdapter::isConnected = false;
    wifiAdapter::autoReconnectFlag = false;
    WifiCredentials* credentials = (WifiCredentials*)param;
    
    if (credentials != nullptr && credentials->ssid != nullptr && credentials->password != nullptr) {
        while (!wifiAdapter::isConnected) {
            lock.lock(); // Aquire lock before starting the connection
            // Attempt to connect
            wifiAdapter::connectionInProgress = true;
            WiFi.begin(credentials->ssid, credentials->password);

            // Continue trying to connect for a maximum of 15 seconds
            const unsigned long connectionTimeout = 15000;
            while (!lock.owns_lock() || (!wifiAdapter::isConnected && millis() - wifiAdapter::wifiConnectionstartTime < connectionTimeout))
            {   lock.unlock(); // Release lock so that other threads can
                vTaskDelay(pdMS_TO_TICKS(1000)); // Let other threads run 
                lock.lock(); // Reacquire lock
            }

            // Update connection result
            wifiAdapter::isConnected = (WiFi.status() == WL_CONNECTED);
            // Wifi is connected. Save the currently connected network to class credentials. This will allow for an easy reconnect
            if (wifiAdapter::isConnected) {
                strcpy(ClassCredentials->ssid, credentials->ssid);
                strcpy(ClassCredentials->password, credentials->password);
                wifiAdapter::autoReconnectFlag = true;
            } else {
                wifiAdapter::connectionInProgress = false; 
                const unsigned long retryTimeout = 35000;
                lock.unlock(); 
                vTaskDelay(pdMS_TO_TICKS(retryTimeout)); // Stop trying to connect
                lock.lock();
            }
        }
    }
    
    wifiAdapter::connectionInProgress = false;   
    // Free the memory of the Credentials struct
    delete credentials;
    lock.unlock();
}

// Helper thread to fetch from car over WiFi in the background
void wifiAdapter::fetchCommand(void* params) {
    std::lock_guard<std::mutex> lock(connectionStateMutex); // Using lock to ensure that other threads don't change connectionStatus
    const int refreshInterval = 200; // Refresh from car every 200ms
     while (true) {
     String payload = ""; 
     if (wifiAdapter::isConnected && VIN != nullptr && serverAddress != nullptr) {
        if (strlen(VIN) > 0 && strlen(serverAddress) > 0) {
            WiFiClient client;
            HTTPClient http;

            http.begin(client, wifiAdapter::serverAddress);
            http.addHeader("Content-Type", "application/json");
            http.addHeader("set-vin", wifiAdapter::VIN);
            int httpResponseCode = http.GET();

                if (httpResponseCode>0) {
                    //HTTP OK
                    std::lock_guard<std::mutex> lock(commandsQueueMutex); // Using lock to ensure that data written to queue is consistent
                    payload = http.getString();
                    payload.replace("\"", "");
                    wifiAdapter::commandsQueue.push(payload);
                }
            // Free resources
            http.end();
        } 
      }
      // If disconnected from wifi, check if to reconnect
      else if (wifiAdapter::autoReconnectFlag && !wifiAdapter::connectionInProgress) {
         // Delete the old connectToWifiTask if it exists
        if (connectToWifiTaskHandle != NULL) {
            vTaskDelete(connectToWifiTaskHandle);
            connectToWifiTaskHandle = NULL;
        }

        // Create a new task to try to reconnect to the last known wifi network
        xTaskCreate(
            connectToWifiTask,
            "ConnectToWifi",
            1000, // Stack size in Bytes
            (void*)ClassCredentials, // Pass a pointer to the last saved ClassCredentials
            2, // Task priority
            &connectToWifiTaskHandle // Task handle
        );
      }
      vTaskDelay(refreshInterval / portTICK_PERIOD_MS);
  }
}

// Helper thread to send status to server in the background over WiFi
void wifiAdapter::sendStatus(void* params) {
    const int refreshInterval = 300; // Refresh from car every 300ms
    while (true) {
        if (wifiAdapter::isConnected && !wifiAdapter::statusQueue.empty() && VIN != nullptr && serverAddress != nullptr) {
            if (strlen(VIN) > 0 && strlen(serverAddress) > 0) {
                std::lock_guard<std::mutex> lock(statusQueueMutex); // Using lock to ensure that the statusQueue doesn't get modified during this operation
                WiFiClient client;
                HTTPClient http;

                http.begin(client, wifiAdapter::serverAddress);
                http.addHeader("Content-Type", "application/json");
                http.addHeader("set-vin", wifiAdapter::VIN);
                int httpResponseCode = http.POST(wifiAdapter::statusQueue.front());
                if (httpResponseCode>0) {
                    //HTTP OK
                    wifiAdapter::statusQueue.pop();
                }
                // Free resources
                http.end();
            }
      }
        vTaskDelay(refreshInterval / portTICK_PERIOD_MS);
    }
}

// Constructor + Destructor
wifiAdapter::wifiAdapter(char* serverAddress) {
    // Save the serverAddress
    this->serverAddress = new char[strlen(serverAddress) + 1]; // Allocate new char in memory to hold serverAddress
    strcpy(this->serverAddress, serverAddress); // Copy string to memory
    // Construct private vars
    isConnected = false;
    connectionInProgress = false;
    wifiConnectionstartTime = 0;
    // Create task to fetch data from car every 2 seconds once connected
    xTaskCreate(
            fetchCommand,
            "fetchCommand",
            1000, // Stack size in Bytes
            NULL, // No pointer to pass
            1, // Task priority
            &fetchCommandHandle // Task handle
        );
    xTaskCreate(
            sendStatus,
            "sendStatus",
            1000, // Stack size in Bytes
            NULL, // No pointer to pass
            1, // Task priority
            &sendStatusHandle // Task handle
        );
}
wifiAdapter::~wifiAdapter() {
    disconnectFromNetwork(); 
    // Kill the running tasks
    vTaskDelete(fetchCommandHandle);
    vTaskDelete(sendStatusHandle);

    // Deallocate memory
    delete[] this->serverAddress;
    delete[] this->VIN;
    delete ClassCredentials;
}



// Implement public methods
// This method will connect to the passed in wifi network. If already connected to the network passed in, then no further action will occur
void wifiAdapter::connectToNetwork(char* ssid, char* password) {
std::lock_guard<std::mutex> lock(connectionStateMutex); 
    // check if the new network to connect to is diffrent than the current class credentials
    if ((strcmp(ClassCredentials->ssid, ssid) != 0) && ((strcmp(ClassCredentials->password, password) != 0))) {
        // attempting to connect to a new network
        // cancel current connection and disconnect
        connectionInProgress = false;
        disconnectFromNetwork();
        autoReconnectFlag = false;
    }
     // Ensure that a conncetion isn't already in progress
    if (!connectionInProgress) {
        // Record start time
        wifiConnectionstartTime = millis();

        // Delete the old connectToWifiTask if it exists
        if (connectToWifiTaskHandle != NULL) {
            vTaskDelete(connectToWifiTaskHandle);
            connectToWifiTaskHandle = NULL;
        }

        // Allocate new credentials
        WifiCredentials* credentials = new WifiCredentials; 
        credentials->ssid = ssid;
        credentials->password = password;
        xTaskCreate(
            connectToWifiTask,
            "ConnectToWifi",
            1000, // Stack size in Bytes
            (void*)credentials, // Pass a pointer to credentials struct
            2, // Task priority
            &connectToWifiTaskHandle // Task handle
        );
    }
}
String wifiAdapter::getCommandWifi() {
    std::lock_guard<std::mutex> lock(commandsQueueMutex); // Using lock to ensure that the status returned is correct
    if (!commandsQueue.empty()) {
        String command = commandsQueue.front();
        commandsQueue.pop();
        return command;
    }
    return "";
}
void wifiAdapter::updateState(const char* state) {
    String stateStr = String (state);
    std::lock_guard<std::mutex> lock(statusQueueMutex);
    statusQueue.push(stateStr);
}
void wifiAdapter::disconnectFromNetwork() {
    std::lock_guard<std::mutex> lock(connectionStateMutex); // using mutex to ensure that isConnected gets approprialty updated
    WiFi.disconnect();
    // Update connection result
    isConnected = (WiFi.status() == WL_CONNECTED);
}

bool wifiAdapter::isDeviceConnected() {
    std::lock_guard<std::mutex> lock(connectionStateMutex); // using mutex to ensure that isConnected gets approprialty read
    return isConnected;
}
// Method to set VIN
void wifiAdapter::setVIN(char* VIN) {
    // update the VIN only if it's different
    if((this->VIN == nullptr) || (strcmp(VIN, this->VIN) != 0)) {
        delete[] this->VIN; // Delete the old VIN from memory
        this->VIN = new char[strlen(VIN) + 1]; // Allocate memory for the new variable
        strcpy(this->VIN, VIN); // Copy in the new VIN to memory
    }
}
