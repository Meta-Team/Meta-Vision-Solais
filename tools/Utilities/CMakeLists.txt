if (OpenCV_FOUND)
    message("Targets OpenCV utilities available to build")

    add_executable(CameraCalibration CameraCalibration.cpp)
    target_link_libraries(CameraCalibration ${OpenCV_LIBRARIES})

    add_executable(ExtractFrameFromVideo ExtractFrameFromVideo.cpp)
    target_link_libraries(ExtractFrameFromVideo ${OpenCV_LIBRARIES})

    add_executable(TakePhotoUsingCamera TakePhotoUsingCamera.cpp)
    target_link_libraries(TakePhotoUsingCamera ${OpenCV_LIBRARIES})
else()
    message(WARNING "Targets OpenCV utilities not available to build")
endif ()