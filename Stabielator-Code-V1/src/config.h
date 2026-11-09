/**
 * @file config.h
 * @brief Productieprofielen, hardwarepinnen, tijdslimieten en elektrische motorinstellingen.
 * @date 2026-11-08
 * @author ProtoIoT Wietse Houwers
 * @note Prototype software for evaluation use only.
 */
#pragma once
#include <Arduino.h>

#ifndef STABIELATOR_LRA
#define STABIELATOR_LRA 0
#endif
constexpr bool HAPTIC_IS_LRA = STABIELATOR_LRA != 0;

#define APP_VERSION_TEXT "0.5.1"
constexpr char APP_VERSION[] = APP_VERSION_TEXT;
#if STABIELATOR_LRA
constexpr char OTA_TARGET[] = "stab-c3-lra";
#else
constexpr char OTA_TARGET[] = "stab-c3-erm";
#endif
constexpr char APP_NAME[] = "Stabielator";
constexpr uint32_t SERIAL_BAUD_RATE = 115200;
constexpr uint32_t SERIAL_WAIT_TIMEOUT_MS = 1500;
constexpr size_t SERIAL_COMMAND_BUFFER_SIZE = 128;
constexpr size_t DEVICE_SERIAL_BUFFER_SIZE = 13;
constexpr char APP_PREFERENCES_NAMESPACE[] = "stabielator";

// Pinout uit het Stabielator v0.1.0 schema. USB: GPIO18/19.
constexpr uint8_t I2C_SDA_PIN = 5;
constexpr uint8_t I2C_SCL_PIN = 4;
constexpr uint32_t I2C_CLOCK_HZ = 100000;
constexpr uint8_t IMU_RESET_PIN = 3;
constexpr uint8_t IMU_INT_PIN = 10;
constexpr uint8_t IMU_ADDRESS = 0x4A;
constexpr uint32_t IMU_REPORT_INTERVAL_US = 20000;
constexpr uint32_t IMU_STALE_MS = 250;
constexpr uint8_t RGB_RED_PIN = 7;
constexpr uint8_t RGB_GREEN_PIN = 8;
constexpr uint8_t RGB_BLUE_PIN = 6;
constexpr bool RGB_COMMON_ANODE = true;
constexpr uint8_t BUTTON_PIN = 9;
constexpr uint32_t BUTTON_DEBOUNCE_MS = 35;
constexpr uint32_t WIFI_PAIR_HOLD_MS = 5000;
// 1-cels LiPo via 300k boven / 100k naar GND; GPIO1 HIGH schakelt uit.
constexpr uint8_t BATTERY_PIN = 0;
constexpr uint8_t KILL_PIN = 1;
constexpr float BATTERY_DIVIDER_RATIO = 4.0f;

constexpr uint8_t HAPTIC_MUX_ADDRESS = 0x70; // TCA9546A, vier kanalen
constexpr uint8_t HAPTIC_DRIVER_ADDRESS = 0x5A;
constexpr uint8_t HAPTIC_MOTOR_COUNT = 4;
constexpr uint8_t HAPTIC_CHANNELS[HAPTIC_MOTOR_COUNT] = {0, 1, 2, 3};
constexpr uint32_t HAPTIC_MAX_PULSE_MS = 1000;
constexpr bool HAPTIC_PROFILE_CONFIGURED = true;
constexpr const char* HAPTIC_MOTOR_MODEL = HAPTIC_IS_LRA ? "VLV041235L-L20" : "VZ6DL2B0055211";
// ERM: VZ6DL2B0055211, 3.0 V nominaal, 2.2-3.6 V DC (datasheet p.3-4).
// 0x16 = 141 * 21.18 mV = 2.986 V (genegeerd in ERM open-loop).
// 0x17 = 138 * 21.59 mV = 2.979 V bij volle open-loop RTP-sterkte.
// LRA: VLV041235L-L20, 1.8 Vrms, max 1.85 Vrms sinus, 240 +/-10 Hz (p.4-5).
// TI DRV2605L 8.5.2.1 eq.5: SAMPLE_TIME=300 us, f=240 Hz:
// 0x16 = 69 -> 69 * .02058 / sqrt(1 - .0015*240) = 1.775 Vrms.
// Bij 250 Hz wordt dat 1.796 Vrms. Eq.9: 0x17 = 123 -> 2.610 V piek,
// of 1.846 Vrms sinus-equivalent, onder de gespecificeerde max 1.85 Vrms.
constexpr uint8_t HAPTIC_RATED_VOLTAGE_REGISTER = HAPTIC_IS_LRA ? 0x45 : 0x8D;
constexpr uint8_t HAPTIC_OVERDRIVE_CLAMP_REGISTER = HAPTIC_IS_LRA ? 0x7B : 0x8A;
// LRA: brake factor 2, loop gain 2, initial BEMF gain 2 (auto-cal past dit aan).
constexpr uint8_t HAPTIC_FEEDBACK_REGISTER = HAPTIC_IS_LRA ? 0xAA : 0x36;
// LRA DRIVE_TIME = 16 * 0.1 + 0.5 = 2.1 ms, startschatting ~238 Hz.
constexpr uint8_t HAPTIC_CONTROL1_REGISTER = HAPTIC_IS_LRA ? 0x90 : 0x93;
constexpr uint8_t HAPTIC_CONTROL2_REGISTER = 0xF5; // Signed/bidirectional, sample 300 us.
constexpr uint8_t HAPTIC_CONTROL3_REGISTER = HAPTIC_IS_LRA ? 0x80 : 0xA0;
constexpr uint8_t HAPTIC_CONTROL4_REGISTER = 0x30; // LRA auto-cal: 1000-1200 ms, OTP uit.
constexpr uint32_t HAPTIC_LRA_CALIBRATION_TIMEOUT_MS = 1500;

constexpr char WIFI_AP_NAME_PREFIX[] = "STABIELATOR";
constexpr char WIFI_PORTAL_TITLE[] = "Stabielator v" APP_VERSION_TEXT;
constexpr size_t WIFI_AP_NAME_BUFFER_SIZE = 26;
constexpr size_t WIFI_SSID_MAX_LENGTH = 32;
constexpr size_t WIFI_PASSWORD_MAX_LENGTH = 64;
constexpr char WIFI_AP_PASSWORD[] = "";
constexpr uint32_t WIFI_RECONNECT_INTERVAL_MS = 10000;
constexpr uint16_t WIFI_CONNECT_TIMEOUT_SECONDS = 15;

constexpr uint8_t WIFI_AP_CHANNEL = 1;
constexpr uint32_t WIFI_PORTAL_FALLBACK_MS = 30000;
