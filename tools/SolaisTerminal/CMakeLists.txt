set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

add_executable(SolarisTerminal
        main.cpp
        MainWindow.ui
        MainWindow.cpp)
target_link_libraries(SolarisTerminal ${QT_LIBS_QUALIFIED})