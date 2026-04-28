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

#include <APRSPacketLib.h>
#include <logger.h>
#include <Wire.h>
#include "configuration.h"
#include "board_pinout.h"
#include "lora_utils.h"
#include "display.h"
#include "utils.h"

extern Beacon                   *currentBeacon;
extern Configuration            Config;
extern logging::Logger          logger;

extern uint32_t                 lastTx;
extern uint32_t                 lastTxTime;

extern bool                     displayEcoMode;
extern uint32_t                 displayTime;
extern bool                     displayState;
extern int                      menuDisplay;
extern String                   versionDate;
extern bool                     flashlight;

extern bool                     statusUpdate;

uint32_t    statusTime          = millis();
uint8_t     wxModuleAddress     = 0x00;
uint8_t     keyboardAddress     = 0x00;
uint8_t     touchModuleAddress  = 0x00;


namespace Utils {

    static char locator[11];    // letterize and getMaidenheadLocator functions are Copyright (c) 2021 Mateusz Salwach - MIT License

    static char letterize(int x) {
        return (char) x + 65;
    }

    char *getMaidenheadLocator(double lat, double lon, uint8_t size) {
        double LON_F[]={20,2.0,0.083333,0.008333,0.0003472083333333333};
        double LAT_F[]={10,1.0,0.0416665,0.004166,0.0001735833333333333};
        int i;
        lon += 180;
        lat += 90;

        if (size <= 0 || size > 10) size = 6;
        size/=2; size*=2;

        for (i = 0; i < size/2; i++) {
            if (i % 2 == 1) {
                locator[i*2] = (char) (lon/LON_F[i] + '0');
                locator[i*2+1] = (char) (lat/LAT_F[i] + '0');
            } else {
                locator[i*2] = letterize((int) (lon/LON_F[i]));
                locator[i*2+1] = letterize((int) (lat/LAT_F[i]));
            }
            lon = fmod(lon, LON_F[i]);
            lat = fmod(lat, LAT_F[i]);
        }
        locator[i*2]=0;
        return locator;
    }

    static String padding(unsigned int number, unsigned int width) {
        String result;
        String num(number);
        if (num.length() > width) width = num.length();
        for (unsigned int i = 0; i < width - num.length(); i++) {
            result.concat('0');
        }
        result.concat(num);
        return result;
    }

    String createDateString(time_t t) {
        String dateString = padding(year(t), 4);
        dateString += "-";
        dateString += padding(month(t), 2);
        dateString += "-";
        dateString += padding(day(t), 2);
        return dateString;
    }

    String createTimeString(time_t t) {
        String timeString = padding(hour(t), 2);
        timeString += ":";
        timeString += padding(minute(t), 2);
        timeString += ":";
        timeString += padding(second(t), 2);
        return timeString;
    }

