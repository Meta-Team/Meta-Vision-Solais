cmake_minimum_required(VERSION 3.10)
project(Meta_Vision_Solais)

set(CMAKE_CXX_STANDARD 17)


# Find OpenCV. Jetson Nano has OpenCV 4.1.1 installed. Here minimal version 4 is required.
find_package(OpenCV 4 REQUIRED)
message("Found OpenCV")
message("    Version: " ${OpenCV_VERSION})
message("    Includes: " ${OpenCV_INCLUDE_DIRS})

# Find Boost
find_package(Boost REQUIRED system filesystem json)

# Shared include directory
include_directories(include)

add_subdirectory(extra)
add_subdirectory(src)
add_subdirectory(tools)
add_subdirectory(unit-tests)