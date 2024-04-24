set(Test_Labels fapi p7)
#Pack DCI test
add_executable(nr_fapi_dci_inversion_test nr_fapi_dci_inversion_test.c)
target_link_libraries(nr_fapi_dci_inversion_test PUBLIC nr_fapi_p5)
target_link_libraries(nr_fapi_dci_inversion_test PRIVATE pthread UTIL minimal_lib)
target_compile_definitions(nr_fapi_dci_inversion_test PRIVATE TEST="nr_fapi_test.h")
add_dependencies(tests nr_fapi_dci_inversion_test)

set(dci_labels dci ${Test_Labels})
add_test(NAME nr_fapi_dci_inversion_test
        COMMAND nr_fapi_dci_inversion_test ${CMAKE_CURRENT_BINARY_DIR}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
set_tests_properties(nr_fapi_dci_inversion_test PROPERTIES LABELS "${dci_labels}")
