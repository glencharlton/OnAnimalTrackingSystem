#######################################
# R Script for Connecting to a PostgreSQL Database
# Author: [Your Name]
# Date: [Current Date]
# Updated Data: [Data Updated]
# Dependencies: RPostgreSQL package
#######################################

##### Clear environment and tidy up #####
rm(list = ls())
gc()

##### Libraries #####
library(shiny)
library(shinymanager)
library(shinythemes)
library(leaflet)
library(leaflet.extras)
library(data.table)
library(sp)
library(RPostgreSQL)
source('psql.R')
source('map.R')


# define some credentials <- get this from a postgresql database
credentials <- database_getCredentials()

# Main Screen
ui <- navbarPage(
  "On Animal Tracking System",
  theme = shinytheme(theme = "lumen"),
  tabPanel("Get Data", uiOutput("summary")),
  tabPanel("Map", uiOutput("map"))
)

# Login Screen
set_labels(
  language = "en",
  "Please authenticate" = "Welcome to the OATS Dashboard. Please Login:",
  "Username:" = "Email:",
  "Password:" = "Password:",
  "Login" = "Login"
  )
ui <- secure_app(ui,
                 theme = shinytheme(theme = "lumen"))

set_labels(
  language = "en",
  "Please authenticate" = "Welcome to the OATS Dashboard. Please Login:",
  "Username:" = "Username:",
  "Password:" = "Password:",
  "Login" = "Login"
)

