SET(TEST_FILES test_buffer.c test_dsp.c test_math.c test_io.c)
  

add_executable(run_tests EXCLUDE_FROM_ALL test_main.c test_common.c ${TEST_FILES})
add_custom_command(OUTPUT test_main.c 
  DEPENDS test_main.c.in util/gendriver.py ${TEST_FILES}
  COMMAND python3 util/gendriver.py
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/test
  COMMENT "Generating test_main.c"
  POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy 
  ${CMAKE_SOURCE_DIR}/test/test_main.c
  ${PROJECT_BINARY_DIR}/test/test_main.c)
target_link_libraries(run_tests hiemal)
target_include_directories(run_tests PRIVATE include ${PROJECT_SOURCE_DIR}/src/core/include)
add_custom_target(test COMMAND test/run_tests 
  WORKING_DIRECTORY ${PROJECT_BINARY_DIR} 
  DEPENDS run_tests
  COMMENT "Running tests")