cmake_minimum_required(VERSION 3.20.0)

# option(POWER_NODE "Select Power Node" OFF)
# option(PROXY_NODE "Select Proxy Node" OFF)

# IF(${POWER_NODE})
# set(CONF_FILE conf_files/prj.conf conf_files/power_node.conf)
# ELSEIF(${PROXY_NODE})
# set(CONF_FILE conf_files/prj.conf conf_files/proxy_node.conf)
# ENDIF()

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(power_node)

include_directories(    
                        
                        ../oslib/ble_driver/
                        )

target_sources(app PRIVATE 
                    src/main.c
                    ../oslib/ble_driver/ble_power.c
                    )

