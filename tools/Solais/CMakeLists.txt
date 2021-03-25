add_executable(Solaris
        main.cpp)
target_link_libraries(Solaris ArmorDetector ${OpenCV_LIBRARIES})
target_include_directories(Solaris PRIVATE ${OpenCV_INCLUDE_DIRS})