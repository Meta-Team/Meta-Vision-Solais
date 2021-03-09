find_package(pugixml REQUIRED)

add_library(ArmorDetector ArmorDetector.cpp AnnotatedMat.cpp DetectorTuner.cpp)
target_link_libraries(ArmorDetector ${OpenCV_LIBRARIES} pugixml)
target_include_directories(ArmorDetector PRIVATE ${OpenCV_INCLUDE_DIRS})
target_compile_definitions(ArmorDetector PUBLIC "$<$<CONFIG:DEBUG>:DEBUG>")