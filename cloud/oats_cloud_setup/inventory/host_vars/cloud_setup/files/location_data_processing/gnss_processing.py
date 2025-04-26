###############################################################################
# Author: Glen Charlton
# Last Modified: 2024/03/27
# Purpose: For processing the GNSS data

# Software License Agreement (BSD License)

# Copyright (c) 2024, Glen Charlton
# All rights reserved.

# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# 1. Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution.
# 3. Neither the name of the copyright holders nor the
# names of its contributors may be used to endorse or promote products
# derived from this software without specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ''AS IS'' AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
###############################################################################


from pykalman import KalmanFilter
import numpy as np
import numpy.ma as ma
import pandas as pd
import time
import datetime
import pytz
from scipy import stats
import math
import json
import psql
from psql_config import database, host, user, password
from joblib import Parallel, delayed

# ignore warnings
import os

os.environ["PYTHONWARNINGS"] = "ignore"  # ignore warnings
pd.options.mode.chained_assignment = None  # default='warn'

input_table = 'sensors.data'
output_table = 'sensors.location'
device_table = 'sensors.devices'


##### Custom Functions #####
def get_bearing(lat1, lon1, lat2, lon2):
    dLon = (lon2 - lon1)
    x = math.cos(math.radians(lat2)) * math.sin(math.radians(dLon))
    y = math.cos(math.radians(lat1)) * math.sin(math.radians(lat2)) - math.sin(math.radians(lat1)) * math.cos(
        math.radians(lat2)) * math.cos(math.radians(dLon))
    bearing = np.arctan2(x, y)
    bearing = np.degrees(bearing)
    return bearing


def get_distance(lon1, lat1, lon2, lat2):
    # convert decimal degrees to radians
    lon1, lat1, lon2, lat2 = map(math.radians, [lon1, lat1, lon2, lat2])
    # haversine formula
    dlon = lon2 - lon1
    dlat = lat2 - lat1
    a = math.sin(dlat / 2) ** 2 + math.cos(lat1) * math.cos(lat2) * math.sin(dlon / 2) ** 2
    c = 2 * math.asin(math.sqrt(a))
    r = 6371000  # in metres
    return c * r


# Define a function to parse the JSON data
def parse_json_data(json_data):
    data = json.loads(json_data)
    return pd.Series(data)


# Function to get devices that need processing
def get_devices():
    ##### Determine devices and start times #####
    query = (""" SELECT device_id, min(timestamp) """ +
             """FROM sensors.data """ +
             """ WHERE created_at > now() - (interval '1 hour' + interval '5 minute') """ +
             """ AND data_type = 'location' and device_id like 'oats_%' """ +
             """ GROUP BY device_id; """)
    con = psql.connect(database, host, user, password)
    devices = psql.query(connection=con, query=query)
    psql.disconnect(con)

    return devices


