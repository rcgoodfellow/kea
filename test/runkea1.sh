#!/bin/bash

export PGPASSWORD=muffins 

until psql -h pgdb -U murphy -c 'create database kea'
do
  echo "Waiting for database to become available"
  sleep 1
done

psql -h pgdb -U murphy -c "CREATE USER keatest WITH PASSWORD 'keatest'"
psql -h pgdb -U murphy -c "CREATE DATABASE keatest"
psql -h pgdb -U murphy -c "GRANT ALL PRIVILEGES ON DATABASE keatest TO keatest"
psql -h pgdb -U murphy -c "CREATE USER keatest_readonly WITH PASSWORD 'keatest'"

export PGPASSWORD=keatest
psql -h pgdb -U keatest -c \
  "ALTER DEFAULT PRIVILEGES IN SCHEMA public \
  GRANT SELECT ON TABLES to keatest_readonly"

  
kea-admin lease-init pgsql -h pgdb -u murphy -p muffins -n kea 

supervisord -n
