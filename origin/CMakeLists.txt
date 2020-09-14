include_directories(${CMAKE_SOURCE_DIR}/include)

add_executable(origin
  origin.c)
target_link_libraries(origin LibZ)
