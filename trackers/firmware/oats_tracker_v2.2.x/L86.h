/*
Author: Glen Charlton
Last Modified: 2024/03/27
Purpose: For OATS v2 boards L86 GNSS module Declarations
 
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

This file is a modified version of the Adafruit GPS library written by 
Limor Fried/Ladyada for Adafruit Industries. See original license below:

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

#ifndef _GPS_L86_h
#define _GPS_L86_h

#include <HardwareSerial.h>
#include <string.h>
#include <arduino.h>
#include <stdio.h>
#include <time.h>
#include "esp_sleep.h"
#include "esp32_time.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "driver/rtc_io.h"
#include "datatypes.h"
#include "network.h"

#include <SPI.h>
#include <Wire.h>
;

/* Definitions of GPS Variables */
#define MAXLINELENGTH 120
#define NMEA_MAX_SENTENCE_ID 20
#define NMEA_MAX_SOURCE_ID 3    
#define NMEA_MAX_SENTENCE_ID 20 
#define NMEA_MAX_SOURCE_ID 3 
#define MAXWAITSENTENCE 10

#ifndef NMEA_FLOAT_T
#define NMEA_FLOAT_T float // let float be overidden on command line
#endif
typedef NMEA_FLOAT_T nmea_float_t; // the type of variables to use for floating point

/* PMTK Commands for L86 */
#define PMTK_SET_NMEA_UPDATE_1HZ "$PMTK220,1000*1F"                                     //  1 Hz
#define PMTK_API_SET_FIX_CTL_1HZ "$PMTK300,1000,0,0,0,0*1C"                             // 1 Hz
#define PMTK_SET_BAUD_9600 "$PMTK251,9600*17"                                           //   9600 bps
#define PMTK_SET_NMEA_OUTPUT_OFF "$PMTK314,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*28"    // turn off output
#define PMTK_SET_NMEA_OUTPUT_RMCGGA "$PMTK314,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*28" // turn on GPRMC and // GPGGA
#define PMTK_STANDBY "$PMTK161,0*28"                                                    // standby command & boot successful message
#define PMTK_STANDBY_SUCCESS "$PMTK001,161,3*36"                                        // Not needed currently
#define PMTK_AWAKE "$PMTK010,002*2D"                                                    // Wake up
#define PMTK_Q_RELEASE "$PMTK605*31"                                                    // ask for the release and version
#define PMTK_PPS_WAITFORFIX "$PMTK285,3,100*3E"                                         //Set PPS to start once 2D/3D Fix available

/* Data Type for running NMEA message validation */
typedef enum
{
  NMEA_BAD = 0,             // passed none of the checks
  NMEA_HAS_DOLLAR = 1,      // has a dollar sign or exclamation mark in the first position
  NMEA_HAS_CHECKSUM = 2,    // has a valid checksum at the end
  NMEA_HAS_NAME = 4,        // there is a token after the $ followed by a comma
  NMEA_HAS_SOURCE = 10,     // has a recognized source ID
  NMEA_HAS_SENTENCE = 20,   // has a recognized sentence ID
  NMEA_HAS_SENTENCE_P = 40  // has a recognized parseable sentence ID
} nmea_check_t;

/* Initialise variables in L86.cpp */
extern int gps_interval_max;
extern int gps_interval_timer;

/* Create L86 Class */
class L86
{
public:

  /* Basic GPS Module Control */
  bool setup_pins();
  bool setup_serial();
  bool init();
  void get();
  location_struct log(int samples);
  void shutdown();
  void startup();
  void write(int write_time);
  void initTime();
  void setTime();

  /* Collection of Variables */
  int32_t time_unix();
  float location_lat();
  float location_lon();
  float location_alt();
  float location_spd();
  float stats_sats();
  float stats_hdop();
  float stats_vdop();
  float stats_pdop();
  float calc_distance(float lat1, float lon1, float lat2, float lon2);
  float calc_bearing(float lat1, float lon1, float lat2, float lon2);

  /* Fix Quality */
  uint8_t stats_fixquality();
  uint8_t stats_fixquality_3d();

  /* L86 Communication */
  bool begin(uint32_t baud);
  L86(); //initalising
  bool available(void);
  size_t write(uint8_t);
  char read(void);
  void sendCommand(const char *);
  bool newNMEAreceived();
  void pause(bool b);
  char *lastNMEA(void);
  bool waitForSentence(const char *wait, uint8_t max = 10, bool usingInterrupts = false); // fix this
  bool standby(void);
  bool wakeup(void);
  nmea_float_t secondsSinceFix();
  nmea_float_t secondsSinceTime();
  nmea_float_t secondsSinceDate();
  void resetSentTime();
  static bool strStartsWith(const char *str, const char *prefix);

  /* NMEA Parsing */
  bool parse(char *);
  bool check(char *nmea);
  bool onList(char *nmea, const char **list);
  uint8_t parseHex(char c);

  void addChecksum(char *buff);

