#include <SPI.h>
#include <mcp2515.h>
#include "updateCANState.h"


void proccessHSCAN(CarState* carState, struct can_frame* frame) {
  calcFuelRange(carState, frame);
  calcTransRange(carState, frame);
  calcIgnStatus(carState, frame);
  calcEngineStatus(carState, frame);
  calcBrakePedalStatus(carState, frame);
  calcTempInfo(carState, frame);
  calcChargingCurrent(carState, frame);
  calcChargingVoltage(carState, frame);
  calcChargeLvl(carState, frame);
  calcVehicleSpeed(carState, frame);
  calcOdo(carState, frame);
  calcVIN(carState, frame);
}

void calcFuelRange(CarState* carState, struct can_frame* frame) {
  if (frame->can_id == 0x7E8 && frame->data[3] == 0x2F) {
    carState->fuelRange = frame->data[4]; // Fuel Range is always returned in miles
  }
}

void calcTransRange(CarState* carState, struct can_frame* frame) {
  if (frame->can_id == 0x1F5) {
    switch(frame->data[3]) {
      case 0x01:
        strcpy(carState->transmissionRange, "Park");
        break;
      case 0x02:
        strcpy(carState->transmissionRange, "Reverse");
        break;
      case 0x03:
        strcpy(carState->transmissionRange, "Neutral");
        break;
      case 0x04:
        strcpy(carState->transmissionRange, "Drive");
        break;
      case 0x06:
        strcpy(carState->transmissionRange, "Low");
        break;
      default:
        break;
    }
  }
}

void calcIgnStatus(CarState* carState, struct can_frame* frame) {
  if (frame->can_id == 0x0C9) {
    bool ignitionStatus = (frame->data[0] == 0x80);
    carState->ignitionStatus = ignitionStatus;
    carState->RPM = ((frame->data[1]) * 100);
  }
}

void calcEngineStatus(CarState* carState, struct can_frame* frame) {
  if (frame->can_id == 0x1EF) {
    carState->engineRunning = (frame->data[2] != 0x00);
  }
}

void calcBrakePedalStatus(CarState* carState, struct can_frame* frame) {
  if (frame->can_id == 0x0D1) {
    carState->brakePressed = (frame->data[1] != 0x00);
  }
}

void calcTempInfo(CarState* carState, struct can_frame* frame) {
  if (frame->can_id == 0x4C1) {
    carState->OAT = ((frame->data[4]/2) - 0x28); // This value is in C
    carState->coolantTemp = ((frame->data[2] - 0x28)); // This value is in C
  }
}

void calcChargingCurrent(CarState* carState, struct can_frame* frame) {
  if (frame->can_id == 0x7EC && frame->data[3] == 0x69) {
    carState->chargingCurrent = (frame->data[4] / 5);
  }
}

void calcChargingVoltage(CarState* carState, struct can_frame* frame) {
  if (frame->can_id == 0x7EC && frame->data[3] == 0x68) {
    carState->chargingVoltage = (frame->data[4] * 2);
  }
}

void calcChargeLvl(CarState* carState, struct can_frame* frame) {
  if (frame->can_id == 0x7EC && frame->data[3] == 0x34) {
    carState->batteryChargePercent = ((frame->data[4]) - 155);
  }
}

void calcVehicleSpeed(CarState* carState, struct can_frame* frame) {
  if (frame->can_id == 0x7E8 && frame->data[3] == 0x0d) {
    carState->vehicleSpeed = frame->data[4];
  }
}

void calcOdo(CarState* carState, struct can_frame* frame) {
  if (frame->can_id == 0x120) {
    /* Odometer is in KM */
   long odo = 0;

    for (int i = 0; i < 4; i++) {
        odo <<= 8; // Shift the current value left by 8 bits
        odo |= frame->data[i]; // OR with the current byte
    }
    carState->odometer = odo/64; // Divide by 64 to get the milage in KM
  }
}

void calcVIN(CarState* carState, struct can_frame* frame) {
  if (frame->can_id == 0x514) {
    // Append 1 as the first digit of the VIN
    carState->VIN[0] = '1';
    // Now write in characters 2 to 9 of the VIN
    for (int i = 1; i < 9; i++) {
      carState->VIN[i] = frame->data[i - 1];
    }
  } else if (frame->can_id == 0x4E1) {
    // Write in characters 10 to 17 of the VIN
    for (int i = 9; i < 17; i++) {
      carState->VIN[i] = frame->data[i - 9];
    }
    carState->VIN[17] = '\0';
  }
}