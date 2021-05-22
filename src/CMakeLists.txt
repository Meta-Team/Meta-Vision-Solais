if (Boost_FOUND)
    add_library(libTerminalSocket TerminalSocket.cpp)
    target_link_libraries(libTerminalSocket PUBLIC ${Boost_SYSTEM_LIBRARY})
else()
    message(WARNING "=> Target libTerminalSocket is not available to build. Depends: Boost")
endif ()


if (OpenCV_FOUND)
    add_library(libArmorSolver ArmorSolver.cpp)
    target_link_libraries(libArmorSolver ${OpenCV_LIBRARIES})
else()
    message(WARNING "=> Target libArmorSolver is not available to build. Depends: OpenCV")
endif()

if (Boost_FOUND AND OpenCV_FOUND AND Protobuf_FOUND AND TARGET libParameters AND TARGET libTerminalSocket AND TARGET libArmorSolver)
    find_package(pugixml REQUIRED)
    add_library(libSolais
            ArmorDetector.cpp
            DataManager.cpp
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
            libTerminalSocket
            libArmorSolver)
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
    message(WARNING "=> Target libSolais is not available to build. Depends: Boost, OpenCV, Protobuf, libParameters, libTerminalSocket, libArmorSolver")
endif()