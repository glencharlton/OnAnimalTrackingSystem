###############################################################################
# Author: Glen Charlton
# Last Modified: 2024/03/27
# Purpose: For creating map outputs for the RShiny Dashboard 

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


##### Map Tab Functions #####

map_basic <- function(df) {
  print('Display Map')
  
  name <- names(table(df$name))
  pal <- colorFactor(palette = 'Set1', df$name)
  
  if (length(name) == 1) {
    map <- leaflet(df) %>%
      addBingTiles(
        "Aq4GyoG8kzfeKO7Nsav5_BcjVVA_d1ULSTeXeW2zM0aPuANIqhvV5IrFtMjOGX3s",
        imagerySet = c("AerialWithLabels"),
        group = "AerialWithLabels",
        maxNativeZoom = 18,
        maxZoom = 21
      ) %>%
      addCircleMarkers(
        data = df,
        lat = ~ as.numeric(latitude),
        lng = ~ as.numeric(longitude),
        color = ~ pal(name),
        stroke = FALSE,
        fillOpacity = 1,
        radius = 5
      ) %>%
      addPolylines(
        data = df,
        lat = ~ as.numeric(latitude),
        lng = ~ as.numeric(longitude)
      ) %>%
      addLegend(pal = pal,
                values = ~ name,
                opacity = 1.0)
  }
  if (length(name) != 1) {
    map <- leaflet(df) %>%
      addBingTiles(
        "Aq4GyoG8kzfeKO7Nsav5_BcjVVA_d1ULSTeXeW2zM0aPuANIqhvV5IrFtMjOGX3s",
        imagerySet = c("AerialWithLabels"),
        group = "AerialWithLabels",
        maxNativeZoom = 18,
        maxZoom = 21
      ) %>%
      addCircleMarkers(
        data = df,
        lat = ~ as.numeric(latitude),
        lng = ~ as.numeric(longitude),
        color = ~ pal(name),
        stroke = FALSE,
        fillOpacity = 1,
        radius = 5
      ) %>%
      addLegend(pal = pal,
                values = ~ name,
                opacity = 1.0)
  }
  return(map)
}

map_kde <- function(df)
{
  print('Run Kernal Density Estimation')
  # Create kernel density output
  kde <-
    bkde2D(df[, c("longitude", "latitude")],
           bandwidth = c(.0001, .0001),
           gridsize = c(1000, 1000))
  # Create Raster from Kernel Density output
  KernelDensityRaster <-
    raster(list(
      x = kde$x1 ,
      y = kde$x2 ,
      z = kde$fhat
    ))
  
  # set low density cells as NA so we can make them transparent with the colorNumeric function
  KernelDensityRaster@data@values <-
    (KernelDensityRaster@data@values / max(KernelDensityRaster@data@values)) * 100
  KernelDensityRaster@data@values[which(KernelDensityRaster@data@values < 10)] <-
    NA
  
  #create pal function for coloring the raster
  palRaster <-
    colorBin(
      "Spectral",
      bins = 7,
      domain = KernelDensityRaster@data@values,
      na.color = "transparent"
    )
  
  print("Display Heat Map")
  # Leaflet map with raster
  map <- leaflet() %>%
    addBingTiles(
      "Aq4GyoG8kzfeKO7Nsav5_BcjVVA_d1ULSTeXeW2zM0aPuANIqhvV5IrFtMjOGX3s",
      imagerySet = c("AerialWithLabels"),
      group = "AerialWithLabels",
      maxNativeZoom = 18,
      maxZoom = 21
    ) %>%
    addRasterImage(KernelDensityRaster,
                   colors = palRaster,
                   opacity = .8) %>%
    addLegend(pal = palRaster,
              values = KernelDensityRaster@data@values,
              title = "Scaled Kernel Density")
  return(map)
}
