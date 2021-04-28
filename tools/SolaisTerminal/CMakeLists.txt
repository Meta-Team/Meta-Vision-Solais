if (CMAKE_PREFIX_PATH)
    set(QT_ENABLED 1)
    message("Qt is enabled")

    set(QT_VERSION 5)
    set(QT_LIBS Core Gui Widgets)
    # Qt5::CorePrivate and Qt5::GuiPrivate are in Core and Gui component
    set(QT_LIBS_QUALIFIED Qt5::Core Qt5::Gui Qt5::Widgets Qt5::CorePrivate Qt5::GuiPrivate)

    find_package(Qt${QT_VERSION} COMPONENTS ${QT_LIBS} REQUIRED)
else()
    message("CMAKE_PREFIX_PATH is not defined, and therefore Qt is disabled. If you want to use Qt, you may need to set it "
            "(-DCMAKE_PREFIX_PATH=\"path/to/Qt/lib/cmake\" or -DCMAKE_PREFIX_PATH=/usr/include/{host}/qt{version}/ on Ubuntu)")
endif ()

if (QT_ENABLED)
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTORCC ON)
    set(CMAKE_AUTOUIC ON)

    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/Parameters.ui.h
            COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/GeneratePhaseUI.py ${PROJECT_SOURCE_DIR}/extra/Parameters.proto > ${CMAKE_CURRENT_BINARY_DIR}/Parameters.ui.h
            MAIN_DEPENDENCY ${PROJECT_SOURCE_DIR}/extra/Parameters.proto
            DEPENDS GeneratePhaseUI.py)

    add_executable(SolarisTerminal
            main.cpp
            MainWindow.ui
            MainWindow.cpp
#            AnnotatedMat.cpp
#            AnnotatedMatViewer.ui
#            AnnotatedMatViewer.cpp
            QAsioEventDispatcher.cpp
            ${CMAKE_CURRENT_BINARY_DIR}/Parameters.ui.h)
    target_link_libraries(SolarisTerminal
            ${OpenCV_LIBRARIES}
            ${QT_LIBS_QUALIFIED}
            ${Boost_FILESYSTEM_LIBRARY}
            ${Boost_SYSTEM_LIBRARY}
            libParameters
            libTerminalSocket)
    target_include_directories(SolarisTerminal
            PRIVATE ${OpenCV_INCLUDE_DIRS} ${CMAKE_CURRENT_BINARY_DIR})
endif ()