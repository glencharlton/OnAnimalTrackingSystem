/*
Author: Glen Charlton
Last Modified: 2024/03/27
Purpose: For OATS v2 boards ADXL343 module Declarations
 
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
#ifndef _ADXL343_h
#define _ADXL343_h

//#include <Wire.h>
//#include <Adafruit_Sensor.h>
//#include <Adafruit_ADXL343.h>
//#include "sleep.h"

#include <Wire.h>
#include <Arduino.h>
#include "datatypes.h"
#include "config.h"
#include "network.h"

#define GRAVITY (9.80665F)
#define ADXL343_MG2G_MULTIPLIER (0.031) /**< 4mg per lsb */
#define ADXL343_MG2MS_MULTIPLIER (0.3041) /**< 31mg per lsb * gravity */

#define ADXL343_ADDRESS_ALT_LOW     0x53 // alt address pin low (GND)
#define ADXL343_ADDRESS_ALT_HIGH    0x1D // alt address pin high (VCC)
#define ADXL343_DEFAULT_ADDRESS     ADXL343_ADDRESS_ALT_LOW

#define ADXL343_RA_DEVID            0x00
#define ADXL343_RA_RESERVED1        0x01
#define ADXL343_RA_THRESH_TAP       0x1D
#define ADXL343_RA_OFSX             0x1E
#define ADXL343_RA_OFSY             0x1F
#define ADXL343_RA_OFSZ             0x20
#define ADXL343_RA_DUR              0x21
#define ADXL343_RA_LATENT           0x22
#define ADXL343_RA_WINDOW           0x23
#define ADXL343_RA_THRESH_ACT       0x24
#define ADXL343_RA_THRESH_INACT     0x25
#define ADXL343_RA_TIME_INACT       0x26
#define ADXL343_RA_ACT_INACT_CTL    0x27
#define ADXL343_RA_THRESH_FF        0x28
#define ADXL343_RA_TIME_FF          0x29
#define ADXL343_RA_TAP_AXES         0x2A
#define ADXL343_RA_ACT_TAP_STATUS   0x2B
#define ADXL343_RA_BW_RATE          0x2C
#define ADXL343_RA_POWER_CTL        0x2D
#define ADXL343_RA_INT_ENABLE       0x2E
#define ADXL343_RA_INT_MAP          0x2F
#define ADXL343_RA_INT_SOURCE       0x30
#define ADXL343_RA_DATA_FORMAT      0x31
#define ADXL343_RA_DATAX0           0x32
#define ADXL343_RA_DATAX1           0x33
#define ADXL343_RA_DATAY0           0x34
#define ADXL343_RA_DATAY1           0x35
#define ADXL343_RA_DATAZ0           0x36
#define ADXL343_RA_DATAZ1           0x37
#define ADXL343_RA_FIFO_CTL         0x38
#define ADXL343_RA_FIFO_STATUS      0x39

#define ADXL343_AIC_ACT_AC_BIT      7
#define ADXL343_AIC_ACT_X_BIT       6
#define ADXL343_AIC_ACT_Y_BIT       5
#define ADXL343_AIC_ACT_Z_BIT       4
#define ADXL343_AIC_INACT_AC_BIT    3
#define ADXL343_AIC_INACT_X_BIT     2
#define ADXL343_AIC_INACT_Y_BIT     1
#define ADXL343_AIC_INACT_Z_BIT     0

#define ADXL343_TAPAXIS_SUP_BIT     3
#define ADXL343_TAPAXIS_X_BIT       2
#define ADXL343_TAPAXIS_Y_BIT       1
#define ADXL343_TAPAXIS_Z_BIT       0

#define ADXL343_TAPSTAT_ACTX_BIT    6
#define ADXL343_TAPSTAT_ACTY_BIT    5
#define ADXL343_TAPSTAT_ACTZ_BIT    4
#define ADXL343_TAPSTAT_ASLEEP_BIT  3
#define ADXL343_TAPSTAT_TAPX_BIT    2
#define ADXL343_TAPSTAT_TAPY_BIT    1
#define ADXL343_TAPSTAT_TAPZ_BIT    0

