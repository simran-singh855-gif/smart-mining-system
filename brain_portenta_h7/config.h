/**
 * @file config.h
 * @brief System configuration, pinouts, and safety thresholds for the Smart Mining & Safety System.
 * @project Smart Mining & Safety System
 */

#ifndef CONFIG_H
#define CONFIG_H

// =============================================================================
// I2C Device Addresses
// =============================================================================
#define NICLA_VISION_I2C_ADDR   0x35    // I2C address of Nicla Vision (Eyes)
// Note: Nicla Sense ME is read via the BHY2Host protocol on ESLOV, which
// operates at its own dedicated bus address.

// =============================================================================
// Safety Thresholds
// =============================================================================
#define MAX_WORKERS             8       // Maximum headcount limit in the cage
#define TEMP_LIMIT              40.0f   // Dangerous temperature limit in Celsius
#define CO2_LIMIT               1000    // Dangerous CO2 equivalent level in ppm
#define TVOC_LIMIT              500     // Dangerous TVOC level in ppb
#define IAQ_LIMIT               200     // Poor Indoor Air Quality index limit (0-500 scale)

// =============================================================================
// Hardware Pin Assignments (Arduino Portenta H7 & Carrier)
// =============================================================================
// Digital Outputs for Actuators
#define BUZZER_PIN              D2      // Piezo buzzer or siren for acoustic alarms
#define FAN_RELAY_PIN           D3      // Relay to trigger backup ventilation fan

// Onboard LED assignments for visual indicator signals
// Note: Portenta H7 onboard LEDs are active-low (LOW is ON, HIGH is OFF)
#define LED_OVERLOAD            LEDR    // Red LED indicating worker overload
#define LED_TOXIC               LEDG    // Green LED indicating toxic air / TVOC spikes
#define LED_SYSTEM_STATUS       LEDB    // Blue LED pulsing to show system is running

// =============================================================================
// System Timers & Sample Rates (Milliseconds)
// =============================================================================
#define POLL_INTERVAL_MS        250     // Rate at which to read sensors (4 Hz)
#define SERIAL_BAUD_RATE        115200  // USB Serial baud rate for diagnostics

#endif // CONFIG_H
