/**
 * @file i2c_bus.h
 * @brief Interface voor de gedeelde I2C-bus en seriele busdiagnose.
 * @date 2026-11-08
 * @author ProtoIoT Wietse Houwers
 * @note Prototype software for evaluation use only.
 */
#pragma once
bool beginI2cBus();
bool restartI2cBus();
void printI2cBusStatus();
