if (Boost_FOUND AND OpenCV_FOUND AND Protobuf_FOUND)
    message("Target Solaris available to build")
    add_executable(Solaris
            main.cpp)
    target_link_libraries(Solaris
            PRIVATE libSolais libParameters)
else()
    message(WARNING "Target Solaris not available to build")
endif()