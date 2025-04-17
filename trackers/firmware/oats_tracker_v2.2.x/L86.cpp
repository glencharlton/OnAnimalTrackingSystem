/*
Created by Glen Charlton
For OATS v2 boards 
L86 GNSS module Definitions and Functions
 
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

Source code from Adafruit_GPS libary used within this file:

Software License Agreement (BSD License)

Copyright (c) 2012, Adafruit Industries
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

#include "L86.h"
HardwareSerial GPSSerial(2); // UART1/Serial1 pins 16,17
ESP32Time esp_rtc;

/* Initalise GPS Settings */
int gps_interval_max = 1000;
int gps_interval_timer = 0;
gpio_num_t gps_en_pin = GPIO_NUM_32;

bool L86::setup_pins()
{
	rtc_gpio_init(gps_en_pin);
	rtc_gpio_set_direction(gps_en_pin, RTC_GPIO_MODE_OUTPUT_ONLY);
	rtc_gpio_set_level(gps_en_pin, 0); // GPIO LOW
  return true;
}

bool L86::setup_serial()
{
	GPSSerial.begin(9600);
	delay(1000);
	if (GPSSerial.available())
	{
		delay(10);
		L86::sendCommand(PMTK_API_SET_FIX_CTL_1HZ);
		L86::sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);
		L86::sendCommand(PMTK_PPS_WAITFORFIX); 
    L86::sendCommand("$PMTK001,285,3*3F");

		delay(1000);
		return true;
	}
	else
	{
		return false;
	}
}


bool L86::init()
{
	L86::get();
	if (L86::stats_fixquality() >= 1)
	{
		return true;
	}
	else
	{
		return false;
	}
}

void L86::write(int write_time)
{
	gps_interval_timer = millis();
	while (millis() < gps_interval_timer + (write_time * 1000))
	{
		char c = L86::read();
		Serial.print(c);
	}
	delay(1000);
}

void L86::get()
{
	gps_interval_timer = millis();
	while (millis() < gps_interval_timer + gps_interval_max)
	{
		char c = L86::read();
		if (L86::newNMEAreceived())
		{
			L86::parse(L86::lastNMEA());
		}
	}
}

