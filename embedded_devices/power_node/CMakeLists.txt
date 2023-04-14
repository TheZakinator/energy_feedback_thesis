cmake_minimum_required(VERSION 3.20.0)


option(PWR_NODE_AIRCON "Select aircon node" OFF)
option(PWR_NODE_TV "Select aircon node" OFF)
option(PWR_NODE_WATER_HEATER "Select aircon node" OFF)

IF(${PWR_NODE_AIRCON})
set(CONF_FILE conf_files/prj.conf conf_files/pwr_node_aircon.conf)
ELSEIF(${PWR_NODE_TV})
set(CONF_FILE conf_files/prj.conf conf_files/pwr_node_tv.conf)
ELSEIF(${PWR_NODE_WATER_HEATER})
set(CONF_FILE conf_files/prj.conf conf_files/pwr_node_water_heater.conf)
ENDIF()

# set(DTC_OVERLAY_FILE power_node_overlay.dtsi)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(power_node)


include_directories(    
                        ../oslib/ble_driver/
                        inc/
                        )

target_sources(app PRIVATE 
                    src/main.c
                    ../oslib/ble_driver/ble_power.c
                    )

