/**
 * @file nervous_sense_me.ino
 * @brief Nervous System (Environmental Sensors) sketch for the Nicla Sense ME.
 * @project Smart Mining & Safety System
 * 
 * Flashes the Nicla Sense ME to act as an environmental sensor server. 
 * Initializes the onboard BHI260AP sensor hub, BME688 air sensor, and listens 
 * for host queries from the Portenta H7 over the ESLOV (I2C) interface.
 */

#include "Arduino_BHY2.h"

void setup() {
  // Initialize the BHY2 library.
  // This loads the default BHI260AP sensor hub configuration, boots up the 
  // embedded Bosch co-processor, and sets up the I2C peripheral interface.
  BHY2.begin();
}

void loop() {
  // Continuously process sensor readings and service data requests from the host.
  // The value of '100' indicates a poll/update rate of 100ms.
  BHY2.update(100);
}
