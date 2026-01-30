cmake_minimum_required(VERSION 3.22)

# check if we are inside of a git repository
execute_process(COMMAND "git" "status"
  RESULT_VARIABLE GIT_STATUS_RET OUTPUT_QUIET ERROR_QUIET)

project(HIEMAL VERSION 0.1.0)

if(${CMAKE_C_COMPILER_ID} MATCHES "GNU")
add_compile_options($<$<COMPILE_LANGUAGE:C,CXX>:-ffile-prefix-map=${CMAKE_SOURCE_DIR}/=>)
endif()

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/Modules ${CMAKE_MODULE_PATH})

if (GIT_STATUS_RET EQUAL "0")
  # get git commit hash to put into the version string
  execute_process(COMMAND "git" "rev-parse" "--short" "HEAD" 
    OUTPUT_VARIABLE HIEMAL_GIT_COMMIT 
    OUTPUT_STRIP_TRAILING_WHITESPACE)
  execute_process(COMMAND "git" "rev-parse" "HEAD" 
    OUTPUT_VARIABLE HIEMAL_GIT_COMMIT_LONG
    OUTPUT_STRIP_TRAILING_WHITESPACE)
  set(HIEMAL_VERSION_STR "${HIEMAL_VERSION_MAJOR}.${HIEMAL_VERSION_MINOR}.${HIEMAL_VERSION_PATCH}+${HIEMAL_GIT_COMMIT}")
else()
set(HIEMAL_VERSION_STR "${HIEMAL_VERSION_MAJOR}.${HIEMAL_VERSION_MINOR}.${HIEMAL_VERSION_PATCH}")
endif()


# command for running ops codegen script
set(OP_FILES src/core/ops/dsp/filter.c
  src/core/ops/source/io_device.c
  src/core/ops/source/file.c
  src/core/ops/sink/io_device.c
  src/core/ops/sink/file.c
  )
add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/src/core/ops/ops.h
  ${CMAKE_BINARY_DIR}/src/core/ops/ops_internal.h
  ${CMAKE_BINARY_DIR}/src/core/ops/ops.c
  DEPENDS src/core/ops/util/gen_ops.py
    src/core/ops/ops.json
    ${OP_FILES}
  COMMAND python3 util/gen_ops.py -o ${CMAKE_BINARY_DIR}/src/core/ops -t ${CMAKE_SOURCE_DIR}/src/core/ops
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/src/core/ops
  COMMENT "generating ops")

add_library(hiemal SHARED)
target_include_directories(hiemal PRIVATE ${CMAKE_BINARY_DIR}/include)
target_link_libraries(hiemal m)
target_sources(hiemal PRIVATE ${OP_FILES})
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  set(WITH_ALSA "OFF" CACHE BOOL "Use ALSA")  
  find_package(ALSA)
  if(NOT ALSA_FOUND)
    set(WITH_ALSA "OFF")
  endif()
endif()

set(WITH_PYTHON "ON" CACHE BOOL "Build python module")
find_package (Python 3.10 COMPONENTS Interpreter Development NumPy)
if(NOT Python_FOUND)
  set(WITH_PYTHON "OFF")
endif()

if (WITH_ALSA)
  target_link_libraries(hiemal asound)
endif()

set(WITH_PROTOBUF "OFF" CACHE BOOL "Build protocol buffers")  
find_package (Protobuf)
if(NOT Protobuf_FOUND)
  set(WITH_PROTOBUF "OFF")
endif()

set(WITH_PULSEAUDIO "OFF" CACHE BOOL "Use PulseAudio")  
find_package (PulseAudio)
if(NOT PulseAudio_FOUND)
  set(WITH_PULSEAUDIO "OFF")
endif()

set(TEST_MTRACE "OFF" CACHE BOOL "Run tests with mtrace enabled")  

set(GDBINIT_FIX "OFF" CACHE BOOL "Create .gdbinit file in build directory to fix \
debug symbol path issues caused by -ffile-prefix-map")

set(PYTEST "ON" CACHE BOOL "Include python tests")
if(NOT Python_Development_FOUND OR NOT WITH_PYTHON)
  set(PYTEST "OFF")
endif()

if (WITH_PULSEAUDIO)
  target_link_libraries(hiemal pulse)
endif()