#define ADXL343_BW_LOWPOWER_BIT     4
#define ADXL343_BW_RATE_BIT         3
#define ADXL343_BW_RATE_LENGTH      4

#define ADXL343_RATE_3200           0b1111
#define ADXL343_RATE_1600           0b1110
#define ADXL343_RATE_800            0b1101
#define ADXL343_RATE_400            0b1100
#define ADXL343_RATE_200            0b1011
#define ADXL343_RATE_100            0b1010
#define ADXL343_RATE_50             0b1001
#define ADXL343_RATE_25             0b1000
#define ADXL343_RATE_12P5           0b0111
#define ADXL343_RATE_6P25           0b0110
#define ADXL343_RATE_3P13           0b0101
#define ADXL343_RATE_1P56           0b0100
#define ADXL343_RATE_0P78           0b0011
#define ADXL343_RATE_0P39           0b0010
#define ADXL343_RATE_0P20           0b0001
#define ADXL343_RATE_0P10           0b0000

#define ADXL343_PCTL_LINK_BIT       5
#define ADXL343_PCTL_AUTOSLEEP_BIT  4
#define ADXL343_PCTL_MEASURE_BIT    3
#define ADXL343_PCTL_SLEEP_BIT      2
#define ADXL343_PCTL_WAKEUP_BIT     1
#define ADXL343_PCTL_WAKEUP_LENGTH  2

#define ADXL343_WAKEUP_8HZ          0b00
#define ADXL343_WAKEUP_4HZ          0b01
#define ADXL343_WAKEUP_2HZ          0b10
#define ADXL343_WAKEUP_1HZ          0b11

#define ADXL343_INT_DATA_READY_BIT  7
#define ADXL343_INT_SINGLE_TAP_BIT  6
#define ADXL343_INT_DOUBLE_TAP_BIT  5
#define ADXL343_INT_ACTIVITY_BIT    4
#define ADXL343_INT_INACTIVITY_BIT  3
#define ADXL343_INT_FREE_FALL_BIT   2
#define ADXL343_INT_WATERMARK_BIT   1
#define ADXL343_INT_OVERRUN_BIT     0

#define ADXL343_FORMAT_SELFTEST_BIT 7
#define ADXL343_FORMAT_SPIMODE_BIT  6
#define ADXL343_FORMAT_INTMODE_BIT  5
#define ADXL343_FORMAT_FULL_RES_BIT 3
#define ADXL343_FORMAT_JUSTIFY_BIT  2
#define ADXL343_FORMAT_RANGE_BIT    1
#define ADXL343_FORMAT_RANGE_LENGTH 2

#define ADXL343_RANGE_2G            0b00
#define ADXL343_RANGE_4G            0b01
#define ADXL343_RANGE_8G            0b10
#define ADXL343_RANGE_16G           0b11

#define ADXL343_FIFO_MODE_BIT       7
#define ADXL343_FIFO_MODE_LENGTH    2
#define ADXL343_FIFO_TRIGGER_BIT    5
#define ADXL343_FIFO_SAMPLES_BIT    4
#define ADXL343_FIFO_SAMPLES_LENGTH 5

#define ADXL343_FIFO_MODE_BYPASS    0b00
#define ADXL343_FIFO_MODE_FIFO      0b01
#define ADXL343_FIFO_MODE_STREAM    0b10
#define ADXL343_FIFO_MODE_TRIGGER   0b11

#define ADXL343_FIFOSTAT_TRIGGER_BIT        7
#define ADXL343_FIFOSTAT_LENGTH_BIT         5
#define ADXL343_FIFOSTAT_LENGTH_LENGTH      6

class ADXL343 {
public:
    ADXL343();
    ADXL343(uint8_t address);

