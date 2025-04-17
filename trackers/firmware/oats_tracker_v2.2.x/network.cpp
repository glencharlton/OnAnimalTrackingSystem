/*
Author: Glen Charlton
Last Modified: 2024/03/27

Purpose: This file is used to declare the functions used to
setup and manage the network connections and data transfer
on the OATS v2 board.
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

#include "network.h"

#define LOG(fmt, ...) (Serial.printf("%d-%d: " fmt "\n", network_time_get(), millis(), ##__VA_ARGS__))

WiFiClient espClient;
PubSubClient client(espClient);
ESP32Time rtc;

bool config_returned = false;
bool time_returned = false;

bool network_found = false;
int32_t network_rssi;
uint8_t *network_bssid;

void network_config()
{
	setCpuFrequencyMhz(80); // Reduce clock speed to 80MHzdelay(100);
	btStop();				// Disable Bluetooth
	adc_power_off();
	esp_bt_controller_disable();
}

bool network_wifi_setup(int secs)
{
	WiFi.disconnect(false); // Reconnect the network
	WiFi.mode(WIFI_STA);	// Switch WiFi on station mode

	int n = WiFi.scanNetworks(false, false, false, 200, wifi_channel);
	if (n == 0)
	{
	}
	else
	{
		network_found = false;
		network_rssi = -96;
		for (int i = 0; i < n; ++i)
		{
			LOG("Network available - SSID: %s - BSSID: %d - RSSI: %d", WiFi.SSID(i), WiFi.BSSID(i), WiFi.RSSI(i));
			if (WiFi.SSID(i) == ssid & WiFi.RSSI(i) >= -94)
			{
				if (WiFi.RSSI(i) > network_rssi)
				{
					network_bssid = WiFi.BSSID(i);
					network_rssi = WiFi.RSSI(i);
					network_found = true;
				}
			}
		}
		/* connect */
		if (network_found)
		{
			LOG("Connecting to Network- SSID: %s - BSSID: %d - RSSI: %d", ssid, network_bssid, network_rssi);
			WiFi.begin(ssid, password, wifi_channel, network_bssid);
			for (int s = 1; s <= (secs * 10); s++)
			{
				if (WiFi.status() != WL_CONNECTED)
				{
					delay(100);
				}
				else
				{
					esp_wifi_set_ps(WIFI_PS_MAX_MODEM);
					return true;
				}
			}
			if (WiFi.status() != WL_CONNECTED)
			{
				return false;
			}
		}
	}
	return (false);
}

void network_mqtt_shutdown()
{
	client.disconnect(); // trigger disconnection from MQTT broker
	espClient.flush();	 // wait until connection is closed completely
	while (client.state() != -1) // i.e. all pending message sent
	{ 
		delay(10);
	}
}

void network_wifi_shutdown()
{
	WiFi.disconnect(true); // Disconnect from the network
	WiFi.mode(WIFI_OFF);   // Switch WiFi off
	setCpuFrequencyMhz(80);
}

bool network_mqtt_setup(int secs)
{
	/* Start MQTT Connection */
	client.setServer(mqttServer, mqttPort);
	client.setCallback(network_mqtt_callback);
	LOG("Connecting to MQTT Broker - %s:%d", mqttServer, mqttPort);
	if (client.connect(client_id))
	{
		LOG("Connected to MQTT Broker");
		return true;
	}
	else
	{
		LOG("Failed to connect to MQTT Broker");
		return false;
	}
}

bool network_mqtt_publish(char *topic, char *msg)
{
	return client.publish(topic, msg, false);
}

void network_mqtt_callback(char *topic, byte *payload, unsigned int length)
{
	
	if (strcmp(topic, mqtt_time_topic) == 0)
	{
		payload[length] = '\0';
		String s = String((char *)payload);
		rtc.setTime(s.toInt());
		time_returned = true;
	}

	if (strcmp(topic, mqtt_config_topic) == 0)
	{
		payload[length] = '\0';

		StaticJsonDocument<200> doc;
		DeserializationError error = deserializeJson(doc, (char *)payload);
		if (error)
		{
			LOG("deserializeJson() failed: %s", error.f_str());
		}
		else
		{
			mqtt_upload_interval = doc["dataUploadInterval"];
			sensor_update_interval = doc["sensorUpdateInterval"];
			sensor_sampling_length = doc["sensorSamplingLength"];
			gnss_settle = doc["gnssSettle"];
			gnss_skip_rate = doc["gnssSkipRate"];
			config_returned = true;
		}
	}
}

bool network_mqtt_connected()
{
	return client.connected();
}

int32_t network_time_get()
{
	return (rtc.getEpoch());
}

int64_t network_millis_get()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (tv.tv_sec * 1000LL + (tv.tv_usec / 1000LL));
}

void network_time_set()
{
	client.subscribe(mqtt_time_topic);
	time_returned = false;
	int start = millis();
	while (!time_returned & millis() < start + 2000)
	{
		client.loop();
	}
	client.unsubscribe(mqtt_time_topic);
}

void network_config_set()
{
	client.subscribe(mqtt_config_topic);
	config_returned = false;

	int interval_timer = millis();
	while (!config_returned)
	{
		client.loop();
		if (millis() > interval_timer + 1500)
		{
			config_returned = true;
		}
	}
	client.unsubscribe(mqtt_config_topic);
}