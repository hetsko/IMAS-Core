# CMake configuration for the gRPC backend

# Find gRPC requirements
find_package( Protobuf REQUIRED )
find_package( gRPC REQUIRED )

# Find gRPC C++ plugin executable explicitly
find_program(GRPC_CPP_PLUGIN_EXECUTABLE grpc_cpp_plugin)
if(NOT GRPC_CPP_PLUGIN_EXECUTABLE)
    message(FATAL_ERROR "Could not find grpc_cpp_plugin. Is protobuf-compiler-grpc installed?")
endif()

# Protobuf proto definition file
set(PROTO_FILE data_transfer.proto)

# Generate gRPC source code using Protobuf compiler
set(PROTO_SRC "${CMAKE_CURRENT_BINARY_DIR}/data_transfer.pb.cc")
set(PROTO_HDR "${CMAKE_CURRENT_BINARY_DIR}/data_transfer.pb.h")
set(GRPC_SRC "${CMAKE_CURRENT_BINARY_DIR}/data_transfer.grpc.pb.cc")
set(GRPC_HDR "${CMAKE_CURRENT_BINARY_DIR}/data_transfer.grpc.pb.h")
add_custom_command(
    OUTPUT ${PROTO_SRC} ${PROTO_HDR} ${GRPC_SRC} ${GRPC_HDR}
    COMMAND ${Protobuf_PROTOC_EXECUTABLE}
    ARGS
        --grpc_out ${CMAKE_CURRENT_BINARY_DIR}
        --cpp_out ${CMAKE_CURRENT_BINARY_DIR}
        --plugin=protoc-gen-grpc=${GRPC_CPP_PLUGIN_EXECUTABLE}
        --proto_path ${CMAKE_CURRENT_SOURCE_DIR}
        ${PROTO_FILE}
    DEPENDS ${PROTO_FILE}
    VERBATIM
)
add_custom_target(al_grpc_codegen
    DEPENDS ${PROTO_SRC} ${PROTO_HDR} ${GRPC_SRC} ${GRPC_HDR}
)
add_dependencies(al al_grpc_codegen)

# Add backend source files
target_sources( al PRIVATE
    # ${PROTO_FILE}
    ${PROTO_SRC}
    ${GRPC_SRC}
    ${PROTO_HDR}
    ${GRPC_HDR}
    grpc_backend.cpp
)

# Include generated *.pb.h files
target_include_directories( al PRIVATE ${CMAKE_CURRENT_BINARY_DIR} )

target_compile_definitions( al PRIVATE -DGRPC )
target_include_directories( al PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} )
target_link_libraries( al PRIVATE gRPC::grpc++ protobuf::libprotobuf )