location_struct L86::log(int samples)
{
	// data structures
	gnss_struct gnss_log[samples];
	location_struct location;
	

	for (int i = 0; i < samples; i++)
	{
		L86::get();
		
		// Restart Logging if 3D fix attained
		if(i > 0)
		{
			if(L86::stats_fixquality_3d() >= 3 & gnss_log[i-1].fix < 3)
			{
				i = 0;
			}
		}

		// Collect raw GNSS data
		gnss_log[i].ts = network_time_get();
		gnss_log[i].lat = L86::location_lat();
		gnss_log[i].lon = L86::location_lon();
		gnss_log[i].alt = L86::location_alt();
		gnss_log[i].spd = L86::location_spd();
		gnss_log[i].sats = L86::stats_sats();
		gnss_log[i].hdop = L86::stats_hdop();
		gnss_log[i].vdop = L86::stats_vdop();
		gnss_log[i].pdop = L86::stats_pdop();
		gnss_log[i].fix = (float)L86::stats_fixquality_3d();
	}
	//time
	location.ts = gnss_log[0].ts;
	location.duration = samples;

	//Calculate Variables
	int i = 0, j = 0;
	float temp = 0;
	
	//Latitude
	i = 0, j = 0, temp = 0;
	for (i = 0; i < samples; i++)
	{
		for (j = 0; j < samples - 1; j++)
		{
			if (gnss_log[j].lat > gnss_log[j + 1].lat)
			{
				temp = gnss_log[j].lat;
				gnss_log[j].lat = gnss_log[j + 1].lat;
				gnss_log[j + 1].lat = temp;
			}
		}
	}
	if (samples % 2 == 0)
	{
		location.lat = (gnss_log[(samples - 1) / 2].lat + gnss_log[samples / 2].lat) / 2.0;
	}
	else
	{
		location.lat = gnss_log[samples / 2].lat;
	}

	//Longitude
	i = 0, j = 0, temp = 0;
	for (i = 0; i < samples; i++)
	{
		for (j = 0; j < samples - 1; j++)
		{
			if (gnss_log[j].lon > gnss_log[j + 1].lon)
			{
				temp = gnss_log[j].lon;
				gnss_log[j].lon = gnss_log[j + 1].lon;
				gnss_log[j + 1].lon = temp;
			}
		}
	}
	if (samples % 2 == 0)
	{
		location.lon = (gnss_log[(samples - 1) / 2].lon + gnss_log[samples / 2].lon) / 2.0;
	}
	else
	{
		location.lon = gnss_log[samples / 2].lon;
	}

	// Altitude
	i = 0, j = 0, temp = 0;
	for (i = 0; i < samples; i++)
	{
		for (j = 0; j < samples - 1; j++)
		{
			if (gnss_log[j].alt > gnss_log[j + 1].alt)
			{
				temp = gnss_log[j].alt;
				gnss_log[j].alt = gnss_log[j + 1].alt;
				gnss_log[j + 1].alt = temp;
			}
		}
	}
	if (samples % 2 == 0)
	{
		location.alt = (gnss_log[(samples - 1) / 2].alt + gnss_log[samples / 2].alt) / 2.0;
	}
	else
	{
		location.alt = gnss_log[samples / 2].alt;
	}

	// Speed
	i = 0, j = 0, temp = 0;
	for (i = 0; i < samples; i++)
	{
		for (j = 0; j < samples - 1; j++)
		{
			if (gnss_log[j].spd > gnss_log[j + 1].spd)
			{
				temp = gnss_log[j].spd;
				gnss_log[j].spd = gnss_log[j + 1].spd;
				gnss_log[j + 1].spd = temp;
			}
		}
	}
	if (samples % 2 == 0)
	{
		location.spd = (gnss_log[(samples - 1) / 2].spd + gnss_log[samples / 2].spd) / 2.0;
	}
	else
	{
		location.spd = gnss_log[samples / 2].spd;
	}

	//sats
	i = 0, temp = 0;
	for (i = 0; i < samples; i++)
	{
		temp += gnss_log[i].sats;
	}
	location.sats = temp / samples;

	//hdop
	i = 0, temp = 0;
	for (i = 0; i < samples; i++)
	{
		temp += gnss_log[i].hdop;
	}
	location.hdop = temp / samples;

	//vdop
	i = 0, temp = 0;
	for (i = 0; i < samples; i++)
	{
		temp += gnss_log[i].vdop;
	}
	location.vdop = temp / samples;

	//pdop
	i = 0, temp = 0;
	for (i = 0; i < samples; i++)
	{
		temp += gnss_log[i].pdop;
	}
	location.pdop = temp / samples;

	//fix
	i = 0, temp = 0;
	for (i = 0; i < samples; i++)
	{
		temp += gnss_log[i].fix;
	}
	location.fix = temp / samples;

	//distance
	i = 0, temp = 0;
	for (i = 1; i < samples; i++)
	{
		temp += L86::calc_distance(gnss_log[i-1].lat, gnss_log[i-1].lon, gnss_log[i].lat, gnss_log[i].lon);
	}
	location.distance = temp;

	//distance
	i = 0, temp = 0;
	float temp2 = 0;
	for (i = 1; i < samples; i++)
	{
		temp2 += abs(temp - L86::calc_bearing(gnss_log[i-1].lat, gnss_log[i-1].lon, gnss_log[i].lat, gnss_log[i].lon));
		temp += L86::calc_bearing(gnss_log[i-1].lat, gnss_log[i-1].lon, gnss_log[i].lat, gnss_log[i].lon);
	}
	location.heading= temp /samples;
	location.turn_angle = temp2 /samples;

	return(location);
}

void L86::shutdown()
{
	GPSSerial.flush();
	GPSSerial.end();
	gpio_hold_dis(gps_en_pin);
	rtc_gpio_set_level(gps_en_pin, 1); // GPIO HIGH
	gpio_hold_en(gps_en_pin);
}

void L86::startup()
{
	GPSSerial.begin(9600);
	gpio_hold_dis(gps_en_pin);
	rtc_gpio_set_level(gps_en_pin, 0); // GPIO HIGH
	gpio_hold_en(gps_en_pin);
}

void L86::initTime()
{
    esp_rtc.setTime(L86::time_unix());
}

void L86::setTime()
{
	if(((abs(esp_rtc.getEpoch() - L86::time_unix()) <= 86400) & (L86::time_unix() > 1668000000))|((esp_rtc.getEpoch() < 1668000000)&(L86::time_unix() > 1668000000))){
    esp_rtc.setTime(L86::time_unix());
  }
}

int32_t L86::time_unix()
{
	struct tm tm;

	tm.tm_isdst = -1;
	tm.tm_yday = 0;
	tm.tm_wday = 0;
	tm.tm_year = L86::year + 100;
	tm.tm_mon = L86::month - 1;
	tm.tm_mday = L86::day;
	tm.tm_hour = L86::hour;
	tm.tm_min = L86::minute;
	tm.tm_sec = L86::seconds;
	return mktime(&tm);
}

