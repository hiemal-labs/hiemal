add_library(hmutil STATIC)
target_include_directories(hmutil PUBLIC ../core/include)
target_link_libraries(hiemal hmutil)
target_sources(hmutil PRIVATE 
  tty.c)