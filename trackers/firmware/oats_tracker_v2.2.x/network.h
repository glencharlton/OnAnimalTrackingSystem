/*
Author: Glen Charlton
Last Modified: 2024/03/27
Purpose: This file is used to declare the functions used to
setup and manage the network connections and data transfer
on the OATS v2 board.

 Software License Agreement (BSD License)
 
 Copyright (c) 2024, Glen Charlton
 All rights reserved.
 
 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:
 1. Redistributions of source code must retain the above copyright
 notice, this list of conditions and the following disclaimer.
 2. Redistributions in binary form must reproduce the above copyright
 notice, this list of conditions and the following disclaimer in the
 documentation and/or other materials provided with the distribution.
 3. Neither the name of the copyright holders nor the
 names of its contributors may be used to endorse or promote products
 derived from this software without specific prior written permission.
 
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ''AS IS'' AND ANY
 EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY
 DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/
#ifndef _NETWORK_h
#define _NETWORK_h
	
	#include <WiFi.h>
	#include <WiFiClient.h>
	#include "PubSubClient.h"
	#include "esp_wpa2.h"
	#include <esp_pm.h>
	#include <esp_wifi.h>
	#include <esp_wifi_types.h>
	#include "time.h"
	#include "sleep.h"
	#include "Arduino.h"
	#include "esp32_time.h"
	#include "config.h"
	#include "ArduinoJson.h"

	#define LOG(fmt, ...) (Serial.printf("%d: " fmt "\n", network_time_get(), ##__VA_ARGS__))

	void network_config();
	bool network_wifi_setup(int secs);
	void network_wifi_shutdown();
	void network_mqtt_shutdown();
	bool network_mqtt_setup(int secs);
	bool network_mqtt_publish(char* topic, char* msg);
	bool network_mqtt_connected();
	void network_mqtt_callback(char* topic, byte* payload, unsigned int length);
	int32_t network_time_get();
	int64_t network_millis_get();
	void network_config_set();
	void network_time_set();

#endif