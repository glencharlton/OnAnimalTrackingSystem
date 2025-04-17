/*
Author: Glen Charlton
Last Modified: 2024/03/27
Purpose: This file is used to declare the functions used to
read and write data to the SD card on the OATS v2 board.

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
#ifndef _SD_ESP32_h
#define _SD_ESP32_h

#include "L86.h"
#include <SD.h>
#include <string.h>
#include "datatypes.h"
#include <stdio.h>
#include "network.h"
#include "config.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "driver/rtc_io.h"
#include <SPI.h>

/* Initalise SD Settings */
extern int rtc_sleep_lastalarm_mins;
extern int gps_wakeTime;

class sd_card {
public:
	bool setup();
	bool start();
	void end();
	void file_write(char* filename);
	void file_read(char* filename);
	void file_close();
	bool file_available();
	void file_clear(char* filename);
	void log_accel(accel_struct data);
	void log_gnss(gnss_struct data);
	void log_features(features_struct data);
	void log_location(location_struct data);
	void log_systemlog(systemlog_struct data);
	accel_struct get_accel();
	gnss_struct get_gnss();
	features_struct get_features();
	location_struct get_location();
	systemlog_struct get_systemlog();
};

#endif