  /* NMEA Data Types */
  int thisCheck = 0;                             // the results of the check on the current sentence
  char thisSource[NMEA_MAX_SOURCE_ID] = {0};     // the first two letters of the current sentence, e.g. WI, GP
  char thisSentence[NMEA_MAX_SENTENCE_ID] = {0}; // the next three letters of the current sentence, e.g. GLL, RMC
  char lastSource[NMEA_MAX_SOURCE_ID] = {0};     // the results of the check on the most recent successfully parsed sentence
  char lastSentence[NMEA_MAX_SENTENCE_ID] = {0}; // the next three letters of the most recent successfully parsed sentence, e.g. GLL, RMC

  uint8_t hour;          // GMT hours
  uint8_t minute;        // GMT minutes
  uint8_t seconds;       // GMT seconds
  uint16_t milliseconds; // GMT milliseconds
  uint8_t year;          // GMT year
  uint8_t month;         // GMT month
  uint8_t day;           // GMT day

  nmea_float_t latitude;  // Floating point latitude value in degrees/minutes
                          // as received from the GPS (DDMM.MMMM)
  nmea_float_t longitude; // Floating point longitude value in degrees/minutes
                          // as received from the GPS (DDDMM.MMMM)

  /** Fixed point latitude and longitude value with degrees stored in units of
    1/10000000 of a degree. See pull #13 for more details:
    https://github.com/adafruit/Adafruit-GPS-Library/pull/13 */
  int32_t latitude_fixed;  // Fixed point latitude in decimal degrees.
                           // Divide by 10000000.0 to get a double.
  int32_t longitude_fixed; // Fixed point longitude in decimal degrees
                           // Divide by 10000000.0 to get a double.

  nmea_float_t latitudeDegrees;  // Latitude in decimal degrees
  nmea_float_t longitudeDegrees; // Longitude in decimal degrees
  nmea_float_t geoidheight;      // Diff between geoid height and WGS84 height
  nmea_float_t altitude;         // Altitude in meters above MSL
  nmea_float_t speed;            // Current speed over ground in knots
  nmea_float_t angle;            // Course in degrees from true north
  nmea_float_t magvariation;     // Magnetic variation in degrees (vs. true north)
  nmea_float_t HDOP;             // Horizontal Dilution of Precision - relative accuracy
                                 // of horizontal position
  nmea_float_t VDOP;             // Vertical Dilution of Precision - relative accuracy
                                 // of vertical position
  nmea_float_t PDOP;             // Position Dilution of Precision - Complex maths derives
                                 // a simple, single number for each kind of DOP
  char lat = 'X';                // N/S
  char lon = 'X';                // E/W
  char mag = 'X';                // Magnetic variation direction
  bool fix;                      // Have a fix?
  uint8_t fixquality;            // Fix quality (0, 1, 2 = Invalid, GPS, DGPS)
  uint8_t fixquality_3d;         // 3D fix quality (1, 2, 3 = Nofix, 2D fix, 3D fix)
  uint8_t satellites;            // Number of satellites in use

typedef struct {
  int16_t *data = NULL;          // array of ints, oldest first
  unsigned n = 0;                // number of history array elements
  uint32_t lastHistory = 0;      // millis() when history was last updated
  uint16_t historyInterval = 20; // seconds between history updates
  nmea_float_t scale = 1.0;      // history = (smoothed - offset) * scale
  nmea_float_t offset = 0.0;     // value = (float) history / scale + offset
} nmea_history_t;

private:

  // NMEA_parse.cpp
  const char *tokenOnList(char *token, const char **list);
  bool parseCoord(char *p, nmea_float_t *angleDegrees = NULL,
                  nmea_float_t *angle = NULL, int32_t *angle_fixed = NULL,
                  char *dir = NULL);
  char *parseStr(char *buff, char *p, int n);
  bool parseTime(char *);
  bool parseFix(char *);
  bool isEmpty(char *pStart);

  const char *sources[6] = {"II", "WI", "GP", "GN", "P", "ZZZ"}; // valid source ids

  const char *sentences_parsed[5] = {"GGA", "GLL", "GSA", "RMC", "ZZZ"}; // parseable sentence ids
  const char *sentences_known[4] = {"DBT", "HDM", "HDT", "ZZZ"};         // known, but not parseable

  // Make all of these times far in the past by setting them near the middle of
  // the millis() range. Timing assumes that sentences are parsed promptly.
  uint32_t lastUpdate =
      2000000000L;                 // millis() when last full sentence successfully parsed
  uint32_t lastFix = 2000000000L;  // millis() when last fix received
  uint32_t lastTime = 2000000000L; // millis() when last time received
  uint32_t lastDate = 2000000000L; // millis() when last date received
  uint32_t recvdTime =
      2000000000L;                 // millis() when last full sentence received
  uint32_t sentTime = 2000000000L; // millis() when first character of last
                                   // full sentence received
  bool paused;

  uint8_t parseResponse(char *response);

  bool noComms = false;
  int8_t _buff_max = -1, _buff_idx = 0;
  char last_char = 0;

  volatile char line1[MAXLINELENGTH]; // We double buffer: read one line in
                                      // and leave one for the main program
  volatile char line2[MAXLINELENGTH]; // Second buffer
  volatile uint8_t lineidx = 0;       // our index into filling the current line
  volatile char *currentline;         // Pointer to current line buffer
  volatile char *lastline;            // Pointer to previous line buffer
  volatile bool recvdflag;            // Received flag
  volatile bool inStandbyMode;        // In standby flag
};

#endif