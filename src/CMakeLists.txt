# Parameters and results ProtoBuf
if (Protobuf_FOUND)
    protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS Parameters.proto)
    add_library(libParameters
            ${PROTO_SRCS} ${PROTO_HDRS})
    target_link_libraries(libParameters
            PUBLIC ${Protobuf_LIBRARIES})
    target_include_directories(libParameters
            PUBLIC ${Protobuf_INCLUDE_DIRS} ${CMAKE_CURRENT_BINARY_DIR}  # for Parameters.pc.h
            )
else()
    message("=> Target libParameters is not available to build. Depends: Protobuf")
endif ()

# libTerminalSocket
if (Boost_FOUND)
    add_library(libTerminalSocket TerminalSocket.cpp)
    target_link_libraries(libTerminalSocket PUBLIC ${Boost_SYSTEM_LIBRARY})
else()
    message("=> Target libTerminalSocket is not available to build. Depends: Boost")
endif ()

# libSerial
if (Boost_FOUND)
    if(DEFINED SERIAL_DEVICE)
        message("Serial device is set as \"${SERIAL_DEVICE}\"")
    else()
        set(SERIAL_DEVICE "/dev/ttyTHS1")
        message("Use default serial device \"${SERIAL_DEVICE}\"")
    endif ()

    add_library(libSerial Serial.cpp CRC.cpp)
    target_compile_definitions(libSerial
            PUBLIC "SERIAL_DEVICE=\"${SERIAL_DEVICE}\"")
    target_link_libraries(libSerial PUBLIC ${Boost_SYSTEM_LIBRARY})
else()
    message("=> Target libSerial is not available to build. Depends: Boost")
endif ()

# libArmorSolver
if (OpenCV_FOUND)
    add_library(libArmorSolver PositionCalculator.cpp)
    target_link_libraries(libArmorSolver ${OpenCV_LIBRARIES})
else()
    message("=> Target libArmorSolver is not available to build. Depends: OpenCV")
endif()

# libCamera
if (OpenCV_FOUND AND TARGET libParameters)
    add_library(libCamera Camera.cpp OpenCVCamera.cpp MVCamera.cpp)
    target_link_libraries(libCamera ${OpenCV_LIBRARIES} libParameters MVSDK)
    if (GSTREAMER_FOUND)
        target_compile_definitions(libParameters PUBLIC "GSTREAMER_FOUND=1")
        message("GStreamer found")
    else()
        message("GStreamer not found")
    endif()
else()
    message("=> Target libCamera is not available to build. Depends: OpenCV, libParameters, MVSDK")
endif()

# libGStreamerCamera
#if (OpenCV_FOUND AND TARGET libParameters)
#    add_library(libGStreamerCamera GStreamerCamera.cpp)
#    target_link_libraries(libGStreamerCamera
#            ${GSTREAMER_LIBRARIES}
#            ${GSTREAMER-APP_LIBRARIES}
#            ${GSTREAMER-AUDIO_LIBRARIES}
#            ${GSTREAMER-PBUTILS_LIBRARIES}
#            ${GSTREAMER-FFT_LIBRARIES}
#            ${OpenCV_LIBRARIES}
#            libParameters
#            )
#else()
#    message("=> Target libGStreamerCamera is not available to build. Depends: GStreamer, OpenCV, libParameters")
#endif ()

# libSolais
if (Boost_FOUND AND OpenCV_FOUND AND Protobuf_FOUND AND TARGET libParameters AND TARGET libTerminalSocket AND TARGET libArmorSolver AND TARGET libSerial)
    add_library(libSolais
            ArmorDetector.cpp
            AimingSolver.cpp
            ParamSetManager.cpp
            ImageSet.cpp
            VideoSet.cpp
            Executor.cpp)
    target_link_libraries(libSolais
            ${OpenCV_LIBRARIES}
            ${Boost_FILESYSTEM_LIBRARY}
            ${Boost_SYSTEM_LIBRARY}
            ${Protobuf_LIBRARIES}
            pthread
            libParameters
            libTerminalSocket
            libSerial
            libArmorSolver
            libCamera
            )
    target_compile_definitions(libSolais PUBLIC "$<$<CONFIG:DEBUG>:DEBUG>")

    if(NOT PARAM_SET_ROOT)
        set(PARAM_SET_ROOT "${PROJECT_SOURCE_DIR}/data")
        message("Directory of parameter sets is default as \"${PARAM_SET_ROOT}\"")
    else()
        message("Directory of parameter sets is set as \"${PARAM_SET_ROOT}\"")
    endif ()

    if(NOT DATA_SET_ROOT)
        set(DATA_SET_ROOT "${PROJECT_SOURCE_DIR}/data")
        message("Directory of data sets is default as \"${DATA_SET_ROOT}\"")
    else()
        message("Directory of data sets is set as \"${DATA_SET_ROOT}\"")
    endif ()

    target_compile_definitions(libSolais
            PUBLIC "PARAM_SET_ROOT=\"${PARAM_SET_ROOT}\""
            PUBLIC "DATA_SET_ROOT=\"${DATA_SET_ROOT}\"")
else()
    message("=> Target libSolais is not available to build. Depends: Boost, OpenCV, Protobuf, libParameters, libTerminalSocket, libSerial, libArmorSolver")
endif()