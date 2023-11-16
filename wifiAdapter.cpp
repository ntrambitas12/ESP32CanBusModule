#include "wifiAdapter.h"
#include <WiFi.h>
#include <HTTPClient.h>

#define WIFI_TASK_SLEEP 5000 // Let wifi task sleep for 5 seconds before attempting to reconnect
#define VIN_LEN 18

// Semaphores and task handlers
TaskHandle_t wifiAdapter::connectToWifiTaskHandle = NULL;
SemaphoreHandle_t wifiAdapter::connectionSemaphore;
SemaphoreHandle_t wifiAdapter::APIVarsSemaphore;
StaticSemaphore_t wifiAdapter::connectionSemaphoreBuffer;
StaticSemaphore_t wifiAdapter::APIVarsSemaphoreBuffer;

// Initial var declarations
unsigned long wifiAdapter::wifiConnectionstartTime;
char* wifiAdapter::serverAddress = nullptr;
char* wifiAdapter::VIN = nullptr;
bool wifiAdapter::isConnected = false;
bool wifiAdapter::connectionInProgress = false;
char wifiAdapter::writeBuffer[MAX_WRITE_BUFF_SIZE] = {'\0'}; // Initialize writeBuffer to empty string
std::queue<std::string> wifiAdapter::commandsQueue;

// Helper thread to connect to WiFi without blocking
void wifiAdapter::connectToWifiTask(void* param) {
    // Tasks must be in an infinite loop, otherwise it will crash per RTOS docs: 
    while (true) {
        // Update the state of WIFI connection
        xSemaphoreTake(connectionSemaphore, portMAX_DELAY);
         wifiAdapter::isConnected = (WiFi.status() == WL_CONNECTED);
        // Method loops until wifi gets connected. Will autoreconnect to the passed in network till stopped
        if (!wifiAdapter::isConnected) {
                //Reset deviceConnected flag to false
                wifiAdapter::isConnected = false;
                WifiCredentials* credentials = (WifiCredentials*)param; 
                // Verify that credentials isn't null   
                 if (credentials != nullptr && credentials->ssid != nullptr && credentials->password != nullptr) {
                        // Attempt to connect
                        wifiAdapter::connectionInProgress = true;
                        Serial.println("DEBUG: WIFI SSID: " + String(credentials->password));
                        WiFi.begin(credentials->ssid, credentials->password);

                        // Continue trying to connect for a maximum of 15 seconds
                        const unsigned long connectionTimeout = 15000;
                        while ( (!wifiAdapter::isConnected && millis() - wifiAdapter::wifiConnectionstartTime < connectionTimeout))
                        {   
                            xSemaphoreGive(connectionSemaphore);
                            vTaskDelay(pdMS_TO_TICKS(WIFI_TASK_SLEEP)); // Let other threads run 
                            xSemaphoreTake(connectionSemaphore, portMAX_DELAY);
                        }
                        // Update connection result
                        wifiAdapter::isConnected = (WiFi.status() == WL_CONNECTED);
                        wifiAdapter::connectionInProgress = false; 
                        wifiConnectionstartTime = millis();
                        Serial.println("DEBUG: FINISHED ATTEMPT TO CONNECT TO WIFI: RESULT: " + String(isConnected));
                    } else {
                        // Credentias param was corrupted. Exit this thread
                        wifiAdapter::connectionInProgress = false;   
                        Serial.println("DEBUG: EXIT CONNECT TO WIFI TASK, CORRUPTED Params ");
                        xSemaphoreGive(connectionSemaphore);
                        // Free the memory of the Credentials struct
                        delete credentials;
                        connectToWifiTaskHandle = NULL;
                        vTaskDelete(NULL);
                    }
            }  
        xSemaphoreGive(connectionSemaphore);
        vTaskDelay(pdMS_TO_TICKS(WIFI_TASK_SLEEP)); // Let other threads run 
    }

}