    bool setConfig();
    bool setReconfig();
    void getFifoData(int samples);
    features_struct getFeatures(int samples);
    accel_struct getAccelData(int32_t ts, int16_t ms);

    void initialize();
    bool testConnection();

    // DEVID register
    uint8_t getDeviceID();

    // THRESH_TAP register
    uint8_t getTapThreshold();
    void setTapThreshold(uint8_t threshold);

    // OFS* registers
    void getOffset(int8_t* x, int8_t* y, int8_t* z);
    void setOffset(int8_t x, int8_t y, int8_t z);
    int8_t getOffsetX();
    void setOffsetX(int8_t x);
    int8_t getOffsetY();
    void setOffsetY(int8_t y);
    int8_t getOffsetZ();
    void setOffsetZ(int8_t z);

    // DUR register
    uint8_t getTapDuration();
    void setTapDuration(uint8_t duration);

    // LATENT register
    uint8_t getDoubleTapLatency();
    void setDoubleTapLatency(uint8_t latency);

    // WINDOW register
    uint8_t getDoubleTapWindow();
    void setDoubleTapWindow(uint8_t window);

    // THRESH_ACT register
    uint8_t getActivityThreshold();
    void setActivityThreshold(uint8_t threshold);

    // THRESH_INACT register
    uint8_t getInactivityThreshold();
    void setInactivityThreshold(uint8_t threshold);

    // TIME_INACT register
    uint8_t getInactivityTime();
    void setInactivityTime(uint8_t time);

    // ACT_INACT_CTL register
    bool getActivityAC();
    void setActivityAC(bool enabled);
    bool getActivityXEnabled();
    void setActivityXEnabled(bool enabled);
    bool getActivityYEnabled();
    void setActivityYEnabled(bool enabled);
    bool getActivityZEnabled();
    void setActivityZEnabled(bool enabled);
    bool getInactivityAC();
    void setInactivityAC(bool enabled);
    bool getInactivityXEnabled();
    void setInactivityXEnabled(bool enabled);
    bool getInactivityYEnabled();
    void setInactivityYEnabled(bool enabled);
    bool getInactivityZEnabled();
    void setInactivityZEnabled(bool enabled);

    // THRESH_FF register
    uint8_t getFreefallThreshold();
    void setFreefallThreshold(uint8_t threshold);

    // TIME_FF register
    uint8_t getFreefallTime();
    void setFreefallTime(uint8_t time);

    // TAP_AXES register
    bool getTapAxisSuppress();
    void setTapAxisSuppress(bool enabled);
    bool getTapAxisXEnabled();
    void setTapAxisXEnabled(bool enabled);
    bool getTapAxisYEnabled();
    void setTapAxisYEnabled(bool enabled);
    bool getTapAxisZEnabled();
    void setTapAxisZEnabled(bool enabled);

    // ACT_TAP_STATUS register
    bool getActivitySourceX();
    bool getActivitySourceY();
    bool getActivitySourceZ();
    bool getAsleep();
    bool getTapSourceX();
    bool getTapSourceY();
    bool getTapSourceZ();

    // BW_RATE register
    bool getLowPowerEnabled();
    void setLowPowerEnabled(bool enabled);
    uint8_t getRate();
    void setRate(uint8_t rate);

    // POWER_CTL register
    bool getLinkEnabled();
    void setLinkEnabled(bool enabled);
    bool getAutoSleepEnabled();
    void setAutoSleepEnabled(bool enabled);
    bool getMeasureEnabled();
    void setMeasureEnabled(bool enabled);
    bool getSleepEnabled();
    void setSleepEnabled(bool enabled);
    uint8_t getWakeupFrequency();
    void setWakeupFrequency(uint8_t frequency);

