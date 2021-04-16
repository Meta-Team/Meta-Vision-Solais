find_package(pugixml REQUIRED)

# FIXME: AnnotatedMat.cpp
add_library(libTerminalSocket TerminalSocket.cpp AnnotatedMat.cpp)
target_link_libraries(libTerminalSocket PUBLIC ${Boost_SYSTEM_LIBRARY} ${OpenCV_LIBRARIES})

add_library(libSolais
        ArmorDetector.cpp
        AnnotatedMat.cpp
        DetectorTuner.cpp
        Camera.cpp
        TerminalSocket.cpp
        Serial.cpp)
target_link_libraries(libSolais
        ${OpenCV_LIBRARIES}
        ${Boost_FILESYSTEM_LIBRARY}
        ${Boost_SYSTEM_LIBRARY}
        ${Protobuf_LIBRARIES}
        pugixml
        pthread
        libParameters
        libTerminalSocket)
target_include_directories(libSolais
        PUBLIC ${OpenCV_INCLUDE_DIRS}
        PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_compile_definitions(libSolais
        PUBLIC "$<$<CONFIG:DEBUG>:DEBUG>")