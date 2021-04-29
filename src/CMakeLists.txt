if (Boost_FOUND)
    add_library(libTerminalSocket TerminalSocket.cpp)
    target_link_libraries(libTerminalSocket PUBLIC ${Boost_SYSTEM_LIBRARY})
endif ()

if (Boost_FOUND AND OpenCV_FOUND AND Protobuf_FOUND)
    find_package(pugixml REQUIRED)
    add_library(libSolais
            ArmorDetector.cpp
            ImageDataManager.cpp
            Camera.cpp
            Serial.cpp
            Executor.cpp)
    target_link_libraries(libSolais
            ${OpenCV_LIBRARIES}
            ${Boost_FILESYSTEM_LIBRARY}
            ${Boost_SYSTEM_LIBRARY}
            ${Protobuf_LIBRARIES}
            pugixml
            pthread
            libParameters
            libTerminalSocket)
    target_compile_definitions(libSolais PUBLIC "$<$<CONFIG:DEBUG>:DEBUG>")

    if(NOT PARAMS_ROOT)
        set(PARAMS_ROOT "${PROJECT_SOURCE_DIR}/data")
        message("Directory of parameter sets is default as \"${PARAMS_ROOT}\"")
    else()
        message("Directory of parameter sets is set as \"${PARAMS_ROOT}\"")
    endif ()

    if(NOT DATA_SET_ROOT)
        set(DATA_SET_ROOT "${PROJECT_SOURCE_DIR}/data")
        message("Directory of data sets is default as \"${DATA_SET_ROOT}\"")
    else()
        message("Directory of data sets is set as \"${DATA_SET_ROOT}\"")
    endif ()

    target_compile_definitions(libSolais
            PUBLIC "PARAMS_ROOT=\"${PARAMS_ROOT}\""
            PUBLIC "DATA_SET_ROOT=\"${DATA_SET_ROOT}\"")
endif()