float L86::location_lat()
{
	return L86::latitude_fixed *0.0000001;
}

float L86::location_lon()
{
	return L86::longitude_fixed *0.0000001;
}

float L86::location_alt()
{
	return L86::altitude;
}

float L86::location_spd()
{
	return L86::speed *0.514444;
}

uint8_t L86::stats_fixquality()
{
	return L86::fixquality;
}

uint8_t L86::stats_fixquality_3d()
{
	return L86::fixquality_3d;
}

float L86::stats_sats()
{
	return (float)L86::satellites;
}

float L86::stats_hdop()
{
	return (float)L86::HDOP;
}

float L86::stats_vdop()
{
	return (float)L86::VDOP;
}

float L86::stats_pdop()
{
	return (float)L86::PDOP;
}

float L86::calc_distance(float lat1,float lon1,float lat2,float lon2)
{
    lat1 = radians(lat1);
    lat2 = radians(lat2);
    float dlat = radians(lat2-lat1);
    float dlon = radians(lon2-lon1);

	float distance = sin(dlat/2) * sin(dlat/2) + sin(dlon/2) * sin(dlon/2) * cos(lat1) * cos(lat2); 
	distance = 2 * atan2(sqrt(distance), sqrt(1-distance)); 
    return distance * 6371000;
}

float L86::calc_bearing(float lat1,float lon1,float lat2,float lon2)
{
    lat1 = radians(lat1);
    lat2 = radians(lat2);
    float dlat = radians(lat2-lat1);
    float dlon = radians(lon2-lon1);

    float y = sin(dlon) * cos(dlat);
    float x = cos(lat1)*sin(lat2) - sin(lat1)*cos(lat2)*cos(dlon);
    float brng = atan2(y,x);
    brng = degrees(brng);// radians to degrees
    brng = (((int)brng + 360) % 360 ); 

    return brng;
}


bool L86::begin(uint32_t baud) {
  GPSSerial.begin(baud);
  delay(10);
  return true;
}

/**************************************************************************/
/*!
    @brief Constructor when using HardwareSerial
    @param ser Pointer to a HardwareSerial object
*/
/**************************************************************************/
L86::L86() {
  recvdflag = false;
  paused = false;
  lineidx = 0;
  currentline = line1;
  lastline = line2;

  hour = minute = seconds = year = month = day = fixquality = fixquality_3d =
      satellites = 0;  // uint8_t
  lat = lon = mag = 0; // char
  fix = false;         // bool
  milliseconds = 0;    // uint16_t
  latitude = longitude = geoidheight = altitude = speed = angle = magvariation = HDOP = VDOP = PDOP = 0.0; // nmea_float_t
}


bool L86::available(void) {
    return GPSSerial.available();
}

/**************************************************************************/
/*!
    @brief Write a byte to the underlying transport - part of 'Print'-class
   functionality
    @param c A single byte to send
    @return Bytes written - 1 on success, 0 on failure
*/
/**************************************************************************/
size_t L86::write(uint8_t c) {
    return GPSSerial.write(c);
}

/**************************************************************************/
/*!
    @brief Read one character from the GPS device.

    Call very frequently and multiple times per opportunity or the buffer
    may overflow if there are frequent NMEA sentences. An 82 character NMEA
    sentence 10 times per second will require 820 calls per second, and
    once a loop() may not be enough. Check for newNMEAreceived() after at
    least every 10 calls, or you may miss some short sentences.
    @return The character that we received, or 0 if nothing was available
*/
/**************************************************************************/
char L86::read(void) {
  static uint32_t firstChar = 0; // first character received in current sentence
  uint32_t tStart = millis();    // as close as we can get to time char was sent
  char c = 0;

	if (!GPSSerial.available())
	{
		return c;
	}
	c = GPSSerial.read();

  currentline[lineidx++] = c;
  if (lineidx >= MAXLINELENGTH)
    lineidx = MAXLINELENGTH -
              1; // ensure there is someplace to put the next received character

  if (c == '\n') {
    currentline[lineidx] = 0;

    if (currentline == line1) {
      currentline = line2;
      lastline = line1;
    } else {
      currentline = line1;
      lastline = line2;
    }

    lineidx = 0;
    recvdflag = true;
    recvdTime = millis(); // time we got the end of the string
    sentTime = firstChar;
    firstChar = 0; // there are no characters yet
    return c;      // wait until next character to set time
  }

  if (firstChar == 0)
    firstChar = tStart;
  return c;
}

