cmake_minimum_required(VERSION 3.17)
project(Meta_Vision_Solais)

set(CMAKE_CXX_STANDARD 14)

set(QT_VERSION 5)
set(QT_LIBS Core Gui Widgets)
set(QT_LIBS_QUALIFIED Qt5::Core Qt5::Gui Qt5::Widgets)

if (NOT CMAKE_PREFIX_PATH)
    message(WARNING "CMAKE_PREFIX_PATH is not defined, you may need to set it "
            "(-DCMAKE_PREFIX_PATH=\"path/to/Qt/lib/cmake\" or -DCMAKE_PREFIX_PATH=/usr/include/{host}/qt{version}/ on Ubuntu)")
endif ()

find_package(Qt${QT_VERSION} COMPONENTS ${QT_LIBS} REQUIRED)

# Find OpenCV. Jetson Nano has OpenCV 4.1.1 installed. Here minimal version 4 is required.
find_package(OpenCV 4 REQUIRED)
message("Found OpenCV")
message("    Version: " ${OpenCV_VERSION})
message("    Libraries: " ${OpenCV_LIBRARIES})
message("    Includes: " ${OpenCV_INCLUDE_DIRS})

include_directories(include)

add_subdirectory(lib)
add_subdirectory(tools)