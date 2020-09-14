include_directories(${CMAKE_SOURCE_DIR}/include)

add_executable(unit
  unit.c)

target_link_libraries(unit LibZ)
