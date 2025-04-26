/*
Author: Glen Charlton
Last Modified: 2024/03/27
Purpose: This file is used to declare the data structures used in the OATS v2 board.

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

#ifndef _DATATYPES_h
#define _DATATYPES_h

struct features_struct {
	int32_t ts;
	float mean_x;
	float mean_y;
	float mean_z;
	float mean_abs;
	float min_x;
	float min_y;
	float min_z;
	float min_abs;
	float max_x;
	float max_y;
	float max_z;
	float max_abs;
	float mean_mv_x;
	float mean_mv_y;
	float mean_mv_z;
	float mean_mv_abs;
	float min_mv_x;
	float min_mv_y;
	float min_mv_z;
	float min_mv_abs;
	float max_mv_x;
	float max_mv_y;
	float max_mv_z;
	float max_mv_abs;
};

struct accel_struct {
	int32_t ts;
	int32_t ms;
    float ax;
    float ay;
    float az;
	float abs;
} ;


struct gnss_struct {
	int32_t ts;
	float lat;
	float lon;
	float alt;
	float spd;
	float sats;
	float hdop;
	float vdop;
	float pdop;
	float fix;
};

struct location_struct {
	int32_t ts;
	float lat;
	float lon;
	float alt;
	float spd;
	int8_t duration;
	float distance;
	float heading;
	float turn_angle;
	float sats;
	float hdop;
	float vdop;
	float pdop;
	float fix;
};

struct systemlog_struct {
	int32_t ts;
	float vbat;
	int32_t ontime;
	int8_t gnss_status;
	int8_t accel_status;
	int8_t network_status;
};

#endif