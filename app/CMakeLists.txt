cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE}) # creates target "app"

project(app LANGUAGES C)

target_compile_options(app PRIVATE
    -Wall
    -Werror
    -Wno-unused-parameter
)

target_sources(app PRIVATE
    src/gps.c
    src/lte.c
    src/main.c
    src/thingsboard.c
    src/payload.c
)