/**************************************************************************/
/*!
    @brief Send a command to the GPS device
    @param str Pointer to a string holding the command to send
*/
/**************************************************************************/
void L86::sendCommand(const char *str) { GPSSerial.println(str); }

/**************************************************************************/
/*!
    @brief Check to see if a new NMEA line has been received
    @return True if received, false if not
*/
/**************************************************************************/
bool L86::newNMEAreceived(void) { return recvdflag; }

/**************************************************************************/
/*!
    @brief Pause/unpause receiving new data
    @param p True = pause, false = unpause
*/
/**************************************************************************/
void L86::pause(bool p) { paused = p; }

/**************************************************************************/
/*!
    @brief Returns the last NMEA line received and unsets the received flag
    @return Pointer to the last line string
*/
/**************************************************************************/
char *L86::lastNMEA(void) {
  recvdflag = false;
  return (char *)lastline;
}

/**************************************************************************/
/*!
    @brief Wait for a specified sentence from the device
    @param wait4me Pointer to a string holding the desired response
    @param max How long to wait, default is MAXWAITSENTENCE
    @param usingInterrupts True if using interrupts to read from the GPS
   (default is false)
    @return True if we got what we wanted, false otherwise
*/
/**************************************************************************/
bool L86::waitForSentence(const char *wait4me, uint8_t max,
                                   bool usingInterrupts) {
  uint8_t i = 0;
  while (i < max) {
    if (!usingInterrupts)
      read();

    if (newNMEAreceived()) {
      char *nmea = lastNMEA();
      i++;

      if (strStartsWith(nmea, wait4me))
        return true;
    }
  }

  return false;
}

/**************************************************************************/
/*!
    @brief Standby Mode Switches
    @return False if already in standby, true if it entered standby
*/
/**************************************************************************/
bool L86::standby(void) {
  if (inStandbyMode) {
    return false; // Returns false if already in standby mode, so that you do
                  // not wake it up by sending commands to GPS
  } else {
    inStandbyMode = true;
    sendCommand(PMTK_STANDBY);
    // return waitForSentence(PMTK_STANDBY_SUCCESS);  // don't seem to be fast
    // enough to catch the message, or something else just is not working
    return true;
  }
}

/**************************************************************************/
/*!
    @brief Wake the sensor up
    @return True if woken up, false if not in standby or failed to wake
*/
/**************************************************************************/
bool L86::wakeup(void) {
  if (inStandbyMode) {
    inStandbyMode = false;
    sendCommand(""); // send byte to wake it up
    return waitForSentence(PMTK_AWAKE);
  } else {
    return false; // Returns false if not in standby mode, nothing to wakeup
  }
}

/**************************************************************************/
/*!
    @brief Time in seconds since the last position fix was obtained. The
    time returned is limited to 2^32 milliseconds, which is about 49.7 days.
    It will wrap around to zero if no position fix is received
    for this long.
    @return nmea_float_t value in seconds since last fix.
*/
/**************************************************************************/
nmea_float_t L86::secondsSinceFix() {
  return (millis() - lastFix) / 1000.;
}

/**************************************************************************/
/*!
    @brief Time in seconds since the last GPS time was obtained. The time
    returned is limited to 2^32 milliseconds, which is about 49.7 days. It
    will wrap around to zero if no GPS time is received for this long.
    @return nmea_float_t value in seconds since last GPS time.
*/
/**************************************************************************/
nmea_float_t L86::secondsSinceTime() {
  return (millis() - lastTime) / 1000.;
}

/**************************************************************************/
/*!
    @brief Time in seconds since the last GPS date was obtained. The time
    returned is limited to 2^32 milliseconds, which is about 49.7 days. It
    will wrap around to zero if no GPS date is received for this long.
    @return nmea_float_t value in seconds since last GPS date.
*/
/**************************************************************************/
nmea_float_t L86::secondsSinceDate() {
  return (millis() - lastDate) / 1000.;
}

/**************************************************************************/
/*!
    @brief Fakes time of receipt of a sentence. Use between build() and parse()
    to make the timing look like the sentence arrived from the L86::
*/
/**************************************************************************/
void L86::resetSentTime() { sentTime = millis(); }

