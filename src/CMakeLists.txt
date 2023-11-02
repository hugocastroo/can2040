# Generated Cmake Pico project file
cmake_minimum_required(VERSION 3.13)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialise pico_sdk from installed location
set(PICO_SDK_PATH "../../pico-sdk")
# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(canHugo C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable.
add_executable(canHugo
canHugo.c
)

#Enable printf output,. enable usb output, disable uart output
pico_enable_stdio_uart(canHugo 0)
pico_enable_stdio_usb(canHugo 1)

# Add the standard library to the build
target_link_libraries(canHugo pico_stdlib hardware_structs hardware_regs cmsis_core_headers)

# Add the standard include files to the build
target_include_directories(canHugo PRIVATE ${CMAKE_CURRENT_LIST_DIR})

#Generate all the output files for the Pico board
pico_add_extra_outputs(canHugo)



