find_package(pugixml REQUIRED)

add_library(libTerminalSocket TerminalSocket.cpp)
target_link_libraries(libTerminalSocket PUBLIC ${Boost_SYSTEM_LIBRARY})

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
target_compile_definitions(libSolais
        PUBLIC "$<$<CONFIG:DEBUG>:DEBUG>")