/**************************************************************************/
/*!
    @brief Checks whether a string starts with a specified prefix
    @param str Pointer to a string
    @param prefix Pointer to the prefix
    @return True if str starts with prefix, false otherwise
*/
/**************************************************************************/
static bool strStartsWith(const char *str, const char *prefix) {
  while (*prefix) {
    if (*prefix++ != *str++)
      return false;
  }
  return true;
}

/**************************************************************************/
/*!
  @file NMEA_parse.cpp

  @mainpage Adafruit Ultimate GPS Breakout

  @section intro Introduction

  This is the Adafruit GPS library - the ultimate GPS library
  for the ultimate GPS module!

  Tested and works great with the Adafruit Ultimate GPS module
  using MTK33x9 chipset
  ------> http://www.adafruit.com/products/746

  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  @section author Author

  Written by Limor Fried/Ladyada for Adafruit Industries.

  @section license License

  BSD license, check license.txt for more information
  All text above must be included in any redistribution
*/
/**************************************************************************/


/**************************************************************************/
/*!
    @brief Parse a standard NMEA string and update the relevant variables.
   Sentences start with a $, then a two character source identifier, then a
   three character sentence identifier that defines the format, then a comma and
   more comma separated fields defined by the sentence name. There are many
   sentences listed that are not yet supported, including proprietary sentences
   that start with P, like the $PMTK commands to the GPS modules. See the
   build() function and http://fort21.ru/download/NMEAdescription.pdf for
   sentence descriptions.

   Encapsulated data sentences are supported by NMEA-183, and start with !
   instead of $. https://gpsd.gitlab.io/gpsd/AIVDM.html provides details
   about encapsulated data sentences used in AIS.

    parse() permits, but does not require Carriage Return and Line Feed at the
   end of sentences. The end of the sentence is recognized by the * for the
   checksum. parse() will not recognize a sentence without a valid checksum.

   NMEA_EXTENSIONS must be defined in order to parse more than basic
   GPS module sentences.

    @param nmea Pointer to the NMEA string
    @return True if successfully parsed, false if fails check or parsing
*/
/**************************************************************************/
bool L86::parse(char *nmea) {
  if (!check(nmea))
    return false;
  // passed the check, so there's a valid source in thisSource and a valid
  // sentence in thisSentence
  char *p = nmea; // Pointer to move through the sentence -- good parsers are
                  // non-destructive
  p = strchr(p, ',') + 1; // Skip to char after the next comma, then check.

  // This may look inefficient, but an M0 will get down the list in about 1 us /
  // strcmp()! Put the GPS sentences from quectel_GPS at the top to make
  // pruning excess code easier. Otherwise, keep them alphabetical for ease of
  // reading.
  if (!strcmp(thisSentence, "GGA")) { //************************************GGA
    // Adafruit from Actisense NGW-1 from SH CP150C
    parseTime(p);
    p = strchr(p, ',') + 1; // parse time with specialized function
    // parse out both latitude and direction, then go to next field, or fail
    if (parseCoord(p, &latitudeDegrees, &latitude, &latitude_fixed, &lat))
    p = strchr(p, ',') + 1;
    p = strchr(p, ',') + 1;
    // parse out both longitude and direction, then go to next field, or fail
    if (parseCoord(p, &longitudeDegrees, &longitude, &longitude_fixed, &lon))
    p = strchr(p, ',') + 1;
    p = strchr(p, ',') + 1;
    if (!isEmpty(p)) { // if it's a , (or a * at end of sentence) the value is
                       // not included
      fixquality = atoi(p); // needs additional processing
      if (fixquality > 0) {
        fix = true;
        lastFix = sentTime;
      } else
        fix = false;
    }
    p = strchr(p, ',') + 1; // then move on to the next
    // Most can just be parsed with atoi() or atof(), then move on to the next.
    if (!isEmpty(p))
      satellites = atoi(p);
    p = strchr(p, ',') + 1;
    if (!isEmpty(p))
      HDOP = atof(p);
    p = strchr(p, ',') + 1;
    if (!isEmpty(p))
      altitude = atof(p);
    p = strchr(p, ',') + 1;
    p = strchr(p, ',') + 1; // skip the units
    if (!isEmpty(p))
      geoidheight = atof(p); // skip the rest

  } else if (!strcmp(thisSentence, "RMC")) { //*****************************RMC
    // in Adafruit from Actisense NGW-1 from SH CP150C
    parseTime(p);
    p = strchr(p, ',') + 1;
    parseFix(p);
    p = strchr(p, ',') + 1;
    // parse out both latitude and direction, then go to next field, or fail
    if (parseCoord(p, &latitudeDegrees, &latitude, &latitude_fixed, &lat))
    p = strchr(p, ',') + 1;
    p = strchr(p, ',') + 1;
    // parse out both longitude and direction, then go to next field, or fail
    if (parseCoord(p, &longitudeDegrees, &longitude, &longitude_fixed, &lon))
    p = strchr(p, ',') + 1;
    p = strchr(p, ',') + 1;
    if (!isEmpty(p))
    p = strchr(p, ',') + 1;
    if (!isEmpty(p))
    p = strchr(p, ',') + 1;
    if (!isEmpty(p)) {
      uint32_t fulldate = atof(p);
      day = fulldate / 10000;
      month = (fulldate % 10000) / 100;
      year = (fulldate % 100);
      lastDate = sentTime;
    } // skip the rest

  } else if (!strcmp(thisSentence, "GLL")) { //*****************************GLL
    // in Adafruit from Actisense NGW-1 from SH CP150C
    // parse out both latitude and direction, then go to next field, or fail
    if (parseCoord(p, &latitudeDegrees, &latitude, &latitude_fixed, &lat))
    p = strchr(p, ',') + 1;
    p = strchr(p, ',') + 1;
    // parse out both longitude and direction, then go to next field, or fail
    if (parseCoord(p, &longitudeDegrees, &longitude, &longitude_fixed, &lon))
    p = strchr(p, ',') + 1;
    p = strchr(p, ',') + 1;
    parseTime(p);
    p = strchr(p, ',') + 1;
    parseFix(p); // skip the rest

  } else if (!strcmp(thisSentence, "GSA")) { //*****************************GSA
    // in Adafruit from Actisense NGW-1
    p = strchr(p, ',') + 1; // skip selection mode
    if (!isEmpty(p))
      fixquality_3d = atoi(p);
    p = strchr(p, ',') + 1;
    // skip 12 Satellite PDNs without interpreting them
    for (int i = 0; i < 12; i++)
      p = strchr(p, ',') + 1;
    if (!isEmpty(p))
      PDOP = atof(p);
    p = strchr(p, ',') + 1;
    // parse out HDOP, we also parse this from the GGA sentence. Chipset should
    // report the same for both
    if (!isEmpty(p))
    p = strchr(p, ',') + 1;
    if (!isEmpty(p))
      VDOP = atof(p); // last before checksum

  }
  else {
    return false; // didn't find the required sentence definition
  }

  // Record the successful parsing of where the last data came from and when
  strcpy(lastSource, thisSource);
  strcpy(lastSentence, thisSentence);
  lastUpdate = millis();
  return true;
}

