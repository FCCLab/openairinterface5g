add_executable(test_nr_ue_power_procedures
  test_nr_ue_power_procedures.cpp)
target_link_libraries(test_nr_ue_power_procedures PRIVATE NR_L2_UE asn1_nr_rrc_hdrs asn1_lte_rrc_hdrs GTest::gtest minimal_lib nr_common UTIL)
add_dependencies(tests test_nr_ue_power_procedures)
add_test(NAME test_nr_ue_power_procedures
  COMMAND ./test_nr_ue_power_procedures
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
