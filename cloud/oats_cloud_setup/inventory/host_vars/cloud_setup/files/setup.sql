--Postgresql setup 

-- Enable TimescaleDB extension
CREATE EXTENSION IF NOT EXISTS timescaledb CASCADE;


--- Create Data Table with Hypertable
create schema sensors;

create table sensors.data ( 
    created_at timestamp with time zone default now(),
    timestamp timestamp with time zone not null, 
    datahub_id text, 
    device_id text, 
    ts_unix integer, 
    data_type text, 
    sensor_data json 
);
alter table sensors.data
    owner to postgres;

SELECT create_hypertable('sensors.data', 'timestamp');
create index sensors_timestamp_idx
    on sensors.data (timestamp desc);
create index sensors_ix_id_time
    on sensors.data (device_id, timestamp);
create index sensors_ix_id_time_type
    on sensors.data (device_id, timestamp, data_type);
    
--- Create Devices Table
create table sensors.devices
(
    device_id  text,
    name       text,
    project    text,
    date_start timestamp,
    date_end   timestamp
);

alter table sensors.devices
    owner to postgres;

--- Create Location data for processed data 
create table sensors.location
(
    timestamp                timestamp with time zone,
    ts                       integer,
    device_id                text,
    datahub_id               text,
    sample_length            integer,
    latitude                 numeric,
    longitude                numeric,
    altitude                 numeric,
    speed_instantaneous      numeric,
    heading_instantaneous    numeric,
    turn_angle_instantaneous numeric,
    distance_instantaneous   numeric,
    distance_longitudinal    numeric,
    speed_longitudinal       numeric,
    heading_longitudinal     numeric,
    climb_longitudinal       numeric,
    satellites               numeric,
    hdop                     numeric,
    vdop                     numeric,
    pdop                     numeric,
    fix                      numeric
);

alter table sensors.location
    owner to postgres;

create schema dashboard; 
create table dashboard.credentials (
    email varchar, 
    password varchar, 
    active bool );    
alter table dashboard.credentials owner to postgres;