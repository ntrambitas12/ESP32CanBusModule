#include "intializeLists.h"

void intializeCommandsList(Command* commandsList) {
  //Remote start  
  strcpy(commandsList[0].commandName, "remoteStart");
  commandsList[0].canFrame.can_id = 0x1024E097 | CAN_EFF_FLAG;
  commandsList[0].canFrame.can_dlc = 3;
  commandsList[0].canFrame.data[0] = 0x80;
  commandsList[0].canFrame.data[1] = 0x01;
  commandsList[0].canFrame.data[2] = 0xFF;
  commandsList[0].lsBus = true;

  //Remote stop
  strcpy(commandsList[1].commandName, "remoteStop");
  commandsList[1].canFrame.can_id = 0x1024E097 | CAN_EFF_FLAG;
  commandsList[1].canFrame.can_dlc = 3;
  commandsList[1].canFrame.data[0] = 0x40;
  commandsList[1].canFrame.data[1] = 0x01;
  commandsList[1].canFrame.data[2] = 0xFF;
  commandsList[1].lsBus = true;


  //Unlock Doors
  strcpy(commandsList[2].commandName, "unlockDoors");
  commandsList[2].canFrame.can_id = 0x1024E097 | CAN_EFF_FLAG;
  commandsList[2].canFrame.can_dlc = 3;
  commandsList[2].canFrame.data[0] = 0x00;
  commandsList[2].canFrame.data[1] = 0x03;
  commandsList[2].canFrame.data[2] = 0xFF;
  commandsList[2].lsBus = true;


  //Lock Doors
  strcpy(commandsList[3].commandName, "lockDoors");
  commandsList[3].canFrame.can_id = 0x1024E097 | CAN_EFF_FLAG;
  commandsList[3].canFrame.can_dlc = 3;
  commandsList[3].canFrame.data[0] = 0x00;
  commandsList[3].canFrame.data[1] = 0x01;
  commandsList[3].canFrame.data[2] = 0xFF;
  commandsList[3].lsBus = true;


  //Open Trunk
  strcpy(commandsList[4].commandName, "trunkOpen");
  commandsList[4].canFrame.can_id = 0x1024E097 | CAN_EFF_FLAG;
  commandsList[4].canFrame.can_dlc = 3;
  commandsList[4].canFrame.data[0] = 0x02;
  commandsList[4].canFrame.data[1] = 0x00;
  commandsList[4].canFrame.data[2] = 0xFF;
  commandsList[4].lsBus = true;


  //Flash Hazards
  strcpy(commandsList[5].commandName, "hazardsOn");
  commandsList[5].canFrame.can_id = 0x1024E097 | CAN_EFF_FLAG;
  commandsList[5].canFrame.can_dlc = 3;
  commandsList[5].canFrame.data[0] = 0x0C;
  commandsList[5].canFrame.data[1] = 0x00;
  commandsList[5].canFrame.data[2] = 0xFF;
  commandsList[5].lsBus = true;


  //Hazards Stop
  strcpy(commandsList[6].commandName, "hazardsOff");
  commandsList[6].canFrame.can_id = 0x1024E097 | CAN_EFF_FLAG;
  commandsList[6].canFrame.can_dlc = 3;
  commandsList[6].canFrame.data[0] = 0x00;
  commandsList[6].canFrame.data[1] = 0x00;
  commandsList[6].canFrame.data[2] = 0xFF;
  commandsList[6].lsBus = true;


  //Panic On
  strcpy(commandsList[7].commandName, "panicOn");
  commandsList[7].canFrame.can_id = 0x1024E097 | CAN_EFF_FLAG;
  commandsList[7].canFrame.can_dlc = 3;
  commandsList[7].canFrame.data[0] = 0x3C;
  commandsList[7].canFrame.data[1] = 0x00;
  commandsList[7].canFrame.data[2] = 0xFF;
  commandsList[7].lsBus = true;


  //Panic Off
  strcpy(commandsList[8].commandName, "panicOff");
  commandsList[8].canFrame.can_id = 0x1024E097 | CAN_EFF_FLAG;
  commandsList[8].canFrame.can_dlc = 3;
  commandsList[8].canFrame.data[0] = 0x14;
  commandsList[8].canFrame.data[1] = 0x00;
  commandsList[8].canFrame.data[2] = 0xFF;
  commandsList[8].lsBus = true;

}

void intializeCarState(CarState* carState) {
  
  // Intitialize everything to 0
    carState->chargingCurrent = 0;
    carState->fuelRange = 0;
    strcpy(carState->transmissionRange, "");
    strcpy(carState->VIN, "TestVIN123");
    carState->coolantTemp = 0;
    carState->chargingVoltage = 0;
    carState->batteryChargePercent = 0;
    carState->vehicleSpeed = 0;
    carState->RPM = 0;
    carState->odometer = 0;
    carState->OAT = 0;
    carState->brakePressed = false;
    carState->ignitionStatus = false;
    carState->engineRunning = false;
    carState->doorLocked = false;
}

void intializePollingList(HSFramePoll* pollingList) {
  // Set the ID's to poll in the list here
  strcpy(pollingList[0].pollName, "fuelLvl");
  pollingList[0].can_id = 0x7E0;
  pollingList[0].payload[0] = 0x00;
  pollingList[0].payload[1] = 0x2F;

  strcpy(pollingList[1].pollName, "chargeCurrent");
  pollingList[1].can_id = 0x7E4;
  pollingList[1].payload[0] = 0x43;
  pollingList[1].payload[1] = 0x69;

  strcpy(pollingList[2].pollName, "chargeVoltage");
  pollingList[2].can_id = 0x7E4;
  pollingList[2].payload[0] = 0x43;
  pollingList[2].payload[1] = 0x68;

  strcpy(pollingList[3].pollName, "chargeLevel");
  pollingList[3].can_id = 0x7E4;
  pollingList[3].payload[0] = 0x83;
  pollingList[3].payload[1] = 0x34;

  strcpy(pollingList[4].pollName, "vehicleSpeed");
  pollingList[4].can_id = 0x7E0;
  pollingList[4].payload[0] = 0x00;
  pollingList[4].payload[1] = 0x0d;
}