// Helper thread to fetch from car over WiFi in the background
void wifiAdapter::fetchCommand(void* params) {
    const int refreshInterval = 400; // Refresh from internet every 400ms
     while (true) {
     String payload = ""; 
     xSemaphoreTake(APIVarsSemaphore, portMAX_DELAY);
     if (wifiAdapter::isConnected == true && VIN != nullptr && serverAddress != nullptr) {
        if (strlen(VIN) > 0 && strlen(serverAddress) > 0) {
          // Set up HTTP request
            WiFiClient client;
            HTTPClient http;
            String fetchCommand = String(wifiAdapter::serverAddress) + "/getCommand";
            http.begin(client, fetchCommand);
            http.addHeader("Content-Type", "application/json");
            http.addHeader("set-vin", wifiAdapter::VIN);
            int httpResponseCode = http.GET();
              // If successful, write payload
                if (httpResponseCode>0) {
                    //HTTP OK
                    payload = http.getString();
                    payload.replace("\"", "");
                    // Ensure that response is not empty
                    if (payload.length() > 0) {
                      wifiAdapter::commandsQueue.push(std::string(payload.c_str()));
                    }
                }
            // Free resources
            http.end();
        } 
      }
      xSemaphoreGive(APIVarsSemaphore);
      vTaskDelay(refreshInterval / portTICK_PERIOD_MS);
  }
}


// Helper thread to send status to server in the background over WiFi
void wifiAdapter::sendStatus(void* params) {
    const int refreshInterval = 900; // Refresh from car every 900ms
    while (true) {
        xSemaphoreTake(APIVarsSemaphore, portMAX_DELAY);
        if (wifiAdapter::isConnected && strlen(writeBuffer) > 0 && VIN != nullptr && serverAddress != nullptr) {
            if (strlen(VIN) > 0 && strlen(serverAddress) > 0) {
                WiFiClient client;
                HTTPClient http;
                String putCommand = String(wifiAdapter::serverAddress) + "/putStatus";
                http.begin(client, putCommand);
                http.addHeader("Content-Type", "application/json");
                http.addHeader("set-vin", wifiAdapter::VIN);
                int httpResponseCode = http.PUT((uint8_t*)writeBuffer, strlen(writeBuffer));
                if (httpResponseCode>0) {
                    //HTTP OK
                    // Take any actions needed after sending
                }
                // Free resources
                http.end();
            }
      }
        xSemaphoreGive(APIVarsSemaphore);
        vTaskDelay(refreshInterval / portTICK_PERIOD_MS);
    }
}

// Constructor + Destructor
wifiAdapter::wifiAdapter(const char* serverAddress) {
    connectionSemaphore = xSemaphoreCreateBinaryStatic(&connectionSemaphoreBuffer);
    APIVarsSemaphore = xSemaphoreCreateBinaryStatic(&APIVarsSemaphoreBuffer);
    // Initialize the semaphore to the "not taken" state
    xSemaphoreGive(connectionSemaphore);  
    xSemaphoreGive(APIVarsSemaphore);  
    // Save the serverAddress
    this->serverAddress = new char[strlen(serverAddress) + 1]; // Allocate new char in memory to hold serverAddress
    strcpy(this->serverAddress, serverAddress); // Copy string to memory
    this->VIN = new char[VIN_LEN]; // Allocate memory for the new variable
    // Construct private vars
    isConnected = false;
    connectionInProgress = false;
    wifiConnectionstartTime = 0;
    // Create task to fetch data from car every 2 seconds once connected
    xTaskCreate(
            fetchCommand,
            "fetchCommand",
            4000, // Stack size in Bytes
            NULL, // No pointer to pass
            1, // Task priority
            &fetchCommandHandle // Task handle
        );
    xTaskCreate(
            sendStatus,
            "sendStatus",
            9000, // Stack size in Bytes, made bigger to handle largest payload
            NULL, // No pointer to pass
            2, // Task priority
            &sendStatusHandle // Task handle
        );
}