server <- function(input, output, session) {
  # call the server part
  # check_credentials returns a function to authenticate users
  res_auth <-
    secure_server(check_credentials = check_credentials(credentials))
  
  output$auth_output <- renderPrint({
    reactiveValuesToList(res_auth)
  })
  
  #### Reactive Results #####
  rResult <-
    reactiveValues(project = NULL,
                   devices = NULL,
                   sensordata = NULL)
  
  #### Logo for each tab #####
  output$summary_logo <- renderImage({
    return(list(
      src = "www/logo.png",
      contentType = "image/PNG",
      alt = "Logo Missing"
    ))
  }, deleteFile = FALSE)
  output$deviceInfo_logo <- renderImage({
    return(list(
      src = "www/logo.png",
      contentType = "image/PNG",
      alt = "Logo Missing"
    ))
  }, deleteFile = FALSE)
  output$map_logo <- renderImage({
    return(list(
      src = "www/logo.png",
      contentType = "image/PNG",
      alt = "Logo Missing"
    ))
  }, deleteFile = FALSE)
  
  #### Summary Tab Functions #####
  
  output$summary_instructions <-
    renderText("Select project and devices to get data before viewing the selected data in other tabs")
  
  #Get Devices for Project
  observeEvent(input$summary_project_sendGET, {
    showModal(modalDialog("Loading Available Devices...", footer = NULL))
    
    rResult$project = input$summary_project
    
    print('Requesting Data for Project from Database')
    
    query <- paste0("
      SELECT
          sensor_summary.name, sensor_summary.project, sensor_summary.device_id, sensor_summary.Location, sensor_summary.Accelerometer, sensor_summary.System, sensor_summary.Battery, sensor_summary.count
          FROM
                (SELECT
                    sensors.devices.name, sensors.devices.project,
                    sd1.device_id,
                    MAX(CASE WHEN data_type = 'location' THEN timestamp AT TIME ZONE 'Australia/Sydney' END) AS Location,
                    MAX(CASE WHEN data_type = 'accelerometer_features' THEN timestamp AT TIME ZONE 'Australia/Sydney' END) AS Accelerometer,
                    MAX(CASE WHEN data_type = 'systemlog' THEN timestamp AT TIME ZONE 'Australia/Sydney' END) AS System,
                    (SELECT sensor_data ->> 'vbat'
                     FROM sensors.data AS sd2
                     WHERE data_type = 'systemlog'
                       AND sd2.device_id = sd1.device_id
                     ORDER BY timestamp DESC
                     LIMIT 1) AS Battery,
                    count(*) as count
                FROM sensors.data AS sd1
                RIGHT JOIN sensors.devices ON sensors.devices.device_id = sd1.device_id
                WHERE project = '", rResult$project, "'
                  AND timestamp > sensors.devices.date_start
                  AND timestamp < sensors.devices.date_end
                GROUP BY sd1.device_id, sensors.devices.name, sensors.devices.project) sensor_summary
            WHERE System IS NOT NULL AND System > '2023-01-01 00:00:00.000000 +00:00'
            ORDER BY device_id;")
    
    df <- database_getQuery(query)
    colnames(df) = c("NAME", "PROJECT", "DEVICE_ID", "LOCATION", "ACCELEROMETER", "SYSTEM", "BATTERY", "PACKETS")
    if (nrow(df) == 0) {
      showModal(modalDialog("Invalid Project", footer = NULL))
      Sys.sleep(3)
    } else{
      print('Assign to rResult')
      rResult$summary <- df
      rResult$devices <- names(table(df$DEVICE_ID))
      print(rResult$summary)
      print(rResult$devices)
      
      df_out <- df
      df_out$LOCATION <- as.character(df_out$LOCATION)
      df_out$ACCELEROMETER <- as.character(df_out$ACCELEROMETER)
      df_out$SYSTEM <- as.character(df_out$SYSTEM)
      
      output$summary_data_available_title <-
        renderText(paste0(
          "Devices summary for devices in this project: ",
          rResult$project
        ))
      output$summary_data_available_table <- renderTable(df_out, "")
      
      #device summary tab output
      showModal(modalDialog("Creating Device Status Table...", footer =
                              NULL))
      print('Database Request for Device Status Table')
    }
    
    removeModal()
    
    
    
    
  })
  
  ##### Map Tab Functions #####
  observeEvent(input$map_sendGET, {
    output$mapPlot <- leaflet::renderLeaflet({
      showModal(modalDialog("Loading Map...", footer = NULL))
      
      print('Get Map Data')
      
      query <- paste0("
                        SELECT
                            devices.device_id as device_id, devices.name as name, devices.project as project,
                            location.timestamp as timestamp, location.ts as ts,
                            location.latitude as latitude, location.longitude as longitude, location.altitude as altitude,
                            location.speed_instantaneous as speed_instantaneous, location.heading_instantaneous as heading_instantaneous, location.turn_angle_instantaneous as turn_angle_instantaneous, location.distance_instantaneous as distance_instantaneous,
                            location.distance_longitudinal as distance_longitudinal, location.speed_longitudinal as speed_longitudinal, location.heading_longitudinal as heading_longitudinal, location.climb_longitudinal as climb_longitudinal,
                            location.satellites as satellites, location.hdop as hdop, location.vdop as vdop, location.pdop as pdop, location.fix as fix
                        FROM sensors.location
                        RIGHT JOIN sensors.devices ON sensors.devices.device_id = sensors.location.device_id
                        WHERE project = '", rResult$project, "'
                            AND timestamp > sensors.devices.date_start
                            AND timestamp < sensors.devices.date_end
                        ORDER BY timestamp, location.device_id;
                      ")
      
      df <- database_getQuery(query)
      
      start <-
        as.numeric(as.POSIXct(
          paste0(input$map_date[1], "00:00:01", format = "%Y-%m-%d, %H:%M:%s"),
          tz = 'Australia/Sydney'
        ))
      end   <-
        as.numeric(as.POSIXct(
          paste0(input$map_date[2], "23:59:59", format = "%Y-%m-%d, %H:%M:%s"),
          tz = 'Australia/Sydney'
        ))
      
      devices <- input$map_devices
      
      print('Wrangle Map Data')
      
      if(length(devices) > 0 ){
        df <- df[df$device_id %in% devices,]
      }
      
      df[,5:20] <- sapply(df[,5:20], as.numeric)
      df <- df[order(df$ts),]
      df <- df[as.numeric(df$ts) >= as.numeric(start) & as.numeric(df$ts) <= as.numeric(end),]
      df$device_id <- as.factor(df$device_id)
      df$name <- as.factor(df$name)

      print(paste0('Display Map for devices: ', input$map_devices, ' between ', start, ' and ', end))
      map <- map_basic(df)
      removeModal()
      return(map)
    })
  })
  
  output$map_download <- downloadHandler(
    filename = function() {
      paste0(
        "oats_location_",
        as.character(input$map_date[1]),
        "_",
        as.character(input$map_date[2]),
        ".csv"
      )
    },
    content = function(file) {
      query <- paste0("
                        SELECT
                            devices.device_id as device_id, devices.name as name, devices.project as project,
                            location.timestamp as timestamp, location.ts as ts,
                            location.latitude as latitude, location.longitude as longitude, location.altitude as altitude,
                            location.speed_instantaneous as speed_instantaneous, location.heading_instantaneous as heading_instantaneous, location.turn_angle_instantaneous as turn_angle_instantaneous, location.distance_instantaneous as distance_instantaneous,
                            location.distance_longitudinal as distance_longitudinal, location.speed_longitudinal as speed_longitudinal, location.heading_longitudinal as heading_longitudinal, location.climb_longitudinal as climb_longitudinal,
                            location.satellites as satellites, location.hdop as hdop, location.vdop as vdop, location.pdop as pdop, location.fix as fix
                        FROM sensors.location
                        RIGHT JOIN sensors.devices ON sensors.devices.device_id = sensors.location.device_id
                        WHERE project = '", rResult$project, "'
                            AND timestamp > sensors.devices.date_start
                            AND timestamp < sensors.devices.date_end
                        ORDER BY timestamp, location.device_id;
                      ")
      
      df <- database_getQuery(query)
      
      start <-
        as.numeric(as.POSIXct(
          paste0(input$map_date[1], "00:00:01", format = "%Y-%m-%d, %H:%M:%s"),
          tz = 'Australia/Sydney'
        ))
      end   <-
        as.numeric(as.POSIXct(
          paste0(input$map_date[2], "23:59:59", format = "%Y-%m-%d, %H:%M:%s"),
          tz = 'Australia/Sydney'
        ))
      
      devices <- input$map_devices
      
      print('Wrangle Map Data')
      
      if(length(devices) > 0 ){
        df <- df[df$device_id %in% devices,]
      }
      
      df[,5:20] <- sapply(df[,5:20], as.numeric)
      df <- df[order(df$ts),]
      df <- df[as.numeric(df$ts) >= as.numeric(start) & as.numeric(df$ts) <= as.numeric(end),]
      df$device_id <- as.factor(df$device_id)
      df$name <- as.factor(df$name)
      
      write.csv(df, file, row.names = FALSE)
    }
  )
  
  output$raw_download <- downloadHandler(
    filename = function() {
      paste0(
        "oats_raw_",
        as.character(input$map_date[1]),
        "_",
        as.character(input$map_date[2]),
        ".csv"
      )
    },
    content = function(file) {
      query <- paste0("
                        SELECT
                            *
                        FROM sensors.data
                        RIGHT JOIN sensors.devices ON sensors.devices.device_id = sensors.data.device_id
                        WHERE timestamp > sensors.devices.date_start
                            AND timestamp < sensors.devices.date_end
                        ORDER BY timestamp, data.device_id;
                      ")
      
      df <- database_getQuery(query)
      
      start <-
        as.numeric(as.POSIXct(
          paste0(input$map_date[1], "00:00:01", format = "%Y-%m-%d, %H:%M:%s"),
          tz = 'Australia/Sydney'
        ))
      end   <-
        as.numeric(as.POSIXct(
          paste0(input$map_date[2], "23:59:59", format = "%Y-%m-%d, %H:%M:%s"),
          tz = 'Australia/Sydney'
        ))
      
      devices <- input$map_devices
      
      print('Wrangle Raw Data')
      
      if(length(devices) > 0 ){
        df <- df[df$device_id %in% devices,]
      }
      
      df <- df[order(df$timestamp),]
      df <- df[as.numeric(df$ts_unix) >= as.numeric(start) & as.numeric(df$ts_unix) <= as.numeric(end),]
      df$device_id <- as.factor(df$device_id)
      df$name <- as.factor(df$name)
      
      write.csv(df, file, row.names = FALSE)
    }
  )
  
  ##### Tab Layout #####
  output$summary <- renderUI({
    sidebarLayout(
      sidebarPanel(
        imageOutput("summary_logo"),
        textOutput("summary_instructions"),
        isolate(textInput(
          "summary_project",
          "Project Name:",
          value = input$summary_project
        )),
        actionButton('summary_project_sendGET', 'Get Device List'),
      ),
      mainPanel(
        textOutput("summary_data_available_title"),
        tableOutput("summary_data_available_table"),
        textOutput("summary_data_fetched_title"),
        tableOutput("summary_data_fetched_table")
      )
    )
  })
  output$deviceInfo <- renderUI({
    sidebarLayout(sidebarPanel(imageOutput("deviceInfo_logo"), ),
                  mainPanel(
                    textOutput("deviceInfo_title"),
                    tableOutput("deviceInfo_table"),
                  ))
  })
  output$map <- renderUI({
    sidebarLayout(
      sidebarPanel(
        imageOutput("map_logo"),
        dateRangeInput("map_date", "Date Range:"),
        selectInput(
          "map_devices",
          "Filter by device: (selects all if empty)",
          choices = rResult$devices,
          multiple = TRUE
        ),
        actionButton('map_sendGET', 'View Map'),
        downloadButton('map_download', 'Download Location Data'),
        downloadButton('raw_download', 'Download Raw Data')
      ),
      mainPanel(leaflet::leafletOutput("mapPlot", height = 900))
    )
  })
}


shinyApp(ui, server)
