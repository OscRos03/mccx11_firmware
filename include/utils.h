/* Copyright (C) 2025 Ricardo Guzman - CA2RXU
 * 
 * This file is part of LoRa APRS Tracker.
 * 
 * LoRa APRS Tracker is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or 
 * (at your option) any later version.
 * 
 * LoRa APRS Tracker is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with LoRa APRS Tracker. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef UTILS_H_
#define UTILS_H_

#include <Arduino.h>
#include <TimeLib.h>

namespace Utils {

    char    *getMaidenheadLocator(double lat, double lon, uint8_t size);
    String  createDateString(time_t t);
    String  createTimeString(time_t t);
    void    checkStatus();
    void    checkDisplayEcoMode();
    String  getSmartBeaconState();
    void    checkFlashlight();
    void    i2cScannerForPeripherals();

    /**
     * @param device i2c adress
     * @param reg register adress
     * @param data bytes to transmit (in the order of the array)
     * @param length length of data array
     * @return operation status (return value from Wire.endTransmission)
     */
    uint8_t i2cWriteRegister(uint8_t device, uint8_t reg, const uint8_t data[], uint8_t length);

    /**
     * @param device i2c adress
     * @param reg register adress
     * @param length number of bytes to request
     * @param output array to which received bytes will be written (in order they are received). Should of length specfied by parameter 'length'
     * @param nrReceivedBytes output for number of received bytes
     * @return operation status (return value from Wire.endTransmission)
     */
    uint8_t i2cReadRegister(uint8_t device, uint8_t reg, uint8_t length, uint8_t output[], uint8_t *nrReceivedBytes);

    /**
     * Write to an i2c register and verify that the data was written correctly
     * 
     * @param device i2c adress
     * @param reg register adress
     * @param data bytes to transmit (in the order of the array)
     * @param responseBuffer array of at least the length specified by the length parameter
     * @param length length of data array
     * @param attempts number of times to try to write the data
     * @return whether the data was written successfully 
     */
    bool    i2cWriteRegisterAndVerify(uint8_t device, uint8_t reg, const uint8_t data[], uint8_t responseBuffer[], uint8_t length, uint8_t attempts);

}

#endif