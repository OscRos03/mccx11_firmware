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

#ifndef BOARD_PINOUT_H_
#define BOARD_PINOUT_H_

    //  LoRa Radio
    #define HAS_SX1262
    #define RADIO_SCLK_PIN      12
    #define RADIO_MISO_PIN      13
    #define RADIO_MOSI_PIN      11
    #define RADIO_CS_PIN        10
    #define RADIO_RST_PIN       6 // Needed
    #define RADIO_DIO1_PIN      7 // Needed
    #define RADIO_BUSY_PIN      8 // Needed

    //  Display
    // #define NO_DISPLAY

    //  GPS
    // #define GPS_I2C          // Use I2C instead of UART
    // #define GPS_SDA             18
    // #define GPS_SCL             17
    #define GPS_RX              43
    #define GPS_TX              44


    //  OTHER
    #define BUTTON_PIN          0
    // #define BATTERY_PIN         35  //LoRa32 Battery PIN 100k/100k

    // #define DEEP_SLEEP_TIME_SEC_LOOP 10

#endif