SET(TEST_FILES test_buffer.c test_dsp.c test_math.c test_io.c test_async.c)
if(WITH_PROTOBUF)
  list(APPEND TEST_FILES test_pb_c_wrap.c)
endif()

set(GENDRIVER_EXCLUDED_FILES test_main.c test_common.c)

# skip protobuf-related tests
if(NOT WITH_PROTOBUF)
  list(APPEND GENDRIVER_EXCLUDED_FILES test_pb_c_wrap.c)
endif()

add_executable(run_tests EXCLUDE_FROM_ALL test_main.c test_common.c ${TEST_FILES})
add_custom_command(OUTPUT test_main.c 
  DEPENDS test_main.c.in util/gendriver.py ${TEST_FILES}
  COMMAND python3 util/gendriver.py --exclude_files ${GENDRIVER_EXCLUDED_FILES}
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/test
  COMMENT "Generating test_main.c"
  POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy 
  ${CMAKE_SOURCE_DIR}/test/test_main.c
  ${PROJECT_BINARY_DIR}/test/test_main.c)
target_link_libraries(run_tests hiemal)
target_include_directories(run_tests PRIVATE include ${PROJECT_SOURCE_DIR}/src/core/include ${CMAKE_BINARY_DIR}/include)
if(WITH_PROTOBUF)
  target_include_directories(run_tests PRIVATE ${PROJECT_SOURCE_DIR}/src/protobuf/_c_wrap)
endif()
add_custom_target(test COMMAND test/run_tests 
  WORKING_DIRECTORY ${PROJECT_BINARY_DIR} 
  DEPENDS run_tests
  COMMENT "Running tests")