/**************************************************************************/
/*!
    @brief Check an NMEA string for basic format, valid source ID and valid
    and valid sentence ID. Update the values of thisCheck, thisSource and
    thisSentence.
    @param nmea Pointer to the NMEA string
    @return True if well formed, false if it has problems
*/
/**************************************************************************/
bool L86::check(char *nmea) {
  thisCheck = 0; // new check
  *thisSentence = *thisSource = 0;
  if (*nmea != '$' && *nmea != '!')
    return false; // doesn't start with $ or !
  else
    thisCheck += NMEA_HAS_DOLLAR;
  // do checksum check -- first look if we even have one -- ignore all but last
  // *
  char *ast = nmea; // not strchr(nmea,'*'); for first *
  while (*ast)
    ast++; // go to the end
  while (*ast != '*' && ast > nmea)
    ast--; // then back to * if it's there
  if (*ast != '*')
    return false; // there is no asterisk
  else {
    uint16_t sum = parseHex(*(ast + 1)) * 16; // extract checksum
    sum += parseHex(*(ast + 2));
    char *p = nmea; // check checksum
    for (char *p1 = p + 1; p1 < ast; p1++)
      sum ^= *p1;
    if (sum != 0)
      return false; // bad checksum :(
    else
      thisCheck += NMEA_HAS_CHECKSUM;
  }
  // extract source of variable length
  char *p = nmea + 1;
  const char *src = tokenOnList(p, sources);
  if (src) {
    strcpy(thisSource, src);
    thisCheck += NMEA_HAS_SOURCE;
  } else
    return false;
  p += strlen(src);
  // extract sentence id and check if parsed
  const char *snc = tokenOnList(p, sentences_parsed);
  if (snc) {
    strcpy(thisSentence, snc);
    thisCheck += NMEA_HAS_SENTENCE_P + NMEA_HAS_SENTENCE;
  } else { // check if known
    snc = tokenOnList(p, sentences_known);
    if (snc) {
      strcpy(thisSentence, snc);
      thisCheck += NMEA_HAS_SENTENCE;
      return false; // known but not parsed
    } else {
      parseStr(thisSentence, p, NMEA_MAX_SENTENCE_ID);
      return false; // unknown
    }
  }
  return true; // passed all the tests
}

