/*
Author: Glen Charlton
Last Modified: 2024/03/27
Purpose: The sleep.h file is used to declare the functions used to
put the OATS v2 board into sleep mode.

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

#include "sleep.h"

void sleep_deep_timer(int time) {
	
	esp_sleep_enable_timer_wakeup(time *1000000); //deep sleep for "time" in seconds
	delay(1);
	esp_deep_sleep_start();
}

void sleep_deep_gnss(int time, gpio_num_t pin) {
	
	esp_sleep_enable_timer_wakeup(time *1000000); //deep sleep for "time" in seconds
	esp_sleep_enable_ext0_wakeup(pin, 1);
	delay(1);
	esp_deep_sleep_start();
}

void sleep_deep_int(gpio_num_t pin) {
	esp_sleep_enable_ext0_wakeup(pin, 1);
	delay(1);
	WiFi.disconnect(true);
	WiFi.mode(WIFI_OFF);
	btStop();
	adc_power_off();
	esp_wifi_stop();
	esp_bt_controller_disable();
	esp_deep_sleep_start();
}

