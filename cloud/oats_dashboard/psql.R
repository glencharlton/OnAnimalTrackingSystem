###############################################################################
# Author: Glen Charlton
# Last Modified: 2024/03/27
# Purpose: For accessing the psql database

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

# Load the required library
library(RPostgreSQL)
source('credentials.R')

database_getCredentials <- function(){
  library(RPostgreSQL)
  con = dbConnect(dbDriver("PostgreSQL"),
                  dbname = postgres_database,
                  user = postgres_user,
                  password = postgres_password,
                  host = postgres_host,
                  port = 5432
  )
  result <- dbGetQuery(con, "SELECT email, password from dashboard.credentials where active = true")
  dbDisconnect(con)
  colnames(result) <- c("username", "password")
  return(result)
}


database_getQuery <- function(query){
  library(RPostgreSQL)
  con = dbConnect(dbDriver("PostgreSQL"),
                  dbname = postgres_database,
                  user = postgres_user,
                  password = postgres_password,
                  host = postgres_host,
                  port = 5432
  )
  result <- dbGetQuery(con, query)
  dbDisconnect(con)
  return(result)
}
