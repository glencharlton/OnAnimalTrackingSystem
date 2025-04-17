/*
Author: Glen Charlton
Last Modified: 2024/03/27
Purpose: The main firmware file. 

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

#include "esp32_time.h"
#include "adxl343.h"
#include "network.h"
#include "vbat_esp32.h"
#include "sleep.h"
#include "sd_esp32.h"
#include "L86.h"
#include "datatypes.h"                                                                                                                        
#include "config.h"
#include <stdio.h>

//#define LOG(fmt, ...) (Serial.printf("%d-%d: " fmt "\n", network_time_get(), millis(), ##__VA_ARGS__))
#define LOG(fmt, ...) (Serial.printf("Core%d %d-%d: " fmt "\n", xPortGetCoreID(), network_time_get(), millis(), ##__VA_ARGS__))

ADXL343 adxl343;
L86 gnss;
sd_card sd;

/* Setup Status Variables */
RTC_DATA_ATTR bool device_initalised = false;
RTC_DATA_ATTR bool vbat_initialised = false;
RTC_DATA_ATTR bool sd_initalised = false;
RTC_DATA_ATTR bool accel_initalised = false;
RTC_DATA_ATTR bool gnss_configured = false;
RTC_DATA_ATTR bool gnss_initalised = false;
RTC_DATA_ATTR bool network_initalised = false;
RTC_DATA_ATTR bool low_battery_mode = false;

/* Data Storage Variables */
int accel_samples = 12.5 * sensor_sampling_length; // @ 12.5Hz
int gnss_samples = 1 * sensor_sampling_length;	   // @ 1Hz
int accel_log_start_millis = 0;
int accel_data_index = 0;
int gnss_locktime = 30;			// GNSS settle time once locked acheived (seconds)
const int gnss_locktime_1 = 30; // GNSS settle time once locked acheived (seconds)
const int gnss_locktime_2 = 60;
const int gnss_locktime_3 = 150;

features_struct features_data;
location_struct location_data;

RTC_DATA_ATTR struct systemlog_struct systemlog_data;

/* GNSS Status */
RTC_DATA_ATTR int32_t gnss_timeout = false;
RTC_DATA_ATTR bool gnss_started = false;
RTC_DATA_ATTR bool gnss_ready = false;
RTC_DATA_ATTR bool gnss_settled = false;
RTC_DATA_ATTR bool gnss_logging = false;
RTC_DATA_ATTR bool gnss_completed = false;
RTC_DATA_ATTR int gnss_lock_status = 4;
RTC_DATA_ATTR int gnss_wait_status = 1;
RTC_DATA_ATTR int gnss_skip_counter = 1;

/* Variables for after partial publish to avoid re-sending data to datahub */
RTC_DATA_ATTR int32_t features_lastpublished_ts = 0;
RTC_DATA_ATTR int32_t accel_lastpublished_ts = 0;
RTC_DATA_ATTR int32_t location_lastpublished_ts = 0;
RTC_DATA_ATTR int32_t systemlog_lastpublished_ts = 0
;

/* ESP32 Tasks for each core */
TaskHandle_t Task1;
TaskHandle_t Task2;

/* Task Variables */
bool mqtt_connected = false;
RTC_DATA_ATTR int mqtt_update_time = 0;
bool accel_completed = false;
esp_sleep_wakeup_cause_t wakeup_reason;
int msg_failed = 0;
int msg_count = 0;

/* Pin Definitions */
const int led_pin = 13;
const int accel_int1_pin = 14;
gpio_num_t gnss_pps_pin = GPIO_NUM_35;