wifiAdapter::~wifiAdapter() {
    disconnectFromNetwork(); 
    // Kill the running tasks and set their handels to NULL
    if (fetchCommandHandle != NULL){
        vTaskDelete(fetchCommandHandle);
        fetchCommandHandle = NULL;
    }

    if (sendStatusHandle != NULL) {
        vTaskDelete(sendStatusHandle);
        sendStatusHandle = NULL;
    }

    if (connectToWifiTaskHandle != NULL) {
    vTaskDelete(connectToWifiTaskHandle); 
    connectToWifiTaskHandle = NULL;
    }

    // Deallocate memory
    delete[] this->serverAddress;
    delete[] this->VIN;
}

// Implement public methods:

// This method will connect to the passed in wifi network. Calling code should call this only when wanting to change network!
void wifiAdapter::connectToNetwork(const char* ssid, const char* password) {
    // Ensure that a conncetion isn't already in progress
    if (!connectionInProgress) {
        // Record start time
        wifiConnectionstartTime = millis();

        // Delete the old connectToWifiTask if it exists
        if (connectToWifiTaskHandle != NULL) {
            vTaskDelete(connectToWifiTaskHandle);
            connectToWifiTaskHandle = NULL;
            // Attempt to disconnect from WIFI
            disconnectFromNetwork();
        }

        // Allocate new credentials
        WifiCredentials* credentials = new WifiCredentials; 
        strncpy(credentials->ssid, ssid, MAX_SSID_LENGTH);
        strncpy(credentials->password, password, MAX_PASSWORD_LENGTH);

        xTaskCreate(
            connectToWifiTask,
            "ConnectToWifi",
            2500, // Stack size in Bytes
            (void*)credentials, // Pass a pointer to credentials struct
            3, // Task priority
            &connectToWifiTaskHandle // Task handle
        );
    }
}
String wifiAdapter::getCommandWifi() {
    xSemaphoreTake(APIVarsSemaphore, portMAX_DELAY);
    if (!commandsQueue.empty()) {
        std::string command = commandsQueue.front();
        commandsQueue.pop();
        xSemaphoreGive(APIVarsSemaphore);
        return String(command.c_str());
    }
    xSemaphoreGive(APIVarsSemaphore);
    return "";
}
void wifiAdapter::updateState(const char* state) { 
    // Ensure that passed in state is less than MAX buffer size and that strings arent equal
    if (strlen(state) < MAX_WRITE_BUFF_SIZE && strcmp(state, writeBuffer) != 0) {
      xSemaphoreTake(APIVarsSemaphore, portMAX_DELAY);
      strcpy(writeBuffer, state); // Copy state into the write buffer
      xSemaphoreGive(APIVarsSemaphore);
    }
}
void wifiAdapter::disconnectFromNetwork() {
    if (isDeviceConnected()) {
        WiFi.disconnect();
        xSemaphoreTake(connectionSemaphore, portMAX_DELAY);
        connectionInProgress = false;
        // Update connection result
        isConnected = (WiFi.status() == WL_CONNECTED);
        xSemaphoreGive(connectionSemaphore);
    } 
}
bool wifiAdapter::isDeviceConnected() {
    bool connected = false;
    xSemaphoreTake(connectionSemaphore, portMAX_DELAY);
    connected = isConnected;
    xSemaphoreGive(connectionSemaphore);
    return connected;
}
// Method to set VIN
void wifiAdapter::setVIN(char* VIN) {
    // update the VIN only if it's different
    if((this->VIN == nullptr) || (strcmp(VIN, this->VIN) != 0)) {
        xSemaphoreTake(APIVarsSemaphore, portMAX_DELAY);
        strcpy(this->VIN, VIN); // Copy in the new VIN to memory
        xSemaphoreGive(APIVarsSemaphore);
        Serial.println("VIN SET");
    }
}
