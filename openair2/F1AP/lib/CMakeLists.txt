add_library(f1ap_lib OBJECT
            f1ap_lib_common.c
)
target_link_libraries(f1ap_lib PRIVATE asn1_f1ap)
target_include_directories(f1ap_lib PUBLIC
                             ../../..
                             ${CMAKE_CURRENT_SOURCE_DIR}
)
