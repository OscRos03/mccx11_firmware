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
#include <logger.h>
#include <WiFi.h>
#include "configuration.h"
#include "web_utils.h"
#include "display.h"

extern      Configuration       Config;
extern      logging::Logger     logger;
extern      TrackerMethod       trackerMethod;
String      home                = "TP-Link_AC1C_2.4GHz";

uint32_t    noClientsTime        = 0;

struct savedNetwork {
    String SSID;
    String BSSID;
    int32_t RSSI;
};


namespace WIFI_Utils {

    void scanAndLog(){
        String logged[] = {};
        int channels[] = {1,6,7,11,13};  // test with different channels and order
        int sizeOfArray = sizeof(channels)/sizeof(channels[0]);
        byte networksInChannel;
        std::vector<savedNetwork> allSavedNetworks; 

        
        for (int i = 0; i<sizeOfArray; i++) {
            networksInChannel = WiFi.scanNetworks(false,false,false,200,channels[i]);

            for (int j = 0; j < networksInChannel; j++) {
                if (WiFi.BSSIDstr(j)) {}

                savedNetwork network;
                network.SSID = WiFi.SSID(j);
                network.BSSID = WiFi.BSSIDstr(j);
                network.RSSI = WiFi.RSSI(j);
                allSavedNetworks.push_back(network);
            }
            WiFi.scanDelete();

        }
        for (int i = 0; i < allSavedNetworks.size(); i++) {
            Serial.printf("%s,\n",allSavedNetworks[i].BSSID.c_str());
        }
    }

    void networkScanner(){
        uint32_t before = millis();
        std::vector<savedNetwork> allSavedNetworks; 
        int channels[] = {1,6,7,11,13};  // test with different channels and order
        int sizeOfArray = sizeof(channels)/sizeof(channels[0]);
        bool found = false;
        byte networksInChannel;

        for (int i = 0; i<sizeOfArray; i++) {
            networksInChannel = WiFi.scanNetworks(false,false,false,200,channels[i]);
            Serial.printf("%d networks in channel %d\n",networksInChannel, channels[i]);
            
            for (int j = 0; j < networksInChannel; j++) {
                // Serial.printf("BSSID: %s\n",WiFi.BSSIDstr(j).c_str());
                // Serial.printf("SSID: %s\n",WiFi.SSID(j).c_str());
                
                savedNetwork network;
                network.SSID = WiFi.SSID(j);
                network.BSSID = WiFi.BSSIDstr(j);
                network.RSSI = WiFi.RSSI(j);
                allSavedNetworks.push_back(network);
                
                if (WiFi.SSID(j) == home) { // naive approach for a "home network"
                    found = true;
                    break;
                }
                
            }
            if (found) break;
            WiFi.scanDelete();
            Serial.print("\n\n");
        }
        
        
        // if (numOfNW == 0) {
        //     Serial.println("no nearby networks");
        //     Config.trackerMethod = TrackerMethod::gps;
        // } else {
        //     for (int i=0; i<numOfNW; i++) {
                // Serial.printf("Network #%d SSID: %s RSSI: %d \n\n", i+1, WiFi.SSID(i).c_str(), WiFi.RSSI(i));
        //         if (WiFi.SSID(i) == home) {    // naive approach
        //             Serial.println("Tracker is home.\n");
        //         }
        //     }
        // }
        uint32_t after = millis();
        switch (found) {
            case true: {
                logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO,"WiFi","On home network");
                break;
            }
            case false: {
                logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO,"WiFi","Home network not found, discovered networks:");
                if (allSavedNetworks.size() == 0) {
                    Serial.println("no nearby networks, switching to gps");
                    Config.trackerMethod = TrackerMethod::gps;
                }
                for (int i = 0; i < allSavedNetworks.size(); i++) {
                    Serial.printf("Network #%d SSID: %s BSSID: %s\n",i,allSavedNetworks[i].SSID.c_str(),allSavedNetworks[i].BSSID.c_str());
                }
                break;
            }
        }
        logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO,"WiFi","time to find networks: %d in ms",after-before);
        //delay(5000);
    }

    void startAutoAP() {
        WiFi.mode(WIFI_MODE_NULL);
        WiFi.mode(WIFI_AP);
        WiFi.softAP("LoRaTracker-AP", Config.wifiAP.password);
    }

    void checkIfWiFiAP() {
        if (Config.wifiAP.active || Config.beacons[0].callsign == "TEST-7"){
            displayShow(" LoRa APRS", "    ** WEB-CONF **","", "WiFiAP:LoRaTracker-AP", "IP    :   192.168.4.1","");
            logger.log(logging::LoggerLevel::LOGGER_LEVEL_WARN, "Main", "WebConfiguration Started!");
            startAutoAP();
            WEB_Utils::setup();
            while (true) {
                if (WiFi.softAPgetStationNum() > 0) {
                    noClientsTime = 0;
                } else {
                    if (noClientsTime == 0) {
                        noClientsTime = millis();
                    } else if ((millis() - noClientsTime) > 2 * 60 * 2000) {
                        // logger.log(logging::LoggerLevel::LOGGER_INFO, "Main","Eggg");
                        logger.log(logging::LoggerLevel::LOGGER_LEVEL_WARN, "Main", "WebConfiguration Stopped!");
                        displayShow("", "", "  STOPPING WiFi AP", 2000);
                        Config.wifiAP.active = false;
                        Config.writeFile();
                        WiFi.softAPdisconnect(true);
                        ESP.restart();
                    }
                }
            }
        } else {
            WiFi.mode(WIFI_OFF);
            logger.log(logging::LoggerLevel::LOGGER_LEVEL_DEBUG, "Main", "WiFi controller stopped");
        }
    }
}