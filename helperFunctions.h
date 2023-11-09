#ifndef helperFunctions
#define helperFunctions

#include "CarStateStruct.h"
#include <ArduinoJson.h>

// Function prototypes
void printData(CarState* carState);
String serializeCarStateToJson(CarState* carState);

#endif // helperFunctions
