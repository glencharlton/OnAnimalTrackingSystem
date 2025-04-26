#  OATS - psql.py
#  Description:      File containing functions for postgresql for project to write data into lookup tables from google sheets
#  Author:           Glen Charlton
#  Created:          11 April 2023

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
def get_query(connection, query):
    try:
        df = pd.read_sql(query, connection)
        return df
    except Exception as ex:
        print("Error executing the query: ", ex)
        return None

# A query to make an update to the database.
def update_query(connection, query):
    try:
        cursor = connection.cursor()
        cursor.execute(query)
        connection.commit()
    except Exception as error:
        print("Error in update operation: ", error)
        connection.rollback()