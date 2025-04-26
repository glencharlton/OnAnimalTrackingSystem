###############################################################################
# Author: Glen Charlton
# Last Modified: 2024/03/27
# Purpose: For accessing the postgresql database

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

import psycopg2
import pandas as pd

##### function for connecting to psql database #####
def connect(database, host, username, password):
    try:
        connection = psycopg2.connect(
            host=host,
            port=5432,
            database=database,
            user=username,
            password=password
        )
        return connection
    except psycopg2.OperationalError as e:
        #print("Error connecting to the database: ", e)
        return None

##### function for disconnecting from database #####
def disconnect(connection):
    connection.close()

##### function to check if the data exists in the database #####
def db_contains(connection, row_id, datahub_id):
    query = "SELECT count(*) FROM sensors.data WHERE row_id = '{}' AND datahub_id = '{}'".format(str(row_id), str(datahub_id))

    cur = connection.cursor()
    cur.execute(query)
    count = cur.fetchone()[0]

    if count == 0:
        return False
    else:
        return True

##### function for inserting dataframe into database table #####
def insert(connection, df, table_name):
    try:
        cursor = connection.cursor()
        column_names = ', '.join(df.columns)
        values = ', '.join(["%s" for _ in df.columns])
        query = "INSERT INTO {} ({}) VALUES ({})".format(table_name, column_names, values)
        cursor.executemany(query, [tuple(row) for row in df.to_numpy()])
        connection.commit()
    except psycopg2.Error as e:
        print("Error inserting the data: ", e)
        connection.rollback()

# This function is used for custom queries.
def query(connection, query):
    try:
        df = pd.read_sql(query, connection)
        return df
    except Exception as ex:
        print("Error executing the query: ", ex)
        return None

def send_query(connection, query):
    try:
        with connection.cursor() as cursor:
            cursor.execute(query)
        connection.commit()
    except Exception as ex:
        print("Error executing the send query:", ex)
