add_library(nr_common nr_common.c)
target_include_directories(nr_common PUBLIC .)
target_link_libraries(nr_common PRIVATE UTIL)
if (ENABLE_TESTS)
  add_subdirectory(tests)
endif()