void setup()
{
	// Setup
	Serial.begin(115200);
	pinMode(led_pin, OUTPUT);
	pinMode(accel_int1_pin, INPUT);
	pinMode(gnss_pps_pin, INPUT);
	network_config();
	gnss_completed = false;

	/* Wakeup Reason */
	wakeup_reason = esp_sleep_get_wakeup_cause();
	switch (wakeup_reason)
	{
	case ESP_SLEEP_WAKEUP_EXT0:
		LOG("Deep Sleep Wakeup Reason: GNSS PPS  - Device ID: %s - Firmware: %s", client_id, fw_version);
		gnss_ready = true;

		// settle
		if(!gnss_settled)
		{
			LOG("Sleeping for GNSS Settle for: %d", gnss_settle);
			gnss_settled = true;
			sleep_deep_timer(gnss_settle);
		}
		break;
	case ESP_SLEEP_WAKEUP_TIMER:
		if (gnss_started)
		{
			if(gnss_settled)
			{
				LOG("Deep Sleep Wakeup Reason: GNSS Settle Complete - Device ID: %s - Firmware: %s", client_id, fw_version);
				gnss_ready = true;
			}
			else
			{
				LOG("Deep Sleep Wakeup Reason: GNSS Timeout - Device ID: %s - Firmware: %s", client_id, fw_version);
				gnss_ready = false;
			}
		}
		else
		{
			LOG("Deep Sleep Wakeup Reason: Sensor Logging Timer - Device ID: %s - Firmware: %s", client_id, fw_version);
			systemlog_data.ts = network_time_get();
			systemlog_data.vbat = vbat_bat_read();
			systemlog_data.gnss_status = 0;
			systemlog_data.accel_status = 0;
			systemlog_data.network_status = 0;
			LOG("Battery Voltage: %.3f", systemlog_data.vbat);
			gnss_ready = false;
		}
		break;
	default:
		LOG("Startup: %s - Firmware: %s", client_id, fw_version);
		systemlog_data.ts = network_time_get();
		systemlog_data.vbat = vbat_bat_read();
		systemlog_data.gnss_status = 0;
		systemlog_data.accel_status = 0;
		systemlog_data.network_status = 0;
		LOG("Battery Voltage: %.3f", systemlog_data.vbat);
		break;
	}

	/* Check Variables are logical */
	if(sensor_update_interval < 60){
		LOG("Sensor Update Interval not set - Defaulting to 600");
		sensor_update_interval = 600;
	}
	if(mqtt_upload_interval < 600 | mqtt_upload_interval < sensor_update_interval){
		LOG("MQTT Upload Interval not set - Defaulting to 10800");
		mqtt_upload_interval = 10800;
	}
	if(sensor_sampling_length < 5){
		LOG("Sensor Sampling Length not set - Defaulting to 5");
		sensor_sampling_length = 5;
	}
	if(gnss_settle < 5){
		LOG("GNSS Settle not set - Defaulting to 30");
		gnss_settle = 30;
	}
	if(mqtt_update_time - network_time_get() > mqtt_upload_interval){
		LOG("MQTT Update Time not logical - Defaulting to 10800");
		mqtt_update_time = network_time_get() + 10800;
	}

	// Battery Solar harvesting recovery 
	if(power_source == "solar" & systemlog_data.vbat <= battery_off)
	{
		low_battery_mode = true;
		LOG("Battery Low: %.3f - Enter Low Battery Mode", systemlog_data.vbat);	
	}
	if(low_battery_mode){
		if(systemlog_data.vbat <= battery_on)
		{
			// Turn off GPS and SD
			gnss.setup_pins();
			gnss.shutdown();
			sd.setup();
			sd.end();

			// System Log to SD
			systemlog_data.ontime = network_time_get() - systemlog_data.ts;
			LOG("Starting SD");
			sd.start();
			LOG("Writing System log to SD");
			sd.file_write(sd_filename_systemlog);
			sd.log_systemlog(systemlog_data);
			sd.file_close();
			sd.end();
			// Deep Sleep
			delay(1);
			LOG("Sleep for: %d - Time Until Next MQTT Upload (secs): %d", sensor_update_interval, mqtt_update_time - network_time_get());
			delay(100);
			sleep_deep_timer(sensor_update_interval - (millis() / 1000));
		} else {
			low_battery_mode = false;
		}
	}

	// Setup Accel
	if (!accel_initalised)
	{
		digitalWrite(led_pin, HIGH);

		while (!adxl343.setConfig())
		{
			delay(1);
		}
		accel_initalised = true;
		LOG("Accel Setup");
	}
	while (!adxl343.setReconfig())
	{
		delay(1);
	}


	// Setup SD
	if (!sd_initalised)
	{
		while (!sd.setup())
		{
			delay(1);
		}
		//sd.file_clear(sd_filename_accel);
		//sd.file_clear(sd_filename_features);
		//sd.file_clear(sd_filename_gnss);
		//sd.file_clear(sd_filename_systemlog);
		sd_initalised = true;
		LOG("SD Setup");
	}

	// gnss setup
	if (!gnss_configured)
	{
		gnss.startup();
		gnss.setup_pins();
		while (!gnss.setup_serial())
		{
			delay(1);
		}
		LOG("GNSS Setup");
		gnss.shutdown();
		gnss_configured = true;
	}

	if (!device_initalised)
	{
		if (accel_initalised & sd_initalised)
		{
			mqtt_update_time = 1650000000;
			device_initalised = true;
			digitalWrite(led_pin, LOW);
			delay(5000);
			LOG("Initial Setup Complete");
			sleep_deep_timer(1);
		}
		else
		{
			LOG("Initial Setup Failed");
			delay(100);
			sleep_deep_timer(sensor_update_interval);
		}
	}

	// create a task that will be executed in the Task1code() function, with priority 1 and executed on core 0
	xTaskCreatePinnedToCore(
		Task1code, /* Task function. */
		"Task1",   /* name of task. */
		10000,	   /* Stack size of task */
		NULL,	   /* parameter of the task */
		1,		   /* priority of the task */
		&Task1,	   /* Task handle to keep track of created task */
		1);		   /* pin task to core 1 */
	delay(1);
	// create a task that will be executed in the Task2code() function, with priority 1 and executed on core 1
	xTaskCreatePinnedToCore(
		Task2code, /* Task function. */
		"Task2",   /* name of task. */
		10000,	   /* Stack size of task */
		NULL,	   /* parameter of the task */
		1,		   /* priority of the task */
		&Task2,	   /* Task handle to keep track of created task */
		0);		   /* pin task to core 0 */
	delay(1);
}

