#ifndef carStateStruct
#define carStateStruct
#include <Arduino.h>
typedef struct
{
   int chargingCurrent;
   int fuelRange; //Holds the estimated fuel range remaining
   char transmissionRange [10]; // PRNDL Shifter Location
   char VIN [18];
   int coolantTemp;
   int chargingVoltage;
   int batteryChargePercent;
   int vehicleSpeed;
   int RPM;
   long odometer;
   int OAT; // Outside Air Temperature
   boolean brakePressed; // false: not pressed. true: pressed
   boolean ignitionStatus; // false: ingition Off; true: ignition On
   boolean engineRunning; // false: engine Off; true: Engine On
   boolean doorLocked; // false: unlocked; true: locked.

} CarState;
#endif //carStateStruct
