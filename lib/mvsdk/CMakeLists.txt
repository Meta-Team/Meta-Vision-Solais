add_library(MVSDK INTERFACE)
target_include_directories(MVSDK INTERFACE include)

if (${CMAKE_HOST_SYSTEM_NAME} MATCHES "Darwin")
    set(MVSDK_LIB ${CMAKE_CURRENT_SOURCE_DIR}/lib/macos/libmvsdk.dylib)

    # This is a kind of hacking. libmvsdk.dylib hard codes the dynamic linking path as
    # @executable_path/../Frameworks/libmvsdk.dylib (run otool -L libmvsdk.dylib to see)
    # Therefore, copy libmvsdk.dylib to match the path
    configure_file(${MVSDK_LIB} ${CMAKE_BINARY_DIR}/Frameworks/libmvsdk.dylib COPYONLY)
    configure_file(${MVSDK_LIB} ${CMAKE_BINARY_DIR}/tools/Frameworks/libmvsdk.dylib COPYONLY)

elseif (${CMAKE_HOST_SYSTEM_NAME} MATCHES "(x86)|(X86)|(i386)|(i686)")
    set(MVSDK_LIB ${CMAKE_CURRENT_SOURCE_DIR}/lib/x86/libmvsdk.dylib)

elseif (${CMAKE_HOST_SYSTEM_NAME} MATCHES "(x86_64)|(x64)")
    set(MVSDK_LIB ${CMAKE_CURRENT_SOURCE_DIR}/lib/x64/libmvsdk.dylib)

elseif (${CMAKE_HOST_SYSTEM_NAME} MATCHES "(aarch64)|(aarch64_be)|(arm64)")
    set(MVSDK_LIB ${CMAKE_CURRENT_SOURCE_DIR}/lib/arm64/libmvsdk.dylib)

elseif (${CMAKE_HOST_SYSTEM_NAME} MATCHES "(arm)")
    set(MVSDK_LIB ${CMAKE_CURRENT_SOURCE_DIR}/lib/arm/libmvsdk.dylib)

else ()
    message(FATAL_ERROR "MVSDK doesn't support host system \"${CMAKE_HOST_SYSTEM_NAME}\".")
endif ()

target_link_libraries(MVSDK INTERFACE ${MVSDK_LIB})