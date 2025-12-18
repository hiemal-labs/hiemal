
target_include_directories(hiemal PUBLIC include)
target_sources(hiemal PRIVATE 
  fmt.c
  error.c
  backend.c
  buffer.c
  dsp.c
  math.c
  io.c
  info.c
  logging.c
  device.c
  backend.c
  common.c
  thread.c
  stream.c)

add_subdirectory(ops)

if(UNIX)
  target_sources(hiemal PRIVATE
    event_unix.c
    semaphore_unix.c
    thread_unix.c)
endif()

if(WITH_ALSA)
  target_sources(hiemal PRIVATE alsa.c)
endif()

if(WITH_PULSEAUDIO)
  target_sources(hiemal PRIVATE pulse.c)
endif()

if(WITH_PROTOBUF)
  target_sources(hiemal PRIVATE recording.c)
endif()

set(HIEMAL_PUBLIC_INCLUDE
  "include/api/backend.h"
  "include/api/buffer.h"
  "include/api/device.h"
  "include/api/recording.h"
  "include/api/stream.h"
  "${CMAKE_BINARY_DIR}/src/core/ops/ops.h")

set_target_properties(hiemal PROPERTIES PUBLIC_HEADER "${HIEMAL_PUBLIC_INCLUDE}")
install(TARGETS hiemal
  PUBLIC_HEADER
    DESTINATION "include/hiemal"
  RESOURCE
    DESTINATION "lib/pkgconfig")