add_library(hm_python_wrapper MODULE)
set_target_properties(hm_python_wrapper PROPERTIES PREFIX "")
set_target_properties(hm_python_wrapper PROPERTIES OUTPUT_NAME "_c")
target_include_directories(hm_python_wrapper PRIVATE
  ${Python_INCLUDE_DIRS}
  ${Python_NumPy_INCLUDE_DIRS}
  ${CMAKE_SOURCE_DIR}/src/core/include
  ${CMAKE_BINARY_DIR}/include)
target_sources(hm_python_wrapper PRIVATE
  src/format.c
  src/module.c
  src/buffer.c
  src/backend.c
  src/device.c)
target_link_libraries(hm_python_wrapper PRIVATE hiemal)