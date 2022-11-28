#create lists for protobuf output files 
#(https://stackoverflow.com/a/56604559)
set (PROTO_FILES
  buffer.proto
)

string(REGEX REPLACE "[.]proto$" ".pb.cc" PB_OUTPUT_SOURCES ${PROTO_FILES})
string(REGEX REPLACE "[.]proto$" ".pb.h" PB_OUTPUT_HEADERS ${PROTO_FILES})

list(TRANSFORM PB_OUTPUT_SOURCES PREPEND ${CMAKE_CURRENT_BINARY_DIR}/ OUTPUT_VARIABLE PB_SRC_DEP)
list(TRANSFORM PB_OUTPUT_HEADERS PREPEND ${CMAKE_CURRENT_BINARY_DIR}/ OUTPUT_VARIABLE PB_INC_DEP)

add_custom_command(OUTPUT ${PB_SRC_DEP} ${PB_INC_DEP}
  COMMAND ${Protobuf_PROTOC_EXECUTABLE} -I ${CMAKE_CURRENT_SOURCE_DIR} --cpp_out=. ${PROTO_FILES}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMENT "Build protobuf files")
add_custom_target(pb DEPENDS ${PB_SRC_DEP} ${PB_INC_DEP})
add_dependencies(hiemal pb)
target_include_directories(hiemal PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_include_directories(hiemal PRIVATE ${Protobuf_INCLUDE_DIRS})
target_sources(hiemal PRIVATE ${PB_SRC_DEP})
target_link_libraries(hiemal ${PROTOBUF_LIB})
target_sources(hiemal PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/_c_wrap/hm_protobuf.cpp)