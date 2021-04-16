cmake_minimum_required(VERSION 3.10)
project(Meta_Vision_Solais)

set(CMAKE_CXX_STANDARD 17)

if (CMAKE_PREFIX_PATH)

    set(QT_ENABLED 1)
    message("Qt is enabled")

    set(QT_VERSION 5)
    set(QT_LIBS Core Gui Widgets)
    set(QT_LIBS_QUALIFIED Qt5::Core Qt5::Gui Qt5::Widgets)

    find_package(Qt${QT_VERSION} COMPONENTS ${QT_LIBS} REQUIRED)

else()

    message("CMAKE_PREFIX_PATH is not defined, and therefore Qt is disabled. If you want to use Qt, you may need to set it "
            "(-DCMAKE_PREFIX_PATH=\"path/to/Qt/lib/cmake\" or -DCMAKE_PREFIX_PATH=/usr/include/{host}/qt{version}/ on Ubuntu)")

endif ()


# Find OpenCV. Jetson Nano has OpenCV 4.1.1 installed. Here minimal version 4 is required.
find_package(OpenCV 4 REQUIRED)
message("Found OpenCV")
message("    Version: " ${OpenCV_VERSION})
message("    Includes: " ${OpenCV_INCLUDE_DIRS})

# Find Boost
find_package(Boost REQUIRED system filesystem)

# Shared include directory
include_directories(include)

add_subdirectory(extra)
add_subdirectory(src)
add_subdirectory(tools)
add_subdirectory(unit-tests)