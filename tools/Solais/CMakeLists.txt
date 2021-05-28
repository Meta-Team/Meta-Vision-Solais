if (TARGET libSolais AND TARGET libParameters)
#    message("=> Target Solaris is available to build")
    add_executable(Solaris
            main.cpp)
    target_link_libraries(Solaris
            PRIVATE libSolais libParameters)
else()
    message("=> Target Solaris is not available to build. Depends: libSolais, libParameters")
endif()