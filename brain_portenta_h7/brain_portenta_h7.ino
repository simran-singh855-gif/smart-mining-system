/**
 * @file brain_portenta_h7.ino
 * @brief Brain (Central Controller) sketch for the Smart Mining & Safety System.
 * @project Smart Mining & Safety System
 * 
 * Aggregates data from Nicla Sense ME (via ESLOV / BHY2Host) and Nicla Vision (via Wire I2C),
 * evaluates dangerous gas, temperature, and occupancy thresholds, and controls safety actuators.
 */

#include <Wire.h>
#include <Arduino_BHY2Host.h>
#include "config.h"

// Instantiate BSEC (Bosch Software Environmental Cluster) sensor from Nicla Sense ME
// We try both the standard ID and have a backup definition.
SensorBSEC bsec(SENSOR_ID_BSEC);

// System State Variables
uint8_t workerCount = 0;
float temperature = 0.0f;
float humidity = 0.0f;
uint32_t co2Equivalent = 0;
uint16_t iaqIndex = 0;
uint8_t bsecAccuracy = 0;

bool niclaSenseAvailable = false;
bool niclaVisionAvailable = false;

// Alert Status Flags
bool isOverloaded = false;
bool isAirToxic = false;
bool isTempDangerous = false;

// Timing variables
unsigned long lastBlinkTime = 0;
bool statusLedState = false;

void setup() {
  // Initialize Serial interface for host debugging & diagnostics
  Serial.begin(SERIAL_BAUD_RATE);
  delay(1500); // Wait for Serial monitor to connect
  
  Serial.println("==================================================");
  Serial.println("   SMART MINING & SAFETY SYSTEM - BRAIN CORE   ");
  Serial.println("==================================================");

  // Configure GPIO Pins for alarms and ventilation relays
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(FAN_RELAY_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);      // Ensure buzzer is off
  digitalWrite(FAN_RELAY_PIN, LOW);   // Ensure ventilation fan is off

  // Onboard RGB LEDs are Active-Low on Portenta H7 (HIGH is OFF, LOW is ON)
  pinMode(LED_OVERLOAD, OUTPUT);
  pinMode(LED_TOXIC, OUTPUT);
  pinMode(LED_SYSTEM_STATUS, OUTPUT);
  digitalWrite(LED_OVERLOAD, HIGH);
  digitalWrite(LED_TOXIC, HIGH);
  digitalWrite(LED_SYSTEM_STATUS, HIGH);

  // Initialize standard I2C Master Interface
  Wire.begin();
  Wire.setClock(100000); // Set standard I2C clock rate (100 kHz)
  Serial.println("[I2C] Master Bus Initialized.");

  // Initialize the Nicla Sense ME via ESLOV (BHY2 Host Interface)
  Serial.println("[Sense ME] Connecting to sensor array over ESLOV...");
  if (BHY2Host.begin(false, NICLA_VIA_ESLOV)) {
    Serial.println("[Sense ME] Connection successful!");
    bsec.begin();
    niclaSenseAvailable = true;
  } else {
    Serial.println("[WARNING] Nicla Sense ME not detected on ESLOV bus. Running in degraded mode.");
  }
}

void loop() {
  // 1. Update the BHY2 Host parser if the sensor array is online
  if (niclaSenseAvailable) {
    BHY2Host.update();
    
    // Extract compensated values from the BSEC algorithm
    temperature = bsec.comp_t();
    humidity = bsec.comp_h();
    co2Equivalent = bsec.co2_eq();
    iaqIndex = bsec.iaq();
    bsecAccuracy = bsec.accuracy();
  }

  // 2. Poll the Nicla Vision (Eyes) for the current occupant count over standard I2C
  pollNiclaVision();

  // 3. Process thresholds and manage output safety triggers
  checkSafetyLimits();

  // 4. Print telemetry to Serial for logging
  printTelemetry();

  // 5. System Status Heartbeat Blink (Blue LED)
  unsigned long currentMillis = millis();
  if (currentMillis - lastBlinkTime >= 500) {
    lastBlinkTime = currentMillis;
    statusLedState = !statusLedState;
    // Remember, onboard LED is active-low, so turn on when state is true
    digitalWrite(LED_SYSTEM_STATUS, statusLedState ? LOW : HIGH);
  }

  // Wait until next sampling period
  delay(POLL_INTERVAL_MS);
}

/**
 * @brief Requests the worker headcount from the Nicla Vision camera over the I2C bus.
 */
