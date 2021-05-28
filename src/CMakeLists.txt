if (Boost_FOUND)
    add_library(libTerminalSocket TerminalSocket.cpp)
    target_link_libraries(libTerminalSocket PUBLIC ${Boost_SYSTEM_LIBRARY})
else()
    message(WARNING "=> Target libTerminalSocket is not available to build. Depends: Boost")
endif ()

if (Boost_FOUND)
    add_library(libSerial Serial.cpp CRC.cpp)
    target_link_libraries(libSerial PUBLIC ${Boost_SYSTEM_LIBRARY})
else()
    message(WARNING "=> Target libSerial is not available to build. Depends: Boost")
endif ()

if (OpenCV_FOUND)
    add_library(libArmorSolver ArmorSolver.cpp)
    target_link_libraries(libArmorSolver ${OpenCV_LIBRARIES})
else()
    message(WARNING "=> Target libArmorSolver is not available to build. Depends: OpenCV")
endif()

#find_package(PkgConfig REQUIRED)
#set(GSTREAMER_MINIMUM_VERSION 1.0.5)
#pkg_check_modules(GST1_TEST gstreamer-1.0)
#if ( GST1_TEST_FOUND AND NOT ${GST1_TEST_VERSION} VERSION_LESS ${GSTREAMER_MINIMUM_VERSION} )
#    pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0)
#    pkg_check_modules(GSTREAMER-APP REQUIRED gstreamer-app-1.0)
#    pkg_check_modules(GSTREAMER-AUDIO REQUIRED gstreamer-audio-1.0)
#    pkg_check_modules(GSTREAMER-PBUTILS REQUIRED gstreamer-pbutils-1.0)
#    pkg_check_modules(GSTREAMER-FFT REQUIRED gstreamer-fft-1.0)
#    add_definitions(-DGST_API_VERSION_1=1)
#else()
#    # fallback to gstreamer-1.0
#    unset(GSTREAMER_MINIMUM_VERSION)
#    pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0)
#    pkg_check_modules(GSTREAMER-APP REQUIRED gstreamer-app-1.0)
#    pkg_check_modules(GSTREAMER-AUDIO REQUIRED gstreamer-audio-1.0)
#    pkg_check_modules(GSTREAMER-FFT REQUIRED gstreamer-fft-1.0)
#    set_source_files_properties(WebKitWebAudioSourceGStreamer.cpp PROPERTIES COMPILE_DEFINITIONS "GLIB_DISABLE_DEPRECATION_WARNINGS=1")
#endif()
#
#include_directories(
#        ${GSTREAMER_INCLUDE_DIRS}
#        ${GSTREAMER-APP_INCLUDE_DIRS}
#        ${GSTREAMER-AUDIO_INCLUDE_DIRS}
#        ${GSTREAMER-PBUTILS_INCLUDE_DIRS}
#        ${GSTREAMER-FFT_INCLUDE_DIRS}
#)
#link_directories(
#        ${GSTREAMER_LIBRARY_DIRS}
#        ${GSTREAMER-APP_LIBRARY_DIRS}
#        ${GSTREAMER-AUDIO_LIBRARY_DIRS}
#        ${GSTREAMER-PBUTILS_LIBRARY_DIRS}
#        ${GSTREAMER-FFT_LIBRARY_DIRS}
#)


if (Boost_FOUND AND OpenCV_FOUND AND Protobuf_FOUND AND pugixml_FOUND AND TARGET libParameters AND TARGET libTerminalSocket AND TARGET libArmorSolver AND TARGET libSerial)
    add_library(libSolais
            ArmorDetector.cpp
            ArmorSelector.cpp
            AimingCalculator.cpp
            ParamSetManager.cpp
            Camera.cpp
            ImageSet.cpp
            VideoSource.cpp
            Executor.cpp)
    target_link_libraries(libSolais
            ${OpenCV_LIBRARIES}
            ${Boost_FILESYSTEM_LIBRARY}
            ${Boost_SYSTEM_LIBRARY}
            ${Protobuf_LIBRARIES}
            pugixml
            pthread
            libParameters
            libTerminalSocket
            libSerial
            libArmorSolver
#            ${GSTREAMER_LIBRARIES}
#            ${GSTREAMER-APP_LIBRARIES}
#            ${GSTREAMER-AUDIO_LIBRARIES}
#            ${GSTREAMER-PBUTILS_LIBRARIES}
#            ${GSTREAMER-FFT_LIBRARIES}
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
    message(WARNING "=> Target libSolais is not available to build. Depends: Boost, OpenCV, Protobuf, libParameters, libTerminalSocket, libSerial, libArmorSolver")
endif()