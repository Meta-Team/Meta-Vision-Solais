if (Protobuf_FOUND)
    protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS Parameters.proto)
    add_library(libParameters
            ${PROTO_SRCS} ${PROTO_HDRS})
    target_link_libraries(libParameters
            PUBLIC ${Protobuf_LIBRARIES})
    target_include_directories(libParameters
            PUBLIC ${Protobuf_INCLUDE_DIRS} ${CMAKE_CURRENT_BINARY_DIR}  # for Parameters.pc.h
            )
endif ()
