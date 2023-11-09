#include "carStateStruct.h" // Include the header for CarState definition
#include <stdio.h>

// HSCAN Function prototypes
void proccessHSCAN(CarState* carState, struct can_frame* frame);
void calcFuelRange(CarState* carState, struct can_frame* frame);
void calcTransRange(CarState* carState, struct can_frame* frame);
void calcIgnStatus(CarState* carState, struct can_frame* frame);
void calcEngineStatus(CarState* carState, struct can_frame* frame);
void calcBrakePedalStatus(CarState* carState, struct can_frame* frame);
void calcTempInfo(CarState* carState, struct can_frame* frame);
void calcChargingCurrent(CarState* carState, struct can_frame* frame);
void calcChargingVoltage(CarState* carState, struct can_frame* frame);
void calcChargeLvl(CarState* carState, struct can_frame* frame);
void calcVehicleSpeed(CarState* carState, struct can_frame* frame);
void calcOdo(CarState* carState, struct can_frame* frame);
void calcVIN(CarState* carState, struct can_frame* frame);

// LSCAN Function prototypes
void proccessLSCAN(CarState* carState, struct can_frame* frame);
void calcDoorLockStatus(CarState* carState, struct can_frame* frame);
