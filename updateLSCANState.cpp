#include <SPI.h>
#include <mcp2515.h>
#include "updateCANState.h"


void proccessLSCAN(CarState* carState, struct can_frame* frame) {
  calcDoorLockStatus(carState, frame);
}

void calcDoorLockStatus(CarState* carState, struct can_frame* frame) {
  if (frame->can_id == 0x9020C040) {
    carState->doorLocked = true; // For debugging, set to true. Testing to make sure bus communication works This is light status btw
  }
}