# check whether or not static or dynamic libraries were found, 
# prefer static if both are present
if(WITH_PROTOBUF)
  if(EXISTS ${Protobuf_LIBRARIES}/libprotobuf.a)
    set(LIBPROTOBUF_TYPE "static")
    set(PROTOBUF_LIB ${Protobuf_LIBRARIES}/libprotobuf.a)
  elseif(EXISTS ${Protobuf_LIBRARIES}/libprotobuf.so)
    set(LIBPROTOBUF_TYPE "shared")
    set(PROTOBUF_LIB ${Protobuf_LIBRARIES}/libprotobuf.so)
  else()
    set(LIBPROTOBUF_TYPE "unknown")
  endif()
endif()

set(WITH_UTILS "OFF" CACHE BOOL "Build hiemal utilities")

set(WITH_ENVVARS "ON" CACHE BOOL "Enable environment variables")

set(RESOURCE_FILES ${CMAKE_BINARY_DIR}/pkgconfig/hiemal.pc)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/include)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/pkgconfig)
configure_file(cmake/cmake_info.h.in include/cmake_info.h @ONLY)
configure_file(cmake/pkgconfig/hiemal.pc.in pkgconfig/hiemal.pc @ONLY)
if(GDBINIT_FIX)
configure_file(cmake/gdbinit.in .gdbinit @ONLY)
endif()
set_target_properties(hiemal PROPERTIES
  RESOURCE "${RESOURCE_FILES}")

add_subdirectory(src/core)
add_subdirectory(src/util)
if(WITH_PYTHON)
  add_subdirectory(src/python)
endif()
if(WITH_UTILS)
  add_subdirectory(src/utils)
endif()
if(WITH_PROTOBUF)
  add_subdirectory(src/protobuf)
endif()
add_subdirectory(docs)
add_subdirectory(test)



# config summary
message(STATUS "")
message(STATUS "================== Configuration Summary ==================")
message(STATUS "General:")
message(STATUS "  Hiemal version: ${HIEMAL_VERSION}")
message(STATUS "  CMake version: ${CMAKE_VERSION}")
message(STATUS "  Platform: ${CMAKE_SYSTEM_NAME} ${CMAKE_SYSTEM_VERSION}")
if (GIT_STATUS_RET EQUAL "0")
  message(STATUS "  Found git repository: YES")
  message(STATUS "    Git commit: ${HIEMAL_GIT_COMMIT}")
else()
  message(STATUS "  Found git repository: NO")
endif()
message(STATUS "  Install location: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "")
if(WITH_UTILS)
  message(STATUS "Utils: YES")
else()
  message(STATUS "Utils: NO")
endif()
message(STATUS "")
if(NOT WITH_PROTOBUF)
  message(STATUS "Protobuf: NO")
else()
  message(STATUS "Protobuf: YES")
  message(STATUS "  libprotobuf: ${LIBPROTOBUF_TYPE}")
endif()
message(STATUS "")
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  message(STATUS "Linux-specific options:")
  if(WITH_ALSA)
    message(STATUS "  ALSA: YES")
  else()
    message(STATUS "  ALSA: NO")
  endif()
  if(WITH_PULSEAUDIO)
    message(STATUS "  PulseAudio: YES (${PulseAudio_LIBRARIES})")
  else()
    message(STATUS "  PulseAudio: NO")
  endif()
  message(STATUS "")
endif()
if (NOT WITH_PYTHON)
  message(STATUS "Python: NO")
else()
  message(STATUS "Python: YES")
  message(STATUS "  Interpreter: ${Python_EXECUTABLE}")
  message(STATUS "  Include Dir: ${Python_INCLUDE_DIRS}")
  message(STATUS "  Library: ${Python_LIBRARIES}")
endif()
message(STATUS "")
message(STATUS "Documentation:")
if (SPHINX_COMMIT_INFO)
  message(STATUS "  Sphinx add commit info to footer: YES")
else()
  message(STATUS "  Sphinx add commit info to footer: NO")
endif()
message(STATUS "")
message(STATUS "Tests")
if (TEST_MTRACE)
  message(STATUS "  Run tests with mtrace: YES")
else()
  message(STATUS "  Run tests with mtrace: NO")
endif()
if (NOT PYTEST)
  message(STATUS "  Python tests: NO")
else()
  message(STATUS "  Python tests: YES")
endif()
message(STATUS "")