// Task1code: Reads GNSS Data
void Task1code(void *pvParameters)
{
	for (;;)
	{
		// add GNSS skipping logic
		if(gnss_skip_rate > 1 & gnss_initalised & !gnss_started){
			LOG("GNSS Skipping Logic Active");
			if(gnss_skip_counter >= gnss_skip_rate){
				gnss_wait_status = 1;
				// reset gnss_skip counter
				gnss_skip_counter = 1;
				LOG("GNSS Skip Counter: %d", gnss_skip_counter);
				LOG("GNSS Wait Status: %d", gnss_wait_status);
			} else {			
				// edit wait status
				gnss_wait_status = 2;
				// increment gnss_skip counter
				gnss_skip_counter += 1;
				LOG("GNSS Skipped - GNSS Skip Counter: %d", gnss_skip_counter);
				LOG("GNSS Wait Status: %d", gnss_wait_status);
			}
		}
		
		if (gnss_wait_status == 1)
		{
			if (!gnss_started)
			{
				LOG("Start GNSS Logging");
				systemlog_data.gnss_status += 1;
				// Turn on GPS and set search time
				gnss.startup();
				gnss_started = true;
				// Extended lock time for when GPS lock not found
				if (gnss_lock_status >= 4)
				{
					gnss_locktime = gnss_locktime_3;
				}
				else if (gnss_lock_status == 3)
				{
					gnss_locktime = gnss_locktime_2;
				}
				else
				{
					gnss_locktime = gnss_locktime_1;
				}
				// Sleep until PPS or until GNSS timeout
				gnss_timeout = network_time_get() + gnss_locktime;
				LOG("Waiting for GNSS Lock: %d - GNSS Timeout: %d", gnss_locktime, gnss_timeout);
				sleep_deep_gnss(gnss_locktime - sensor_sampling_length, gnss_pps_pin);
			}

			// Check if GNSS ready based on deep sleep wakeup reason
			if (!gnss_ready)
			{
				LOG("GNSS Fix not available");
			}
			else
			{
				gnss.startup();
				LOG("Continuing GNSS Logging");
				systemlog_data.gnss_status += 1;

				// Main GNSS Loop 
				gnss.get();
				if (gnss.stats_fixquality() >= 1)
				{
					LOG("GNSS Fixed");
					systemlog_data.gnss_status += 1;
					// set time
					gnss.setTime();
					// Initialise gnss
					if (!gnss_initalised)
					{
						LOG("GNSS Initialising");
						gnss_lock_status = 1;
						gnss_initalised = true;
						delay(30000);
						gnss.initTime();
						LOG("GNSS Initialised");
					}

					// collect data
					LOG("GNSS Logging Start");
					gnss_logging = true;
					location_data = gnss.log(gnss_samples);
					
					// gnss shutdown
					gnss_lock_status = 1;
					gnss_logging = false;
					gnss.shutdown();
					gnss_started = false;
					LOG("GNSS Shutdown");

					while (!accel_completed)
					{
						delay(1);
					}
					LOG("Starting SD");
					sd.start();
					LOG("Write Location Data to SD");
					sd.file_write(sd_filename_location);
					sd.log_location(location_data);
					sd.file_close();
					sd.end();
					systemlog_data.gnss_status += 1;

					gnss_settled = false;
					gnss_completed = true;
				}
				else
				{
					LOG("GNSS Fix not available");
					gnss_ready = false;
				}
			}

			if (!gnss_completed | !gnss_ready)
			{
				// shutdown gps
				gnss.shutdown();
				gnss_started = false;
				LOG("GNSS Shutdown");
				// increment lock status
				gnss_lock_status += 1;
				LOG("GNSS lock status: %d", gnss_lock_status);
				// Setting lock and wait status if lock not successful
				if (gnss_lock_status >= 4)
				{
					gnss_wait_status = 4;
					gnss_lock_status = 4;
				}
				else if (gnss_lock_status == 3)
				{
					gnss_wait_status == 2;
				}
				else
				{
					gnss_wait_status = 1;
				}
				LOG("GNSS wait status: %d", gnss_wait_status);
			}
		}
		else
		{
			// decrement wait status
			delay(100);
			gnss_wait_status -= 1;
			LOG("GNSS wait status: %d", gnss_wait_status);
		}	

		delay(100);
		gnss_settled = false;
		gnss_completed = true;
		delay(1200000);
	}
}

