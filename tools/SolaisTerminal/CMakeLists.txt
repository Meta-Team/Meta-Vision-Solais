if (QT_ENABLED)
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTORCC ON)
    set(CMAKE_AUTOUIC ON)

    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/Parameters.ui.h
            COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/GeneratePhaseUI.py ${PROJECT_SOURCE_DIR}/extra/Parameters.proto > ${CMAKE_CURRENT_BINARY_DIR}/Parameters.ui.h
            MAIN_DEPENDENCY ${PROJECT_SOURCE_DIR}/extra/Parameters.proto)

    add_executable(SolarisTerminal
            main.cpp
            MainWindow.ui
            MainWindow.cpp
            ValueUIBindings.hpp
            AnnotatedMatViewer.ui
            AnnotatedMatViewer.cpp
            ${CMAKE_CURRENT_BINARY_DIR}/Parameters.ui.h)
    target_link_libraries(SolarisTerminal
            ${QT_LIBS_QUALIFIED}
            ${Boost_FILESYSTEM_LIBRARY}
            ${Boost_SYSTEM_LIBRARY}
            libParameters
            libTerminalSocket)
    target_include_directories(SolarisTerminal
            PRIVATE ${OpenCV_INCLUDE_DIRS} ${CMAKE_CURRENT_BINARY_DIR})
endif ()