    // INT_ENABLE register
    bool getIntDataReadyEnabled();
    void setIntDataReadyEnabled(bool enabled);
    bool getIntSingleTapEnabled();
    void setIntSingleTapEnabled(bool enabled);
    bool getIntDoubleTapEnabled();
    void setIntDoubleTapEnabled(bool enabled);
    bool getIntActivityEnabled();
    void setIntActivityEnabled(bool enabled);
    bool getIntInactivityEnabled();
    void setIntInactivityEnabled(bool enabled);
    bool getIntFreefallEnabled();
    void setIntFreefallEnabled(bool enabled);
    bool getIntWatermarkEnabled();
    void setIntWatermarkEnabled(bool enabled);
    bool getIntOverrunEnabled();
    void setIntOverrunEnabled(bool enabled);

    // INT_MAP register
    uint8_t getIntDataReadyPin();
    void setIntDataReadyPin(uint8_t pin);
    uint8_t getIntSingleTapPin();
    void setIntSingleTapPin(uint8_t pin);
    uint8_t getIntDoubleTapPin();
    void setIntDoubleTapPin(uint8_t pin);
    uint8_t getIntActivityPin();
    void setIntActivityPin(uint8_t pin);
    uint8_t getIntInactivityPin();
    void setIntInactivityPin(uint8_t pin);
    uint8_t getIntFreefallPin();
    void setIntFreefallPin(uint8_t pin);
    uint8_t getIntWatermarkPin();
    void setIntWatermarkPin(uint8_t pin);
    uint8_t getIntOverrunPin();
    void setIntOverrunPin(uint8_t pin);

    // INT_SOURCE register
    uint8_t getIntDataReadySource();
    uint8_t getIntSingleTapSource();
    uint8_t getIntDoubleTapSource();
    uint8_t getIntActivitySource();
    uint8_t getIntInactivitySource();
    uint8_t getIntFreefallSource();
    uint8_t getIntWatermarkSource();
    uint8_t getIntOverrunSource();

    // DATA_FORMAT register
    uint8_t getSelfTestEnabled();
    void setSelfTestEnabled(uint8_t enabled);
    uint8_t getSPIMode();
    void setSPIMode(uint8_t mode);
    uint8_t getInterruptMode();
    void setInterruptMode(uint8_t mode);
    uint8_t getFullResolution();
    void setFullResolution(uint8_t resolution);
    uint8_t getDataJustification();
    void setDataJustification(uint8_t justification);
    uint8_t getRange();
    void setRange(uint8_t range);

    // DATA* registers
    void getAcceleration(int16_t* x, int16_t* y, int16_t* z);
    int16_t getAccelerationX();
    int16_t getAccelerationY();
    int16_t getAccelerationZ();

    // FIFO_CTL register
    uint8_t getFIFOMode();
    void setFIFOMode(uint8_t mode);
    uint8_t getFIFOTriggerInterruptPin();
    void setFIFOTriggerInterruptPin(uint8_t interrupt);
    uint8_t getFIFOSamples();
    void setFIFOSamples(uint8_t size);

    // FIFO_STATUS register
    bool getFIFOTriggerOccurred();
    uint8_t getFIFOLength();

private:
    uint8_t devAddr;
    uint8_t buffer[6];
};

class I2Cdev {
public:
    I2Cdev();

    static int8_t readBit(uint8_t devAddr, uint8_t regAddr, uint8_t bitNum, uint8_t* data);
    static int8_t readBits(uint8_t devAddr, uint8_t regAddr, uint8_t bitStart, uint8_t length, uint8_t* data);
    static int8_t readByte(uint8_t devAddr, uint8_t regAddr, uint8_t* data);
    static int8_t readBytes(uint8_t devAddr, uint8_t regAddr, uint8_t length, uint8_t* data);
    static bool writeBit(uint8_t devAddr, uint8_t regAddr, uint8_t bitNum, uint8_t data);
    static bool writeBits(uint8_t devAddr, uint8_t regAddr, uint8_t bitStart, uint8_t length, uint8_t data);
    static bool writeByte(uint8_t devAddr, uint8_t regAddr, uint8_t data);
    static bool writeBytes(uint8_t devAddr, uint8_t regAddr, uint8_t length, uint8_t* data);
};


#endif

