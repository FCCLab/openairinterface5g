add_library(LOG
    log.c)
target_include_directories(LOG PUBLIC .)
target_link_libraries(LOG PRIVATE ${T_LIB})