def gnss_processing(device):
    try:
        print(datetime.datetime.now(), ", Starting Processing Device, ", device)
        device_id = device

        con = psql.connect(database, host, user, password)
        query = (""" SELECT max(timestamp) FROM """ + output_table + """ WHERE device_id = '""" + device_id + """'""")
        startDate = psql.query(connection=con, query=query).iloc[0, 0]
        psql.disconnect(con)

        if startDate is None:
            con = psql.connect(database, host, user, password)
            query = (""" SELECT max(date_start) FROM """ + device_table + """ WHERE device_id = '""" + device_id + """'""")
            startDate = psql.query(connection=con, query=query).iloc[0, 0]
            psql.disconnect(con)
        else:
            startDate = startDate.replace(hour=0, minute=0, second=0, microsecond=0)
            startDate = startDate - datetime.timedelta(days=14)

        ##### Get data #####
        # query data
        con = psql.connect(database, host, user, password)
        query = (""" SELECT * FROM """ + input_table + """ WHERE data_type = 'location' """ +
                 """ AND device_id = '""" + device_id + """'""" +
                 """ AND timestamp > '""" + str(startDate) + """'""" +
                 """ ORDER BY timestamp""")
        inputData = psql.query(connection=con, query=query)
        psql.disconnect(con)

        # print the number of rows in the input data
        print(datetime.datetime.now(), ", Initial Dataframe, " + str(inputData.shape[0]), ", ", device)

        # json
        inputData['sensor_data'] = inputData['sensor_data'].astype(str)
        inputData['sensor_data'] = inputData['sensor_data'].str.replace("'", '"')
        inputData['sensor_data'] = inputData['sensor_data'].apply(json.loads)
        df_expanded = pd.json_normalize(inputData['sensor_data'])
        df_merged = pd.concat([inputData, df_expanded], axis=1)
        inputData = df_merged.drop('sensor_data', axis=1)

        ##### Info #####
        startDate = min(inputData['timestamp'])
        startDate = startDate.replace(minute=0, second=0, microsecond=0)

        # create days column
        inputData['timestamp'] = pd.to_datetime(inputData['timestamp'], utc=True)
        inputData['days'] = (inputData['timestamp'] - startDate).dt.days

        summary = inputData.describe(percentiles=[0.01, 0.05, 0.1, 0.95, 0.99])

        ##### Fudge #####
        inputData['location.latitude'] = inputData['location.latitude'] + 3.324351
        inputData['location.longitude'] = inputData['location.longitude'] + 0.481391


        ##### Remove Erroneous Data ######
        inputData = inputData[inputData["quality.fix"] <= 3]
        print(datetime.datetime.now(), ", After fix quality, " + str(inputData.shape[0]), ", ", device)
        inputData = inputData[inputData["quality.hdop"] <= 5]
        print(datetime.datetime.now(), ", After HDOP Quality, " + str(inputData.shape[0]), ", ", device)
        inputData = inputData[inputData["quality.vdop"] <= 5]
        print(datetime.datetime.now(), ", After VDOP Quality, " + str(inputData.shape[0]), ", ", device)
        inputData = inputData[inputData["quality.pdop"] <= 5]
        print(datetime.datetime.now(), ", After PDOP Quality, " + str(inputData.shape[0]), ", ", device)
        inputData = inputData[inputData["quality.sats"] >= 4.5]
        print(datetime.datetime.now(), ", After Satellites, " + str(inputData.shape[0]), ", ", device)
        inputData = inputData[inputData["location.latitude"] + inputData["location.longitude"] != 0]
        inputData = inputData[(inputData["location.altitude"] >= 0)]
        print(datetime.datetime.now(), ", After Altitude, " + str(inputData.shape[0]), ", ", device)

        inputData['z_lat'] = 0
        inputData['z_lon'] = 0
        inputData['z_alt'] = 0
        inputData['z_spd'] = 0
        for day in range(0, int(np.max(inputData['days'])) + 1):
            if len(inputData[inputData['days'] == day]) > 0:
                # z score
                if min(inputData[inputData['days'] == day]["location.latitude"]) != max(
                        inputData[inputData['days'] == day]["location.latitude"]):
                    inputData[inputData['days'] == day]["z_lat"] = np.abs(
                        stats.zscore(inputData[inputData['days'] == day]["location.latitude"]))
                else:
                    inputData[inputData['days'] == day]["z_lat"] = 0
                if min(inputData[inputData['days'] == day]["location.longitude"]) != max(
                        inputData[inputData['days'] == day]["location.longitude"]):
                    inputData[inputData['days'] == day]["z_lon"] = np.abs(
                        stats.zscore(inputData[inputData['days'] == day]["location.longitude"]))
                else:
                    inputData[inputData['days'] == day]["z_lon"] = 0
                if min(inputData[inputData['days'] == day]["location.altitude"]) != max(
                        inputData[inputData['days'] == day]["location.altitude"]):
                    inputData[inputData['days'] == day]["z_alt"] = np.abs(
                        stats.zscore(inputData[inputData['days'] == day]["location.altitude"]))
                else:
                    inputData[inputData['days'] == day]["z_alt"] = 0
                if min(inputData[inputData['days'] == day]["location.speed"]) != max(
                        inputData[inputData['days'] == day]["location.speed"]):
                    inputData[inputData['days'] == day]["z_spd"] = np.abs(
                        stats.zscore(inputData[inputData['days'] == day]["location.speed"]))
                else:
                    inputData[inputData['days'] == day]["z_spd"] = 0

        inputData = inputData[inputData["z_lat"] < 3]
        inputData = inputData[inputData["z_lon"] < 3]
        inputData = inputData[inputData["z_alt"] < 3]
        inputData = inputData[inputData["z_spd"] < 3]
        inputData = inputData.reset_index(drop=True)
        print(datetime.datetime.now(), ", After Z Score, " + str(inputData.shape[0]), ", ", device)

        ##### Filtering Process - identify and remove outliers #####

        # Add missing time points
        inputData = inputData.sort_values('timestamp', ascending=True)
        inputData = inputData.drop_duplicates(subset='timestamp', keep='first')
        inputData = inputData.reset_index(drop=True)

        # Add missing time points
        dataRate = math.floor(np.median(inputData["ts_unix"][1:] - inputData["ts_unix"].shift(1)[1:]) / 100.0) * 100
        for n in range(1, len(inputData)):
            time_diff = inputData["ts_unix"][n] - inputData["ts_unix"][n - 1]
            if round(time_diff / dataRate, 0) > 1:
                for missing_row in range(1, int(round(time_diff / dataRate, 0))):
                    df = inputData[inputData["ts_unix"] == inputData["ts_unix"][n]]
                    df["ts_unix"] = int(inputData["ts_unix"][n] + (dataRate * missing_row))
                    df.iloc[:, 6:] = np.nan
                    inputData = pd.concat([inputData, df])
                    inputData = inputData.reset_index(drop=True)

        inputData = inputData.drop_duplicates(subset='timestamp', keep='first')
        inputData = inputData.sort_values('timestamp', ascending=True)
        inputData = inputData.reset_index(drop=True)

        # kalman
        iterations = 1
        measurements = np.ma.masked_invalid(
            inputData[['location.longitude', 'location.latitude', 'location.altitude']].values)
        for i in range(0, iterations):
            transition_matrices = np.array([[1, 0, 0, 1, 0, 0],
                                            [0, 1, 0, 0, 1, 0],
                                            [0, 0, 1, 0, 0, 1],
                                            [0, 0, 0, 1, 0, 0],
                                            [0, 0, 0, 0, 1, 0],
                                            [0, 0, 0, 0, 0, 1]])

            observation_matrices = np.array([[1, 0, 0, 0, 0, 0],
                                             [0, 1, 0, 0, 0, 0],
                                             [0, 0, 1, 0, 0, 0]])

            observation_covariance = np.diag([1e-4, 1e-4, 100]) ** 2

            initial_state_mean = np.hstack([measurements[0, :], 3 * [0.]])
            initial_state_covariance = np.diag([1e-3, 1e-3, 100, 1e-4, 1e-4, 1e-4]) ** 2

            kf = KalmanFilter(transition_matrices=transition_matrices,
                              observation_matrices=observation_matrices,
                              observation_covariance=observation_covariance,
                              initial_state_mean=initial_state_mean,
                              initial_state_covariance=initial_state_covariance,
                              em_vars=['transition_covariance'])
            kf = kf.em(measurements, n_iter=1000)
            (smoothed_state_means, smoothed_state_covariances) = kf.smooth(measurements)
            measurements = np.ma.masked_invalid(pd.DataFrame(smoothed_state_means[:, 0:3],
                                                             columns=['location.longitude', 'location.latitude',
                                                                      'location.altitude']))

        # Reconfigure dataframe
        inputData = pd.concat([
            inputData,
            pd.DataFrame(smoothed_state_means[:, 0:3], columns=['lon_smooth', 'lat_smooth', 'alt_smooth', ])],
            axis=1)

        # Find indexes of inputData where alt_smooth - location.altitude > 100
        inputData = inputData[abs(inputData['alt_smooth'] - inputData['location.altitude']) <= 100]
        # calculate the distance between the smoothed and raw data using get_distance
        inputData.reset_index(drop=True, inplace=True)
        err_dist = np.array(0)
        for n in range(1, inputData.shape[0]):
            err_dist = np.append(err_dist, get_distance(inputData["lon_smooth"][n], inputData["lat_smooth"][n],
                                                        inputData["location.longitude"][n],
                                                        inputData["location.latitude"][n]))
        # Find indexes of inputData where err_dist > 50
        inputData = inputData[err_dist <= 50]

        # drop unnecessary columns
        inputData = inputData.drop(['lon_smooth', 'lat_smooth', 'alt_smooth'], axis=1)
        # print status
        print(datetime.datetime.now(), ", After Kalman, " + str(inputData.shape[0]), ", ", device)

        ##### Filtering Process - final #####

        # Add missing time points
        inputData = inputData.sort_values('timestamp', ascending=True)
        inputData = inputData.drop_duplicates(subset='timestamp', keep='first')
        inputData = inputData.reset_index(drop=True)

        # Add missing time points
        dataRate = math.floor(np.median(inputData["ts_unix"][1:] - inputData["ts_unix"].shift(1)[1:]) / 100.0) * 100
        for n in range(1, len(inputData)):
            time_diff = inputData["ts_unix"][n] - inputData["ts_unix"][n - 1]
            if round(time_diff / dataRate, 0) > 1:
                for missing_row in range(1, int(round(time_diff / dataRate, 0))):
                    df = inputData[inputData["ts_unix"] == inputData["ts_unix"][n]]
                    df["ts_unix"] = int(inputData["ts_unix"][n] + (dataRate * missing_row))
                    df.iloc[:, 6:] = np.nan
                    inputData = pd.concat([inputData, df])
                    inputData = inputData.reset_index(drop=True)

        inputData = inputData.drop_duplicates(subset='timestamp', keep='first')
        inputData = inputData.sort_values('timestamp', ascending=True)
        inputData = inputData.reset_index(drop=True)

        # kalman
        iterations = 1
        measurements = np.ma.masked_invalid(
            inputData[['location.longitude', 'location.latitude', 'location.altitude']].values)
        for i in range(0, iterations):
            transition_matrices = np.array([[1, 0, 0, 1, 0, 0],
                                            [0, 1, 0, 0, 1, 0],
                                            [0, 0, 1, 0, 0, 1],
                                            [0, 0, 0, 1, 0, 0],
                                            [0, 0, 0, 0, 1, 0],
                                            [0, 0, 0, 0, 0, 1]])

            observation_matrices = np.array([[1, 0, 0, 0, 0, 0],
                                             [0, 1, 0, 0, 0, 0],
                                             [0, 0, 1, 0, 0, 0]])

            observation_covariance = np.diag([1e-4, 1e-4, 100]) ** 2

            initial_state_mean = np.hstack([measurements[0, :], 3 * [0.]])
            initial_state_covariance = np.diag([1e-3, 1e-3, 100, 1e-4, 1e-4, 1e-4]) ** 2

            kf = KalmanFilter(transition_matrices=transition_matrices,
                              observation_matrices=observation_matrices,
                              observation_covariance=observation_covariance,
                              initial_state_mean=initial_state_mean,
                              initial_state_covariance=initial_state_covariance,
                              em_vars=['transition_covariance'])
            kf = kf.em(measurements, n_iter=1000)
            (smoothed_state_means, smoothed_state_covariances) = kf.smooth(measurements)
            measurements = np.ma.masked_invalid(pd.DataFrame(smoothed_state_means[:, 0:3],
                                                             columns=['location.longitude', 'location.latitude',
                                                                      'location.altitude']))

        # Reconfigure dataframe
        inputData = pd.concat([
            inputData,
            pd.DataFrame(smoothed_state_means[:, 0:3], columns=['lon_smooth', 'lat_smooth', 'alt_smooth', ])],
            axis=1)

        ##### Calculate Longitudinal Values #####
        if inputData.shape[0] > 0:

            inputData.reset_index(drop=True, inplace=True)
            dist_smooth = np.array(0)
            heading_smooth = np.array(0)
            speed_smooth = np.array(0)
            climb_smooth = np.array(0)
            err_array = np.array(0)

            valid_index = 1
            for n in range(1, inputData.shape[0]):
                # setup
                err_data = False

                # basics
                ts = inputData["ts_unix"][n]
                prev_ts = inputData["ts_unix"][n - valid_index]
                lat = inputData["lat_smooth"][n]
                prev_lat = inputData["lat_smooth"][n - valid_index]
                lon = inputData["lon_smooth"][n]
                prev_lon = inputData["lon_smooth"][n - valid_index]
                alt = inputData["alt_smooth"][n]
                prev_alt = inputData["alt_smooth"][n - valid_index]

                # distance
                distance = round(get_distance(prev_lat, prev_lon, lat, lon), 3)
                dist_smooth = np.append(dist_smooth, distance)

                # heading
                heading = round(get_bearing(prev_lat, prev_lon, lat, lon), 3)
                heading_smooth = np.append(heading_smooth, heading)

                # speed
                speed = round(distance / (ts - prev_ts), 3)
                speed_smooth = np.append(speed_smooth, speed)
                if speed > 14:
                    err_data = True

                # climb
                climb = round(alt - prev_alt, 0)
                climb_smooth = np.append(climb_smooth, climb)
                if abs(climb) > 200:
                    err_data = True

                # error
                err_array = np.append(err_array, err_data)
                if err_data:
                    valid_index = valid_index + 1
                else:
                    valid_index = 1

            inputData["dist_smooth"] = dist_smooth
            inputData["heading_smooth"] = heading_smooth
            inputData["speed_smooth"] = speed_smooth
            inputData["climb_smooth"] = climb_smooth

            # remove data where err_array is true
            inputData = inputData[err_array == 0]

            # fix indexes
            inputData.reset_index(drop=True, inplace=True)

            # remove data older than the new data
            inputData = inputData[inputData["ts_unix"] > (int(time.mktime(startDate.timetuple())) + (6 * 3600))]

            # fudge lat and long


            # setup dataframe for processed table
            outputData = inputData[[
                'timestamp', 'ts_unix', 'device_id', 'datahub_id', 'location.duration',
                'lat_smooth', 'lon_smooth', 'alt_smooth',
                'location.speed', 'location.heading', 'location.turnAngle', 'location.distance',
                'dist_smooth', 'heading_smooth', 'speed_smooth', 'climb_smooth',
                'quality.sats', 'quality.hdop', 'quality.vdop', 'quality.pdop', 'quality.fix']]
            outputData = outputData.set_axis(
                ["timestamp", "ts", "device_id", "datahub_id", "sample_length",
                 "latitude", "longitude", "altitude",
                 "speed_instantaneous", "heading_instantaneous", "turn_angle_instantaneous", "distance_instantaneous",
                 "distance_longitudinal", "speed_longitudinal", "heading_longitudinal", "climb_longitudinal",
                 "satellites", "hdop", "vdop", "pdop", "fix"],
                axis=1)

            # change column types
            outputData['device_id'] = outputData['device_id'].astype("string")
            outputData['datahub_id'] = outputData['datahub_id'].astype("string")
            outputData['timestamp'] = outputData['timestamp'].astype("string")

            # set data type for output
            outputData = outputData.round({
                "latitude": 6,
                "longitude": 6,
                "altitude": 0,
                "distance_instantaneous": 3,
                "speed_instantaneous": 3,
                "heading_instantaneous": 3,
                "turn_angle_instantaneous": 3,
                "distance_longitudinal": 3,
                "speed_longitudinal": 3,
                "heading_longitudinal": 3,
                "climb_longitudinal": 3,
                "hdop": 3,
                "vdop": 3,
                "pdop": 3,
                "satellites": 0
            })

            # remove data in the database to avoid adding duplicates
            outputData = outputData.sort_values('ts', ascending=True)
            outputData = outputData.drop_duplicates(subset='ts', keep='first')
            outputData.reset_index(drop=True, inplace=True)

            con = psql.connect(database, host, user, password)
            # remove updated data
            query = str(
                """DELETE FROM """ + output_table + """ where device_id = '""" + device_id + """' and ts >= """ + str(
                    int(min(outputData["ts"]))) + """ and ts <= """ + str(int(max(outputData["ts"]))))
            psql.send_query(connection=con, query=query)

            # insert new data into table
            psql.insert(con, outputData, output_table)

            psql.disconnect(con)
            print(datetime.datetime.now(), "Uploaded data for device: " + device_id)
    except:
        print(datetime.datetime.now(), "Error processing data for device: " + device)

devices = get_devices()
print(datetime.datetime.now(), " - Starting Processing ", devices['device_id'].shape[0], " devices")
result = Parallel(n_jobs=6)(delayed(gnss_processing)(device) for device in devices['device_id'])
# for device in devices['device_id']:
#   gnss_processing(device)
print(datetime.datetime.now(), " - Processing Complete")
