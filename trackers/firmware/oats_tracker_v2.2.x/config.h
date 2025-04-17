/*
Author: Glen Charlton
Last Modified: 2024/03/27
Purpose: The declaration of the configuration variables used in the OATS v2 board.

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

#ifndef _CONFIG_h
#define _CONFIG_h

#include "datatypes.h"

// Tag Variables
extern const char* client_id;
extern const char* fw_version;
extern const char* power_source;
extern bool send_processed_accel;

// Configuration Variables
extern RTC_DATA_ATTR int sensor_update_interval;
extern RTC_DATA_ATTR int mqtt_upload_interval;
extern RTC_DATA_ATTR int sensor_sampling_length;
extern RTC_DATA_ATTR int gnss_settle;
extern RTC_DATA_ATTR int gnss_skip_rate;

// Battery Settings
extern float battery_off;
extern float battery_on;

// Network Variables
extern char* ssid;
extern const char* password;
extern const int wifi_channel;
extern const char* mqttServer;
extern const int mqttPort;
extern const char* mqttUser;
extern const char* mqttPassword;
extern const char* ntpServer;

// MQTT Topics 
extern char* mqtt_init_topic;
extern char* mqtt_systemlog_topic;
extern char* mqtt_config_topic;
extern char* mqtt_accel_topic;
extern char* mqtt_gnss_topic;
extern char* mqtt_behav_topic;
extern char* mqtt_features_topic;
extern char* mqtt_location_topic;
extern char* mqtt_time_topic;

// SD Files
extern char sd_filename_systemlog[];
extern char sd_filename_accel[];
extern char sd_filename_gnss[];
extern char sd_filename_features[];
extern char sd_filename_location[];

// Variables for Json communication
extern const int MSG_SIZE;
extern char msg[];
extern const char* init_json_template;
extern const char* systemlog_json_template;
extern const char* accel_json_template;
extern const char* gnss_json_template;
extern const char* features_json_template;
extern const char* location_json_template;

#endif