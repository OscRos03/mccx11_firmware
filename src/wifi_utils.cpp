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

#define CONNECTION_TIMEOUT 10

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
        
        
        uint32_t after = millis();
        if (found){
            logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO,"WiFi","On home network");
        } else {
            logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO,"WiFi","Home network not found, discovered networks:");
            for (int i = 0; i < allSavedNetworks.size(); i++) {
                Serial.printf("Network #%d SSID: %s BSSID: %s\n",i,allSavedNetworks[i].SSID.c_str(),allSavedNetworks[i].BSSID.c_str());
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
        if (Config.wifiAP.active || Config.beacons[0].callsign == "NOCALL-7"){
            displayShow(" LoRa APRS", "    ** WEB-CONF **","", "WiFiAP:LoRaTracker-AP", "IP    :   192.168.4.1","");
            logger.log(logging::LoggerLevel::LOGGER_LEVEL_WARN, "Main", "WebConfiguration Started!");
            startAutoAP();
            logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "Main", "After startAutoAP");
            WEB_Utils::setup();
            logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "Main", "After WEB_utils::setup()");
            while (true) {
                if (WiFi.softAPgetStationNum() > 0) {
                    noClientsTime = 0;
                    logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "Main", "WiFi.softAPgetStationNum() > 0");
                } else {
                    if (noClientsTime == 0) {
                        noClientsTime = millis();
                        logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "Main", "noClientsTime: %lu", noClientsTime);
                    } else if ((millis() - noClientsTime) > 2 * 60 * 2000) {
                        logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "Main", "Time is up, stopping WiFi AP");
                        logger.log(logging::LoggerLevel::LOGGER_LEVEL_WARN, "Main", "WebConfiguration Stopped!");
                        displayShow("", "", "  STOPPING WiFi AP", 2000);
                        Config.wifiAP.active = false;
                        Config.writeFile();
                        WiFi.softAPdisconnect(true);
                        ESP.restart();
                    }
                    logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "Main", "Long loop, (millis() - noClientsTime): %lu", millis() - noClientsTime);
                    delay(100);
                }
            }
        } else {
            WiFi.mode(WIFI_OFF);
            logger.log(logging::LoggerLevel::LOGGER_LEVEL_DEBUG, "Main", "WiFi controller stopped");
        }
    }

    bool connectHomeNetwork(const char* ssid, const char* password) {
        if (!WiFi.mode(WIFI_STA)){
            logger.log(logging::LoggerLevel::LOGGER_LEVEL_WARN, "WiFi", "Could not enter WiFi STA mode");
            return false;
        }
        WiFi.begin(ssid, password);
        logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "WiFi", "Connecting to WiFi...");


        int i = 0;
        while (WiFi.status() != WL_CONNECTED) {
            delay(1000);
            logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "WiFi", "Seconds elapsed: %d", ++i);
            if (i >= CONNECTION_TIMEOUT){
                return false;
            }
        }
        logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "WiFi", "Connected, IP Address: %s", WiFi.localIP());
        return true;
    }
}