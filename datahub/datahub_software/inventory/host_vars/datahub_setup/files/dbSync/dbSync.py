###############################################################################
# Author: Glen Charlton
# Last Modified: 2024/04/11
# Purpose: Python script to do a full psql datahub to cloud sync. 
#
# Software License Agreement (BSD License)
#
# Copyright (c) 2024, Glen Charlton
# All rights reserved.
#
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


##### Libraries #####
import sys
import pandas as pd
import json
import time

##### Local Files #####
import psql
import config

try:
    #Get datahub id from json file
    with open('/home/pi/config.json') as json_file:
        data = json.load(json_file)
        for item in data:
            if item['key'] == 'datahub_id':
                datahub_id = item['value']
                break

    #Connect to local database to get unchecked rows
    con = psql.connect('sensors', 'localhost', config.local_user, config.local_pw)
    # Get local datapoints
    query = "SELECT * FROM {} WHERE checked = false ORDER BY ts_unix".format('oats_data')
    df_local = psql.get_query(con, query)
    #disccount to cloud database
    con = psql.disconnect(con)
    print('Datapoints to be checked: ', len(df_local))

    # Loop through each row and check if in cloud database
    #Connect to cloud database
    con = psql.connect('sensors', config.cloud_hostname, config.cloud_user, config.cloud_pw)
    # Create output dataframe
    df_output = pd.DataFrame(columns=df_local.columns)
    for i in range(0, df_local.shape[0]):
        query = "SELECT count(*) FROM {} WHERE device_id = '{}' AND data_type = '{}' AND timestamp = '{}'".format('sensors.data', df_local['device_id'][i], df_local['data_type'][i], df_local['timestamp'][i])
        if(psql.get_query(con, query).iloc[0][0] >= 1):
            df_local['checked'][i] = True
        else:
            # add to output dataframe
            df_output = pd.concat([df_output, df_local.iloc[[i]]])
    #disccount to cloud database
    con = psql.disconnect(con)
    print('Datapoints to be uploaded: ', len(df_output))

    # Insert missing data into dataframe in batches of 1000
    batch_size = 1000
    num_missing = len(df_output)
    num_batches = (num_missing + batch_size - 1) // batch_size

    for batch_index in range(num_batches):
        start_index = batch_index * batch_size
        end_index = batch_index * batch_size + batch_size
        if end_index > num_missing:
            end_index = num_missing
        df_batch = df_output.iloc[start_index:end_index]
        # select and order columns
        df_batch = df_batch[['timestamp', 'datahub_id', 'device_id', 'ts_unix', 'data_type', 'sensor_data']]
        con = psql.connect('sensors', config.cloud_hostname, config.cloud_user, config.cloud_pw)
        psql.insert(con, df_batch, 'sensors.data')
        con = psql.disconnect(con)

    # Loop through each row and check if in cloud database
    print('Dataoints to be checked: ', len(df_local))
    #Connect to cloud database
    con = psql.connect('sensors', config.cloud_hostname, config.cloud_user, config.cloud_pw)
    # Create output dataframe
    df_output = pd.DataFrame(columns=df_local.columns)
    for i in range(0, df_local.shape[0]):
        query = "SELECT count(*) FROM {} WHERE device_id = '{}' AND data_type = '{}' AND timestamp = '{}'".format('sensors.data', df_local['device_id'][i], df_local['data_type'][i], df_local['timestamp'][i])
        if(psql.get_query(con, query).iloc[0][0] >= 1):
            df_local['checked'][i] = True
        else:
            # add to output dataframe
            df_output = pd.concat([df_output, df_local.iloc[[i]]])
    #disccount to cloud database
    con = psql.disconnect(con)
    print('Failed to upload: ', len(df_output))

    # Connect to local database to update checked rows
    con = psql.connect('sensors', 'localhost', config.local_user, config.local_pw)
    # Loop through dataframe and update checked rows
    for i in range(0, df_local.shape[0]):
        query = "UPDATE {} SET checked = {} WHERE row_id = '{}'".format('oats_data', df_local['checked'][i], df_local['row_id'][i])
        psql.update_query(con, query)
    #disccount to cloud database
    con = psql.disconnect(con)

    print('Data Sync Complete')
except:
    print("No Data Uploaded")