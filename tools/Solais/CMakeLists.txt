if (TARGET libSolais AND TARGET libParameters)
#    message("=> Target Solais is available to build")
    add_executable(Solais
            main.cpp)
    target_link_libraries(Solais
            PRIVATE libSolais libParameters)
else()
    message("=> Target Solais is not available to build. Depends: libSolais, libParameters")
endif()