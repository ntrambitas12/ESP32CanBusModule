#include "helperFunctions.h"


void printData(CarState* carState) {
    Serial.println("Fuel Range: ");
    Serial.print(carState->fuelRange);
    Serial.println("");

    Serial.println("Gear Status: ");
    Serial.print(carState->transmissionRange);
    Serial.println("");

    Serial.println("Brake Pressed: ");
    Serial.print(carState->brakePressed);
    Serial.println("");

    Serial.println("Ignition Status: ");
    Serial.print(carState->ignitionStatus);
    Serial.println("");

    Serial.println("Engine Running: ");
    Serial.print(carState->engineRunning);
    Serial.println("");

    Serial.println("OAT: ");
    Serial.print(carState->OAT);
    Serial.println("");

    Serial.println("Coolant Temp: ");
    Serial.print(carState->coolantTemp);
    Serial.println("");

    Serial.println("Engine RPM: ");
    Serial.print(carState->RPM);
    Serial.println("");

    Serial.println("Vehicle Speed: ");
    Serial.print(carState->vehicleSpeed);
    Serial.println("");

    Serial.println("Battery Charge Level: ");
    Serial.print(carState->batteryChargePercent);
    Serial.println("");


    Serial.println("Battery Charging Voltage: ");
    Serial.print(carState->chargingVoltage);
    Serial.println("");

    Serial.println("Battery Charging Current: ");
    Serial.print(carState->chargingCurrent);
    Serial.println("");

    Serial.println("VIN Number: ");
    Serial.print(carState->VIN);
    Serial.println("");

    Serial.println("Odometer: ");
    Serial.println(carState->odometer);
    Serial.println("");

    Serial.println("Door Locked: ");
    Serial.println(carState->doorLocked);
    Serial.println("");
}

String serializeCarStateToJson(CarState* carState) {
  // Define a JSON object
  StaticJsonDocument<256> jsonDoc; // Adjust the size as needed

  // Populate the JSON object with struct values using the -> operator
  jsonDoc["chargingCurrent"] = carState->chargingCurrent;
  jsonDoc["fuelRange"] = carState->fuelRange;
  jsonDoc["transmissionRange"] = carState->transmissionRange;
  jsonDoc["VIN"] = carState->VIN;
  jsonDoc["coolantTemp"] = carState->coolantTemp;
  jsonDoc["chargingVoltage"] = carState->chargingVoltage;
  jsonDoc["batteryChargePercent"] = carState->batteryChargePercent;
  jsonDoc["vehicleSpeed"] = carState->vehicleSpeed;
  jsonDoc["RPM"] = carState->RPM;
  jsonDoc["odometer"] = carState->odometer;
  jsonDoc["OAT"] = carState->OAT;
  jsonDoc["brakePressed"] = carState->brakePressed;
  jsonDoc["ignitionStatus"] = carState->ignitionStatus;
  jsonDoc["engineRunning"] = carState->engineRunning;
  jsonDoc["doorLocked"] = carState->doorLocked;

  // Serialize the JSON document to a string
  String jsonString;
  serializeJson(jsonDoc, jsonString);

  return jsonString;
}
