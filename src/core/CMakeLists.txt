
target_include_directories(hiemal PUBLIC include)
target_sources(hiemal PRIVATE 
  fmt.c 
  error.c 
  buffer.c 
  dsp.c 
  math.c 
  async.c 
  signal_chain.c 
  io.c
  info.c
  logging.c
  device.c
  backend.c)

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
  "include/api/buffer.h"
  "include/api/recording.h")

set_target_properties(hiemal PROPERTIES PUBLIC_HEADER "${HIEMAL_PUBLIC_INCLUDE}")
install(TARGETS hiemal
  PUBLIC_HEADER
    DESTINATION "include/hiemal"
  RESOURCE
    DESTINATION "lib/pkgconfig")