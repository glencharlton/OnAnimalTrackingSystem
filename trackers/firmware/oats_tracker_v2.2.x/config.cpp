/*
Author: Glen Charlton
Last Modified: 2024/03/27
Purpose: Declaration of the configuration variables used in the OATS v2 board. 

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

#include "Arduino.h"
#include "config.h"

// Tag Variables
const char* client_id = "oats_0002";
const char* fw_version = "OATS_Tracker_v2.4.1";
const char* power_source = "solar"; // solar or primary
bool send_processed_accel = true; // true = features, false = raw


// Default Configuration Variables - note editable via datahub although will revert to these after reset 
RTC_DATA_ATTR int mqtt_upload_interval = 10800; // Interval between attempting publishing of data to datahub (once time exceeded, attempt is made every sensor sample)
RTC_DATA_ATTR int sensor_update_interval = 600; // Interval between sensor reads 
RTC_DATA_ATTR int sensor_sampling_length = 5; //length of GNSS and feature epoch/window length (seconds)
RTC_DATA_ATTR int gnss_settle = 10;	 // GNSS settle time once locked acheived (seconds)
RTC_DATA_ATTR int gnss_skip_rate = 0; // length of accelerometer feature window (seconds)

// Battery Settings
float battery_off = 3.6; // battery voltage cutoff (V)
float battery_on = 3.7; // battery voltage for return post cutoff (V)

// Network Variables
char* ssid = "datahub";
const char* password = "password"; 
const int wifi_channel = 6; // locking the wifi channel allows for shorter wifi search time
const char* mqttServer = "192.168.4.1"; // datahub SBC network location
const int mqttPort = 1883; 
const char* ntpServer = "pool.ntp.org"; //not required - uses time either from datahub or GNSS

// MQTT Topics 
char* mqtt_init_topic = "oats/system/init/";
char* mqtt_systemlog_topic = "oats/data/systemlog/";
char* mqtt_config_topic = "oats/system/config/";
char* mqtt_accel_topic = "oats/data/accel/";
char* mqtt_gnss_topic =  "oats/data/gnss/";
char* mqtt_features_topic =  "oats/data/accelerometer_features/";
char* mqtt_location_topic =  "oats/data/location/";
char* mqtt_time_topic =  "oats/sys/time/";

// SD Files
char sd_filename_systemlog[] = "/syslog.dat";
char sd_filename_accel[] = "/accel.dat";
char sd_filename_gnss[] = "/gnss.dat";
char sd_filename_features[] = "/feat.dat";
char sd_filename_location[] = "/loc.dat";


// Variables for JSON communication
const int MSG_SIZE = 1024;
char msg[MSG_SIZE];
const char* init_json_template = "{\"id\": \"%s\", \"fw\": \"%s\"}";
const char* systemlog_json_template = "{\"id\": \"%s\",\"fw\": \"%s\",\"time\": \"%d\",\"vbat\": \"%.3f\",\"ontime\": \"%d\",\"gnss\": \"%d\",\"accel\": \"%d\",\"network\": \"%d\"}";
const char* accel_json_template = "{\"id\": \"%s\",\"time\":\"%d\", \"ms\":\"%d\",\"accel\":{\"x\":%.3f,\"y\":%.3f,\"z\":%.3f,\"abs\":%.3f}}";
const char* gnss_json_template = "{\"id\": \"%s\",\"time\":\"%d\",\"location\":{\"lat\":%.6f,\"lon\":%.6f,\"alt\":%.3f,\"spd\":%.3f},\"quality\":{\"sats\":%d,\"hdop\":%.3f,\"vdop\":%.3f,\"pdop\":%.3f,\"quality\":%d}}";
const char* features_json_template = "{\"id\": \"%s\",\"time\":\"%d\", \"epoch_length\":\"%d\",\"features\":{\"mean_x\":%.3f,\"mean_y\":%.3f,\"mean_z\":%.3f,\"mean_abs\":%.3f,\"min_x\":%.3f,\"min_y\":%.3f,\"min_z\":%.3f,\"min_abs\":%.3f,\"max_x\":%.3f,\"max_y\":%.3f,\"max_z\":%.3f,\"max_abs\":%.3f,\"mean_mv_x\":%.3f,\"mean_mv_y\":%.3f,\"mean_mv_z\":%.3f,\"mean_mv_abs\":%.3f,\"min_mv_x\":%.3f,\"min_mv_y\":%.3f,\"min_mv_z\":%.3f,\"min_mv_abs\":%.3f,\"max_mv_x\":%.3f,\"max_mv_y\":%.3f,\"max_mv_z\":%.3f,\"max_mv_abs\":%.3f}}";
const char* location_json_template = "{\"id\": \"%s\",\"time\":\"%d\",\"location\":{\"latitude\":%.6f,\"longitude\":%.6f,\"altitude\":%.3f,\"speed\":%.3f, \"duration\":%d, \"distance\":%.3f, \"heading\":%.3f, \"turnAngle\":%.3f},\"quality\":{\"sats\":%.3f,\"hdop\":%.3f,\"vdop\":%.3f,\"pdop\":%.3f, \"fix\":%.3f}}";
