add_library(oml src/oml.c)
target_include_directories(oml PUBLIC inc)
target_link_libraries(oml PRIVATE LOG)