/**************************************************************************/
/*!
    @brief Check if a token at the start of a string is on a list.
    @param token Pointer to the string
    @param list A list of strings, with the final entry starting "ZZ"
    @return Pointer to the found token, or NULL if it fails
*/
/**************************************************************************/
const char *L86::tokenOnList(char *token, const char **list) {
  int i = 0; // index in the list
  while (strncmp(list[i], "ZZ", 2) &&
         i < 1000) { // stop at terminator and don't crash without it
    // test for a match on the sentence name
    if (!strncmp((const char *)list[i], (const char *)token, strlen(list[i])))
      return list[i];
    i++;
  }
  return NULL; // couldn't find a match
}

/**************************************************************************/
/*!
    @brief Check if an NMEA string is valid and is on a list, perhaps to
    decide if it should be passed to a particular NMEA device.
    @param nmea Pointer to the NMEA string
    @param list A list of strings, with the final entry "ZZ"
    @return True if on the list, false if it fails check or is not on the list
*/
/**************************************************************************/
bool L86::onList(char *nmea, const char **list) {
  if (!check(nmea)) // sets thisSentence if valid
    return false;   // not a valid sentence
  // stop at terminator with first two letters ZZ and don't crash without it
  for (int i = 0; strncmp(list[i], "ZZ", 2) && i < 1000; i++) {
    // test for a match on the sentence name
    if (!strcmp((const char *)list[i], (const char *)thisSentence))
      return true;
  }
  return false; // couldn't find a match
}

/**************************************************************************/
/*!
    @brief Parse a part of an NMEA string for lat or lon angle and direction.
    Works for either DDMM.mmmm,N (latitude) or DDDMM.mmmm,W (longitude) format.
    Insensitive to number of decimal places present. Only fills the variables
    if it succeeds and the variable pointer is not NULL. This allows calling
    to fill only the variables of interest. Does rudimentary validation on
    angle range.

    Supersedes private functions parseLat(), parseLon(), parseLatDir(),
    parseLonDir(), all previously called from parse().
    @param pStart Pointer to the location of the token in the NMEA string
    @param angle Pointer to the angle to fill with value in degrees/minutes as
      received from the GPS (DDDMM.MMMM), unsigned
    @param angle_fixed Pointer to the fix point version latitude in decimal
      degrees * 10000000, signed
    @param angleDegrees Pointer to the angle to fill with decimal degrees,
      signed. As actual double on SAMD, etc. resolution is better than the
      fixed point version.
    @param dir Pointer to character to fill the direction N/S/E/W
    @return true if successful, false if failed or no value
*/
/**************************************************************************/
bool L86::parseCoord(char *pStart, nmea_float_t *angleDegrees,
                              nmea_float_t *angle, int32_t *angle_fixed,
                              char *dir) {
  char *p = pStart;
  if (!isEmpty(p)) {
    // get the number in DDDMM.mmmm format and break into components
    char degreebuff[10] = {0}; // Ensure string is terminated after strncpy
    char *e = strchr(p, '.');
    if (e == NULL || e - p > 6)
      return false;                // no decimal point in range
    strncpy(degreebuff, p, e - p); // get DDDMM
    long dddmm = atol(degreebuff);
    long degrees = (dddmm / 100);         // truncate the minutes
    long minutes = dddmm - degrees * 100; // remove the degrees
    p = e;                                // start from the decimal point
    nmea_float_t decminutes = atof(e); // the fraction after the decimal point
    p = strchr(p, ',') + 1;            // go to the next field

    // get the NSEW direction as a character
    char nsew = 'X';
    if (!isEmpty(p))
      nsew = *p; // field is not empty
    else
      return false; // no direction provided

    // set the various numerical formats to their values
    long fixed = degrees * 10000000 + (minutes * 10000000) / 60 +
                 (decminutes * 10000000) / 60;
    nmea_float_t ang = degrees * 100 + minutes + decminutes;
    nmea_float_t deg = fixed / (nmea_float_t)10000000.;
    if (nsew == 'S' ||
        nsew == 'W') { // fixed and deg are signed, but DDDMM.mmmm is not
      fixed = -fixed;
      deg = -deg;
    }

    // reject directions that are not NSEW
    if (nsew != 'N' && nsew != 'S' && nsew != 'E' && nsew != 'W')
      return false;

    // reject angles that are out of range
    if (nsew == 'N' || nsew == 'S')
      if (abs(deg) > 90)
        return false;
    if (abs(deg) > 180)
      return false;

    // store in locations passed as args
    if (angle != NULL)
      *angle = ang;
    if (angle_fixed != NULL)
      *angle_fixed = fixed;
    if (angleDegrees != NULL)
      *angleDegrees = deg;
    if (dir != NULL)
      *dir = nsew;
  } else
    return false; // no number
  return true;
}

