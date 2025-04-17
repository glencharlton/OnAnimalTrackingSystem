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
#include "sd_esp32.h"
#include <arduino.h>
;

File dataFile;
gpio_num_t sd_en_pin = GPIO_NUM_33;
int sd_cs = 25;
bool sd_busy = false;

bool sd_card::setup() {
	rtc_gpio_init(sd_en_pin);
	rtc_gpio_set_direction(sd_en_pin, RTC_GPIO_MODE_OUTPUT_ONLY);
	gpio_hold_dis(sd_en_pin);
	rtc_gpio_set_level(sd_en_pin, 0); //GPIO HIGH
	gpio_hold_en(sd_en_pin);

	pinMode(33, OUTPUT);
	SPI.begin(26, 27, 12, sd_cs);
	SPI.setDataMode(SPI_MODE0);

	if (SD.begin(sd_cs)) {
		return true;
	}
	else {
		return false;
	}
}

bool sd_card::start() {
	while(sd_busy)
	{
		delay(1);
	}
	sd_busy = true;
	rtc_gpio_set_level(sd_en_pin, 0); 
	if (SD.begin(sd_cs)) {
		return true;
	}
	else {
		return false;
	}
}

void sd_card::end() {
	SD.end();
	gpio_hold_dis(sd_en_pin);
	rtc_gpio_set_level(sd_en_pin, 1);
	gpio_hold_en(sd_en_pin);
	sd_busy = false;
}

void sd_card::file_write(char* filename) {
	dataFile = SD.open(filename, FILE_APPEND);
}

void sd_card::file_read(char* filename) {
	dataFile = SD.open(filename, FILE_READ);
}

void sd_card::file_close() {
	dataFile.close();
}

bool sd_card::file_available() {
	return (dataFile.available());
}

void sd_card::file_clear(char* filename) {
	SD.remove(filename);
}

void sd_card::log_accel(accel_struct data) {
	dataFile.write((uint8_t*)&data, sizeof(data));
}

void sd_card::log_gnss(gnss_struct data) {
	dataFile.write((uint8_t*)&data, sizeof(data));
}

void sd_card::log_systemlog(systemlog_struct data) {
	dataFile.write((uint8_t*)&data, sizeof(data));
}

void sd_card::log_features(features_struct data) {
	dataFile.write((uint8_t*)&data, sizeof(data));
}

void sd_card::log_location(location_struct data) {
	dataFile.write((uint8_t*)&data, sizeof(data));
}

accel_struct sd_card::get_accel() {
	accel_struct data;
	dataFile.read((uint8_t*)&data, sizeof(data));
	return data;
}

gnss_struct sd_card::get_gnss() {
	gnss_struct data;
	dataFile.read((uint8_t*)&data, sizeof(data));
	return data;
}

features_struct sd_card::get_features() {
	features_struct data;
	dataFile.read((uint8_t*)&data, sizeof(data));
	return data;
}

location_struct sd_card::get_location() {
	location_struct data;
	dataFile.read((uint8_t*)&data, sizeof(data));
	return data;
}

systemlog_struct sd_card::get_systemlog() {
	systemlog_struct data;
	dataFile.read((uint8_t*)&data, sizeof(data));
	return data;
}
