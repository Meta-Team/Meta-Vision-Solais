if (QT_ENABLED)
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTORCC ON)
    set(CMAKE_AUTOUIC ON)

    add_executable(SolarisTerminal
            main.cpp
            MainWindow.ui
            MainWindow.cpp
            ValueUIBindings.hpp
            AnnotatedMatViewer.ui
            AnnotatedMatViewer.cpp)
    target_link_libraries(SolarisTerminal ${QT_LIBS_QUALIFIED} ArmorDetector ${OpenCV_LIBRARIES})
    target_include_directories(SolarisTerminal PRIVATE ${OpenCV_INCLUDE_DIRS})
endif ()