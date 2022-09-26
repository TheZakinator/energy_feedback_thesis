find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(embedded_devices/proxy_node)

# include_directories()

target_sources(app PRIVATE 
                        src/main.c)

cmake_minimum_required(VERSION 3.20.0)

set(BOARD_FLASH_RUNNER jlink)