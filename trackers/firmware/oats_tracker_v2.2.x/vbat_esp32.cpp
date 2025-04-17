/*
Author: Glen Charlton
Last Modified: 2024/03/27
Purpose: This file is used to declare the functions used to 
read the battery level of the OATS v2 board.

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

#include "vbat_esp32.h"
#include <arduino.h>

gpio_num_t vbat_en_pin = GPIO_NUM_4;
const int vbat_read_pin = 34;

float vbat_bat_read() {

	rtc_gpio_init(vbat_en_pin);
	rtc_gpio_set_direction(vbat_en_pin, RTC_GPIO_MODE_OUTPUT_ONLY);
	gpio_hold_dis(vbat_en_pin);
	rtc_gpio_set_level(vbat_en_pin, 0);
	gpio_hold_en(vbat_en_pin);
	pinMode(vbat_read_pin, INPUT); //VBAT read pin as input
	delay(10);
	
	float vbat = 0;
	for(int i=0; i<10; i++)
	{
		vbat += (analogRead(vbat_read_pin) / 4095.0) * 2.0 * 3.3 * 1.1; // read VBAT
		delay(1);
	}
	vbat = vbat / 10;

	gpio_hold_dis(vbat_en_pin);
	rtc_gpio_set_level(vbat_en_pin, 1);
	gpio_hold_en(vbat_en_pin);

	return (vbat);
}