// Task2code: Reads Accel Data, Logs to SD and Publishes to MQTT
void Task2code(void *pvParameters)
{
	for (;;)
	{
		accel_completed = false;
		while (!gnss_completed & !accel_completed)
		{
			if (gnss_initalised)
			{
				if (gnss_logging | gnss_wait_status != 1 | !gnss_ready)
				{
					if (!accel_completed)
					{
						systemlog_data.accel_status += 1;
						if(send_processed_accel){
							LOG("Accel Start");
							features_data = adxl343.getFeatures(accel_samples);

							LOG("Starting SD");
							sd.start();
							LOG("Write Features Data to SD");
							sd.file_write(sd_filename_features);
							sd.log_features(features_data);
							sd.file_close();
							sd.end();
						}
						else{
							LOG("Accel Start (raw)");
							int log_start = 0;
							int i = 0;
							int32_t ts = network_time_get();
							int16_t ms = 0;
							accel_struct raw_accel_data[accel_samples];
							adxl343.getAccelData(ts, ms);
							while (i < accel_samples)
							{
								if (millis() >= log_start + 80)
								{
									log_start = millis();
									//get raw data
									raw_accel_data[i] = adxl343.getAccelData(ts, ms);
									// increment timestamps
									ms = ms + 80;
									if(ms > 1000) {
										ms = ms - 1000;
										ts = ts + 1;
									}
									i++;
								}
							}
							LOG("Write Raw Accel Data to SD");
							for(int j=0; j<=i; j++){
								LOG("Starting SD");
								sd.start();
								LOG("Writing Raw Accel Data to SD");
								sd.file_write(sd_filename_accel);
								sd.log_accel(raw_accel_data[j]);
								sd.file_close();
								sd.end();
							}
						}
						systemlog_data.gnss_status += 1;

						accel_completed = true;
					}
					else
					{
						delay(1);
					}
				}
				else
				{
					delay(1);
				}
			}
			else
			{
				delay(1);
			}
		}

		// Wait for GNSS to be completed
		while(!gnss_completed)
		{
			delay(1);
		}

		// Check if time to publish
		if ((network_time_get()) > mqtt_update_time & gnss_skip_counter == 1)
		{
			// Publish Data to MQTT
			LOG("Publishing to MQTT");
			publish_data(20, 2);
		}
		
		// System Log to SD
		systemlog_data.ontime = network_time_get() - systemlog_data.ts;
		LOG("Starting SD");
		sd.start();
		LOG("Write System Log Data to SD");
		sd.file_write(sd_filename_systemlog);
		sd.log_systemlog(systemlog_data);
		sd.file_close();
		sd.end();

		// Deep Sleep
		delay(1);
		gnss.shutdown();
		LOG("Sleep for: %d - Time Until Next MQTT Upload (secs): %d", sensor_update_interval, mqtt_update_time - network_time_get());
		delay(100);
		sleep_deep_timer(sensor_update_interval - (millis() / 1000));
	}
}

void loop() { delay(1000); } // never used

