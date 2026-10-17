// Only modify this file to include
// - function definitions (prototypes)
// - include files
// - extern variable definitions
// In the appropriate section

#ifndef _ESP01EnergyMeterV2_H_
#define _ESP01EnergyMeterV2_H_
#include "Arduino.h"
//add your includes for the project ESP01EnergyMeterV2 here
#include "WiFiHelper.h"
#include "ServerHelper.h"
#include "Logger.h"

#include <WiFiConfig.h>

#include <PZEM004Tv30.h>

//end of add your includes here


//add your function definitions for the project ESP01EnergyMeterV2 here
void indexPage();
void handleNotFound();
void setupHTTPActions();
void setupAfterWiFiConnected();

void health();
void sendSensorData();
void writeJson(int httpStatus, const JsonDocument &doc);



//Do not add code below this line
#endif /* _ESP01EnergyMeterV2_H_ */
