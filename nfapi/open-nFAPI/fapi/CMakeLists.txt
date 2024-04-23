add_library(nr_fapi_p5
        src/nr_fapi_p5.c
)
target_include_directories(nr_fapi_p5 PUBLIC inc)
target_link_libraries(nr_fapi_p5 PUBLIC nfapi_common NFAPI_LIB NFAPI_USER_LIB)
