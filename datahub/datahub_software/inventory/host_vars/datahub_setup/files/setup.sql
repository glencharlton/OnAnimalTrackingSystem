--Postgresql setup 

-- create main data table
create table oats_data
(
    row_id            bigserial
        primary key,
    datahub_timestamp integer,
    datahub_id        name,
    device_id         name,
    timestamp         timestamp with time zone not null,
    ts_unix           numeric,
    data_type         name,
    sensor_data       varchar(1024),
    checked           boolean default false
);

create index oats_data_device_id_index
    on oats_data (device_id, data_type);