void publish_data(int wifi, int mqtt)
{
	systemlog_data.network_status += 1;
	// Connect to Wifi
	if (network_wifi_setup(wifi))
	{
		LOG("Network Connected");
		systemlog_data.network_status += 1;

					// Define next data upload time
		mqtt_update_time = network_time_get() + mqtt_upload_interval;
		LOG("Next MQTT Update - %d", mqtt_update_time);

		// Connect to MQTT
		if (network_mqtt_setup(mqtt))
		{
			LOG("MQTT Connected");
			systemlog_data.network_status += 1;
			mqtt_connected = true;

			// Initialise network if required else wait for sensor data to be collected
			if (!network_initalised)
			{
				snprintf(msg, MSG_SIZE, init_json_template, client_id, fw_version);
				network_mqtt_publish(mqtt_init_topic, msg);
				LOG("MQTT MSG: %s", msg);
				network_initalised = true;
			} else {
				// Wait for Sensor Data to be collected
				LOG("Waiting for sensor data to be collected");
				while (!accel_completed)
				{
					delay(1);
				}
				while(!gnss_completed)
				{
					delay(1);
				}
			}

			// Read and Publish GNSS Data from SD
			LOG("Starting SD");
			sd.start();
			LOG("Publishing GNSS Data from SD");
			sd.file_read(sd_filename_location);
			while (sd.file_available() & mqtt_connected)
			{
				location_struct sd_location_data = sd.get_location();
				if (sd_location_data.ts >= location_lastpublished_ts)
				{
					location_lastpublished_ts = sd_location_data.ts;
					snprintf(msg, MSG_SIZE, location_json_template,
							client_id,
							sd_location_data.ts,
							sd_location_data.lat,
							sd_location_data.lon,
							sd_location_data.alt,
							sd_location_data.spd,
							sd_location_data.duration,
							sd_location_data.distance,
							sd_location_data.heading,
							sd_location_data.turn_angle,
							sd_location_data.sats,
							sd_location_data.hdop,
							sd_location_data.vdop,
							sd_location_data.pdop,
							sd_location_data.fix
							);
					if(network_mqtt_publish(mqtt_location_topic, msg))
					{
						LOG("Location MQTT - %s", msg);	
						location_lastpublished_ts = sd_location_data.ts;
						msg_count += 1;
					} else{
						LOG("Location MQTT Publish Failed- %d", sd_location_data.ts);
						mqtt_connected = false;
						delay(100);
					}					
					delay(100);
				}
				else
				{
					delay(1);
				}
			}
			if (mqtt_connected)
			{
				LOG("GNSS MQTT - Complete Publish - Messages: %d", msg_count);
				msg_count = 0;
				sd.file_close();
				delay(10);
				sd.file_clear(sd_filename_location);
				delay(10);
				sd.end();
			}
			else
			{
				LOG("GNSS MQTT - Partial Publish - Messages: %d", msg_count);
				msg_count = 0;
				sd.file_close();
				delay(10);
				sd.end();
			}

			
			// Read and Publish Features Data from SD
			LOG("Starting SD");
			sd.start();
			LOG("Publishing Features Data from SD");
			sd.file_read(sd_filename_features);
			msg_count = 0;
			while (sd.file_available() & mqtt_connected)
			{
				features_struct sd_features_data = sd.get_features();
				if (sd_features_data.ts >= features_lastpublished_ts)
				{
					snprintf(msg, MSG_SIZE, features_json_template,
							client_id,
							sd_features_data.ts,
							sensor_sampling_length,
							sd_features_data.mean_x,
							sd_features_data.mean_y,
							sd_features_data.mean_z,
							sd_features_data.mean_abs,
							sd_features_data.min_x,
							sd_features_data.min_y,
							sd_features_data.min_z,
							sd_features_data.min_abs,
							sd_features_data.max_x,
							sd_features_data.max_y,
							sd_features_data.max_z,
							sd_features_data.max_abs,
							sd_features_data.mean_mv_x,
							sd_features_data.mean_mv_y,
							sd_features_data.mean_mv_z,
							sd_features_data.mean_mv_abs,
							sd_features_data.min_mv_x,
							sd_features_data.min_mv_y,
							sd_features_data.min_mv_z,
							sd_features_data.min_mv_abs,
							sd_features_data.max_mv_x,
							sd_features_data.max_mv_y,
							sd_features_data.max_mv_z,
							sd_features_data.max_mv_abs);
					
					if(network_mqtt_publish(mqtt_features_topic, msg))
					{
						LOG("Features MQTT - %s", msg);	
						features_lastpublished_ts = sd_features_data.ts;
						msg_count += 1;
					} else{
						LOG("Features MQTT Publish Failed- %d", sd_features_data.ts);
						mqtt_connected = false;
						delay(100);
					}					
					delay(100);
				} else{
					delay(1);
				}

			}
			if (mqtt_connected)
			{
				LOG("Features MQTT - Complete Publish - Messages: %d", msg_count);
				msg_count = 0;
				sd.file_close();
				delay(10);
				sd.file_clear(sd_filename_features);
				delay(10);
				sd.end();
			}
			else
			{
				LOG("Features MQTT - Partial Publish - Messages: %d", msg_count);
				msg_count = 0;
				sd.file_close();
				delay(10);
				sd.end();
			}

			// Read and Publish Accel Data from SD
			LOG("Starting SD");
			sd.file_read(sd_filename_accel);
			LOG("Publishing Raw Accel Data from SD");
			msg_count = 0;
			while (sd.file_available() & mqtt_connected)
			{
				accel_struct sd_accel_data = sd.get_accel();
				if (sd_accel_data.ts >= accel_lastpublished_ts)
				{
					snprintf(msg, MSG_SIZE, accel_json_template,
							client_id,
							sd_accel_data.ts,
							sd_accel_data.ms,
							sd_accel_data.ax,
							sd_accel_data.ay,
							sd_accel_data.az,
							sd_accel_data.abs);					
					if(network_mqtt_publish(mqtt_accel_topic, msg))
					{
						LOG("Accel MQTT - %s", msg);	
						accel_lastpublished_ts = sd_accel_data.ts;
						msg_count += 1;
					} else{
						LOG("Accel MQTT Publish Failed- %d", sd_accel_data.ts);
						mqtt_connected = false;
						delay(100);
					}					
					delay(100);
				} else{
					delay(1);
				}
			}
			if (mqtt_connected)
			{
				LOG("Accel MQTT - Complete Publish - Messages: %d", msg_count);
				msg_count = 0;
				sd.file_close();
				delay(10);
				sd.file_clear(sd_filename_accel);
				delay(10);
				sd.end();
			}
			else
			{
				LOG("Accel MQTT - Partial Publish - Messages: %d", msg_count);
				msg_count = 0;
				sd.file_close();
				delay(10);
				sd.end();
			}

			// Publish System Log
			// Read and Publish system Data from SD
			LOG("Starting SD");
			sd.start();
			LOG("Publishing System Log Data from SD");
			sd.file_read(sd_filename_systemlog);
			while (sd.file_available() & mqtt_connected)
			{
				systemlog_struct sd_systemlog_data = sd.get_systemlog();
				if (sd_systemlog_data.ts >= systemlog_lastpublished_ts)
				{
					systemlog_lastpublished_ts = sd_systemlog_data.ts;
					snprintf(msg, MSG_SIZE, systemlog_json_template,
						client_id, fw_version,
						sd_systemlog_data.ts,
						sd_systemlog_data.vbat,
						sd_systemlog_data.ontime,
						sd_systemlog_data.gnss_status,
						sd_systemlog_data.accel_status,
						sd_systemlog_data.network_status);
					if(network_mqtt_publish(mqtt_systemlog_topic, msg))
					{
						LOG("System Log MQTT - %s", msg);	
						systemlog_lastpublished_ts = sd_systemlog_data.ts;
						msg_count += 1;
					} else{
						LOG("systemlog MQTT Publish Failed- %d", sd_systemlog_data.ts);
						mqtt_connected = false;
						delay(100);
					}					
					delay(100);
				}
				else
				{
					delay(1);
				}
			}
			if (mqtt_connected)
			{
				LOG("System Log MQTT - Complete Publish - Messages: %d", msg_count);
				msg_count = 0;
				sd.file_close();
				delay(10);
				sd.file_clear(sd_filename_systemlog);
				delay(10);
				sd.end();
			}
			else
			{
				LOG("System Log MQTT - Partial Publish - Messages: %d", msg_count);
				msg_count = 0;
				sd.file_close();
				delay(10);
				sd.end();
			}

			if (mqtt_connected)
			{
				systemlog_data.network_status += 1;
				network_config_set();
				LOG("Config Received and Updated");
				/* Subscribe to for system updates */
				LOG("Sensor Update Interval: %d", sensor_update_interval);
				LOG("MQTT Update Interval: %d", mqtt_upload_interval);
				delay(10);
			}
		}
		LOG("MQTT Shutdown");
		network_mqtt_shutdown();
	}
	LOG("Wifi Shutdown");
	network_wifi_shutdown();
}