/**************************************************************************/
/*!
    @brief Parse a string token from pointer p to the next comma, asterisk
    or end of string.
    @param buff Pointer to the buffer to store the string in
    @param p Pointer into a string
    @param n Max permitted size of string including terminating 0
    @return Pointer to the string buffer
*/
/**************************************************************************/
char *L86::parseStr(char *buff, char *p, int n) {
  char *e = strchr(p, ',');
  int len = 0;
  if (e) {
    len = min(e - p, n - 1);
    strncpy(buff, p, len); // copy up to the comma
    buff[len] = 0;
  } else {
    e = strchr(p, '*');
    if (e) {
      len = min(e - p, n - 1);
      strncpy(buff, p, len); // or up to the *
      buff[e - p] = 0;
    } else {
      len = min((int)strlen(p), n - 1);
      strncpy(buff, p, len); // or to the end or max capacity
    }
  }
  return buff;
}

/**************************************************************************/
/*!
    @brief Parse a part of an NMEA string for time. Independent of number
    of decimal places after the '.'
    @param p Pointer to the location of the token in the NMEA string
    @return true if successful, false otherwise
*/
/**************************************************************************/
bool L86::parseTime(char *p) {
  if (!isEmpty(p)) { // get time
    uint32_t time = atol(p);
    hour = time / 10000;
    minute = (time % 10000) / 100;
    seconds = (time % 100);
    char *dec = strchr(p, '.');
    char *comstar = min(strchr(p, ','), strchr(p, '*'));
    if (dec != NULL && comstar != NULL && dec < comstar)
      milliseconds = atof(dec) * 1000;
    else
      milliseconds = 0;
    lastTime = sentTime;
    return true;
  }
  return false;
}

/**************************************************************************/
/*!
    @brief Parse a part of an NMEA string for whether there is a fix
    @param p Pointer to the location of the token in the NMEA string
    @return True if we parsed it, false if it has invalid data
*/
/**************************************************************************/
bool L86::parseFix(char *p) {
  if (!isEmpty(p)) {
    if (p[0] == 'A') {
      fix = true;
      lastFix = sentTime;
    } else if (p[0] == 'V')
      fix = false;
    else
      return false;
    return true;
  }
  return false;
}

/**************************************************************************/
/*!
    @brief Is the field empty, or should we try conversion? Won't work
    for a text field that starts with an asterisk or a comma, but that
    probably violates the NMEA-183 standard.
    @param pStart Pointer to the location of the token in the NMEA string
    @return true if empty field, false if something there
*/
/**************************************************************************/
bool L86::isEmpty(char *pStart) {
  if (',' != *pStart && '*' != *pStart && pStart != NULL)
    return false;
  else
    return true;
}

/**************************************************************************/
/*!
    @brief Parse a hex character and return the appropriate decimal value
    @param c Hex character, e.g. '0' or 'B'
    @return Integer value of the hex character. Returns 0 if c is not a proper
   character
*/
/**************************************************************************/
// read a Hex value and return the decimal equivalent
uint8_t L86::parseHex(char c) {
  if (c < '0')
    return 0;
  if (c <= '9')
    return c - '0';
  if (c < 'A')
    return 0;
  if (c <= 'F')
    return (c - 'A') + 10;
  // if (c > 'F')
  return 0;
}

/**************************************************************************/
/*!
    @brief Add *CS where CS is the two character hex checksum for all but
    the first character in the string. The checksum is the result of an
    exclusive or of all the characters in the string. Also useful if you
    are creating new PMTK strings for controlling a GPS module and need a
    checksum added.
    @param buff Pointer to the string, which must be long enough
    @return none
*/
/**************************************************************************/
void L86::addChecksum(char *buff) {
  char cs = 0;
  int i = 1;
  while (buff[i]) {
    cs ^= buff[i];
    i++;
  }
  sprintf(buff, "%s*%02X", buff, cs);
}

