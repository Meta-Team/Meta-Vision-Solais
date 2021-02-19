add_library(ArmorDetector ArmorDetector.cpp)
target_link_libraries(ArmorDetector ${OpenCV_LIBRARIES})
target_include_directories(ArmorDetector PRIVATE ${OpenCV_INCLUDE_DIRS})