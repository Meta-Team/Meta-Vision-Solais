add_library(ArmorDetector ArmorDetector.cpp AnnotatedMat.cpp)
target_link_libraries(ArmorDetector ${OpenCV_LIBRARIES})
target_include_directories(ArmorDetector PRIVATE ${OpenCV_INCLUDE_DIRS})
target_compile_definitions(ArmorDetector PUBLIC "$<$<CONFIG:DEBUG>:DEBUG>")