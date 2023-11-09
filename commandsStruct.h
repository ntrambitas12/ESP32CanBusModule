#include <Arduino.h>
#include "mcp2515.h"

typedef struct
{
    char commandName[20];
    struct can_frame canFrame;
    boolean lsBus; //if true, LSBUS. Else, HSBUS
} Command;