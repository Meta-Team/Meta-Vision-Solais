add_executable(Solaris
        main.cpp)
target_link_libraries(Solaris
        PRIVATE libSolais)
target_include_directories(Solaris
        PRIVATE ${OpenCV_INCLUDE_DIRS})