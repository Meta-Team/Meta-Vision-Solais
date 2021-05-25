cmake_minimum_required(VERSION 3.10)
project(Meta_Vision_Solais)

set(CMAKE_CXX_STANDARD 17)

# Find Boost
find_package(Boost COMPONENTS system filesystem json)
if (Boost_FOUND)
    string(REPLACE ";" ", " Boost_LIBRARIES_STRING "${Boost_LIBRARIES}")
    #    message("Boost ${Boost_VERSION} found: ${Boost_LIBRARIES_STRING}")
else ()
    message(WARNING "Boost not found so some targets may not be available")
endif ()

# Find OpenCV. Jetson Nano has OpenCV 4.1.1 installed. Here minimal version 4 is required.
find_package(OpenCV 4)
if (OpenCV_FOUND)
    #    message("OpenCV ${OpenCV_VERSION} found")
else ()
    message(WARNING "OpenCV not found so some targets may not be available")
endif ()

# Find Protocol Buffers
find_package(Protobuf)
if (Protobuf_FOUND)
    #    message("Protobuf ${Protobuf_VERSION} found")
else ()
    message(WARNING "Protobuf not found so some targets may not be available")
endif ()

# Find Qt
if (Qt5_DIR)
    set(QT_VERSION 5)
    set(QT_LIBS Core Gui Widgets)
    set(QT_LIBS_QUALIFIED Qt5::Core Qt5::Gui Qt5::Widgets)

    find_package(Qt${QT_VERSION} COMPONENTS ${QT_LIBS} REQUIRED)

    if (Qt5_FOUND)
        #        message("Qt${QT_VERSION} found")

        # Prioritize Qt5 package include directories

        include_directories(BEFORE "${Qt5_DIR}/../../../include")

        #        foreach (qt_lib ${QT_LIBS_QUALIFIED})
        #            get_target_property(qt_lib_includes ${qt_lib} INTERFACE_INCLUDE_DIRECTORIES)
        #            message("${qt_lib_includes}")
        #            include_directories(BEFORE "${qt_lib_includes}")
        #        endforeach ()
    endif ()
else ()
    message("Qt5_DIR is not defined, and therefore Qt is disabled. If you want to use Qt, you may need to set it "
            "(-DQt5_DIR=\"path/to/Qt/lib/cmake\" or -DQt5_DIR=/usr/include/{host}/qt{version}/ on Ubuntu)")
endif ()

# Shared include directory
include_directories(include)

add_subdirectory(extra)
add_subdirectory(src)
add_subdirectory(tools)
add_subdirectory(unit-tests)