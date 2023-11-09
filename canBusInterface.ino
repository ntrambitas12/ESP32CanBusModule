#include "mcp2515.h"
#include "intializeLists.h"
#include "bluetoothAdapter.h"
#include "wifiAdapter.h"
#include "helperFunctions.h"


#define pollListSize 5 //Adjust this based on the number of items to poll on HSBUS
#define commandsListSize 10 //Adjust this based on the number of supported commands

/*Adjust these to match the ESP32 board*/
#define MOSI_PIN 18
#define MISO_PIN 19
#define SCLK_PIN 5
#define CS_PIN_HSBUS  13 // CS Pin for where MCP2515 connects to ESP32
#define CS_PIN_LSBUS  12 // CS Pin for where MCP2515 connects to ESP32
#define EEPROM_SIZE 512

// Global Variables
MCP2515 hsbus(MOSI_PIN, MISO_PIN, SCLK_PIN, CS_PIN_HSBUS);
MCP2515 lsbus(MOSI_PIN, MISO_PIN, SCLK_PIN, CS_PIN_LSBUS);
CarState carState;
HSFramePoll pollingList[pollListSize];
Command commandsList[commandsListSize];
SemaphoreHandle_t mutex; // Mutex to control access to CANBUS
bluetoothAdapter* BTAdapter;


//WIFI info
// const char* ssid = "OnePlus10";
// const char* password = "2228171973";

//Your Domain name with URL path or IP address with path
const char* serverName = "http://192.168.255.150:3000/vci/getCommand";


void setup() {
  Serial.begin(9600);
  Serial.println("Started");
  // Bluetooth Object
  BTAdapter = new bluetoothAdapter();
  intializeCarState(&carState);
  intializePollingList(pollingList);
  intializeCommandsList(commandsList);
  
  // intialize the hsbus can transmitter
  hsbus.setBitrate(CAN_500KBPS, MCP_8MHZ);
  hsbus.setNormalMode();
  // intialize the lsbus can transmitter
  lsbus.setBitrate(CAN_33KBPS, MCP_8MHZ);
  lsbus.setNormalMode();
 
  // Intialize mutex
  mutex = xSemaphoreCreateMutex();
  //Setup tasks
  xTaskCreate(
    pollHSCAN, // Function to call
    "Poll HSCAN Bus", // Name of task
    2000, // Stack size in bytes
    NULL, // Parameter to pass
    2, // Task priority
    NULL // Task Handle
  );
  xTaskCreate(
    incomingCAN, // Function to call
    "Read CAN State", // Name of task
    2000, // Stack size in bytes
    NULL, // Parameter to pass
    3, // Task priority
    NULL // Task Handle
  );
  
}


void loop() {
  // Main thread of execution. Write code here to ping server, send user requests to car, and update state
  if (Serial.available()) {
    String command = Serial.readString();
    sendCommand(command.c_str());
    printData(&carState);
  }

  // Check if any command was received via BT
  std::string BTcommand = BTAdapter->getCommand();
  if (BTcommand.length() > 0) {
    // Do something with the command...
    Serial.println(BTcommand.c_str());
    sendCommand(BTcommand.c_str());
    }

  // Send out the updated state via Bluetooth
  String jsonSerialized = serializeCarStateToJson(&carState);
  BTAdapter->updateState(jsonSerialized.c_str());
    

   
}

/* Helper Functions */
void sendCommand(const char *command) {
  // Iterate list of possible commands
  for (int i = 0; i < commandsListSize; i++) {
    if ( strcmp(commandsList[i].commandName, command) == 0) {
      // Match found, take the mutex
      if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE){
        // Figure out which channel to send on
        if (commandsList[i].lsBus) {
          lsbus.sendMessage(&(commandsList[i].canFrame));
        } else {
          hsbus.sendMessage(&(commandsList[i].canFrame));
        } 
        xSemaphoreGive(mutex);
      }
      break; // If found, no need to continue searching. Break
    }
  }
}
void incomingCAN(void * parameter) {
  while(true) {
    if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE) {
      // Read the incoming messages on the HSCAN 
      struct can_frame canMsg;
      if (hsbus.readMessage(&canMsg)  == MCP2515::ERROR_OK) {
        proccessHSCAN(&carState, &canMsg);
      }
      if (lsbus.readMessage(&canMsg)  == MCP2515::ERROR_OK) {
        proccessLSCAN(&carState, &canMsg);
      }
      xSemaphoreGive(mutex);
    }
  }
}

void pollHSCAN(void * parameter) {
  while(true) {
    static int pollIdx = 0;
    if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE) {
      struct can_frame hsPollMsg = {
      .can_id = 0x00,
      .can_dlc = 4,
      .data = {0x03, 0x22, 0x00, 0x00},
      };
      // copy the contents from each poll into the hsPoll can frame and send it to the car
      hsPollMsg.can_id = pollingList[pollIdx].can_id;
      hsPollMsg.data[2] = pollingList[pollIdx].payload[0];
      hsPollMsg.data[3] = pollingList[pollIdx].payload[1];
      hsbus.sendMessage(&hsPollMsg);
      pollIdx++;
      if (pollIdx >= pollListSize) {
        pollIdx = 0;
      }
      xSemaphoreGive(mutex);
      vTaskDelay(400 / portTICK_PERIOD_MS); // Delay task for 400ms
    }
  }
}




