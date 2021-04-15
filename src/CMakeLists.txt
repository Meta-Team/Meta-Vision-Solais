find_package(pugixml REQUIRED)
find_package(Boost REQUIRED system filesystem)

add_library(ArmorDetector
        ArmorDetector.cpp
        AnnotatedMat.cpp
        DetectorTuner.cpp
        Camera.cpp
        TerminalSocket.cpp
        Serial.cpp)
target_link_libraries(ArmorDetector ${OpenCV_LIBRARIES} ${Boost_FILESYSTEM_LIBRARY} ${Boost_SYSTEM_LIBRARY} pugixml pthread)
target_include_directories(ArmorDetector PRIVATE ${OpenCV_INCLUDE_DIRS})
target_compile_definitions(ArmorDetector PUBLIC "$<$<CONFIG:DEBUG>:DEBUG>")