void pollNiclaVision() {
  // Request 1 byte of occupancy data from address 0x35
  Wire.requestFrom(NICLA_VISION_I2C_ADDR, 1);
  
  if (Wire.available()) {
    workerCount = Wire.read();
    niclaVisionAvailable = true;
  } else {
    niclaVisionAvailable = false;
    // Log communication issue once in a while or set a fail-safe worker count
    // workerCount = 0;
  }
}

/**
 * @brief Assesses sensor measurements and occupancy against safety configurations,
 *        triggering alarms and backup ventilation when parameters are violated.
 */
void checkSafetyLimits() {
  // Reset alert flags
  isOverloaded = false;
  isAirToxic = false;
  isTempDangerous = false;

  // A. Check worker headcount overload
  if (workerCount > MAX_WORKERS) {
    isOverloaded = true;
  }

  // B. Check air toxicity (Gas/CO2/VOC)
  // Only evaluate if sensor data is active and BSEC has calibrated (accuracy > 0)
  if (niclaSenseAvailable && bsecAccuracy > 0) {
    if (co2Equivalent > CO2_LIMIT || iaqIndex > IAQ_LIMIT) {
      isAirToxic = true;
    }
  }

  // C. Check high temperature / fire risk
  if (niclaSenseAvailable && temperature > TEMP_LIMIT) {
    isTempDangerous = true;
  }

  // --- ACTUATOR CONTROL LOGIC ---
  
  // Rule 1: Emergency Ventilation (relays)
  // Fan turns on if air turns toxic, temperature spikes, or cage is overloaded (extra ventilation)
  if (isAirToxic || isTempDangerous || isOverloaded) {
    digitalWrite(FAN_RELAY_PIN, HIGH); // Turn backup ventilation fan ON
  } else {
    digitalWrite(FAN_RELAY_PIN, LOW);  // Keep/Turn fan OFF
  }

  // Rule 2: Acoustic Alarm (siren/buzzer)
  if (isOverloaded) {
    // Overload: Constant tone
    digitalWrite(BUZZER_PIN, HIGH);
  } else if (isAirToxic || isTempDangerous) {
    // Air Quality / Fire: Pulsating alarm (e.g. 500ms intervals)
    static bool buzzerToggle = false;
    static unsigned long lastBuzzerToggle = 0;
    if (millis() - lastBuzzerToggle >= 500) {
      lastBuzzerToggle = millis();
      buzzerToggle = !buzzerToggle;
    }
    digitalWrite(BUZZER_PIN, buzzerToggle ? HIGH : LOW);
  } else {
    // Safe: Silent
    digitalWrite(BUZZER_PIN, LOW);
  }

  // Rule 3: Onboard Visual Status LEDs (Active-Low)
  // Red LED signals Overload or Dangerous Temperature
  if (isOverloaded || isTempDangerous) {
    digitalWrite(LED_OVERLOAD, LOW); // Red ON
  } else {
    digitalWrite(LED_OVERLOAD, HIGH); // Red OFF
  }

  // Green LED signals Toxic Air Quality
  if (isAirToxic) {
    digitalWrite(LED_TOXIC, LOW); // Green ON
  } else {
    digitalWrite(LED_TOXIC, HIGH); // Green OFF
  }
}

/**
 * @brief Prints current system metrics to USB serial for edge console monitoring.
 */
void printTelemetry() {
  Serial.print("TELEMETRY | ");
  
  // Vision (Eyes)
  if (niclaVisionAvailable) {
    Serial.print("Workers: ");
    Serial.print(workerCount);
    Serial.print("/");
    Serial.print(MAX_WORKERS);
  } else {
    Serial.print("Workers: [COMM ERROR]");
  }
  
  Serial.print(" | ");

  // Environmental (Nervous System)
  if (niclaSenseAvailable) {
    Serial.print("Temp: ");
    Serial.print(temperature, 1);
    Serial.print("C | Humidity: ");
    Serial.print(humidity, 1);
    Serial.print("% | CO2Eq: ");
    Serial.print(co2Equivalent);
    Serial.print("ppm | IAQ: ");
    Serial.print(iaqIndex);
    Serial.print(" (Acc: ");
    Serial.print(bsecAccuracy);
    Serial.print(")");
  } else {
    Serial.print("Sensors: [COMM ERROR]");
  }

  // Actuator/Alarm States
  Serial.print(" | Actuators: ");
  if (isOverloaded) Serial.print("[OVERLOAD ALARM] ");
  if (isAirToxic) Serial.print("[TOXIC AIR ALARM] ");
  if (isTempDangerous) Serial.print("[FIRE/TEMP ALARM] ");
  
  if (digitalRead(FAN_RELAY_PIN) == HIGH) {
    Serial.print("[FAN: ON]");
  } else {
    Serial.print("[FAN: OFF]");
  }
  
  Serial.println();
}