    void checkStatus() {
        if (statusUpdate) {
            if (currentBeacon->status == "") {
                statusUpdate = false;
            } else {
                uint32_t currentTime = millis();
                uint32_t statusTx = currentTime - statusTime;
                lastTx = currentTime - lastTxTime;
                logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "GPS","checking status -- found new location");
                if (statusTx > 10 * 60 * 1000 && lastTx > 10 * 1000) {
                    LoRa_Utils::sendNewPacket(APRSPacketLib::generateStatusPacket(currentBeacon->callsign, "APLRT1", Config.path, currentBeacon->status));
                    statusUpdate = false;
                }
            }
        }
    }

    void checkDisplayEcoMode() {
        if (displayState && displayEcoMode && menuDisplay == 0) {
            uint32_t currentTime = millis();
            uint32_t lastDisplayTime = currentTime - displayTime;
            if (currentTime > 10 * 1000 && lastDisplayTime >= Config.display.timeout * 1000) {
                displayToggle(false);
                displayState = false;
            }
        }
    }

    String getSmartBeaconState() {
        if (currentBeacon->smartBeaconActive) return "On";
        return "Off";
    }

    void checkFlashlight() {
        bool desiredState       = flashlight ? HIGH : LOW;
        uint8_t flashlightPin   = Config.notification.ledFlashlightPin;
        if (desiredState != digitalRead(flashlightPin)) digitalWrite(flashlightPin, desiredState);
    }

    void i2cScannerForPeripherals() {
        uint8_t err, addr;
        if (Config.telemetry.active) {
            for (addr = 1; addr < 0x7F; addr++) {
                #if defined(HELTEC_V3_GPS) || defined(HELTEC_V3_2_GPS)
                    Wire1.beginTransmission(addr);
                    err = Wire1.endTransmission();
                #else
                    Wire.beginTransmission(addr);
                    err = Wire.endTransmission();
                #endif
                if (err == 0) {
                    //Serial.println(addr); this shows any connected board to I2C
                    if (addr == 0x76 || addr == 0x77) {
                        wxModuleAddress = addr;
                        logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "Main", "Wx Module Connected to I2C");
                    }
                }
            }
        }

        #if defined(TTGO_T_DECK_GPS) || defined(TTGO_T_DECK_PLUS)
            delay(500);
            const uint8_t keyboardAddr = 0x55;
            for (int i = 0; i < 10; ++i) {
                Wire.beginTransmission(keyboardAddr);
                int err = Wire.endTransmission();
                if (err == 0) {
                    keyboardAddress = keyboardAddr;
                    logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "Main", "T-Deck Keyboard Connected to I2C");
                    break;
                }
                delay(50);
            }
        #else
            for (addr = 1; addr < 0x7F; addr++) {
                Wire.beginTransmission(addr);
                err = Wire.endTransmission();
                if (err == 0 && addr == 0x5F) { // CARDKB from m5stack.com (YEL - SDA / WTH SCL)
                    //Serial.println(addr); this shows any connected board to I2C
                    keyboardAddress = addr;
                    logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "Main", "CARDKB Keyboard Connected to I2C");
                }
            }
        #endif

        #ifdef HAS_TOUCHSCREEN
            for (addr = 1; addr < 0x7F; addr++) {
                Wire.beginTransmission(addr);
                err = Wire.endTransmission();
                if (err == 0) {
                    if (addr == 0x14 || addr == 0x5D ) {
                        touchModuleAddress = addr;
                        logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "Main", "Touch Module Connected to I2C");
                    }
                }
            }
        #endif
    }

    uint8_t i2cWriteRegister(uint8_t device, uint8_t reg, const uint16_t data, uint8_t attempts) {
        uint8_t status = 0;
        for (uint8_t _ = 0; _ < attempts; _++) {
            Wire.beginTransmission(device);
            Wire.write(reg);
            Wire.write(data & 0xFF);
            Wire.write(data >> 8 & 0xFF);
            status = Wire.endTransmission();
            if (status == 0) break;
        }
        return status;
    }

    uint8_t i2cReadRegister(uint8_t device, uint8_t reg, uint16_t &output, uint8_t attempts) {
        uint8_t status;
        uint8_t nrReceivedBytes;
        if (attempts == 0) return 0;
        for (uint8_t _ = 0; _ < attempts; _++) {
            Wire.beginTransmission(device);
            Wire.write(reg);
            status = Wire.endTransmission(false); // dont relase the bus
            if (status > 0) continue;

            nrReceivedBytes = Wire.requestFrom(device, (uint8_t) 2);
            if (nrReceivedBytes != 2) continue;

            uint8_t lsB = Wire.read();
            uint8_t msB = Wire.read();
            output = (msB << 8) | lsB;

            return 0;
        }
        if (status > 0) return status;
        return 6; // too few bytes error code
    }

    uint8_t i2cWriteRegisterAndVerify(uint8_t device, uint8_t reg, const uint16_t data, uint8_t attempts) {
        uint8_t write_status = 0;
        uint8_t read_status = 0;
        if (attempts == 0) return 0;
        for (uint8_t _ = 0; _ < attempts; _++) {
            write_status = i2cWriteRegister(device, reg, data);
            if (write_status > 0) continue;
            delay(1);

            uint16_t response;
            read_status = i2cReadRegister(device, reg, response);
            if (read_status > 0) continue;

            if (response == data) return 0;
        }
        if (write_status > 0) return write_status;
        if (read_status > 0) return read_status;
        return 7; // error code for respons != data
    }
}