set(UE_ID_COMMON_E2SM_SRCS
  ../../../flexric/src/sm/rc_sm/ie/asn/AMFPointer.c
  ../../../flexric/src/sm/rc_sm/ie/asn/AMFRegionID.c
  ../../../flexric/src/sm/rc_sm/ie/asn/AMFSetID.c
  ../../../flexric/src/sm/rc_sm/ie/asn/AMF-UE-NGAP-ID.c
  ../../../flexric/src/sm/rc_sm/ie/asn/ENB-ID.c
  ../../../flexric/src/sm/rc_sm/ie/asn/ENB-UE-X2AP-ID-Extension.c
  ../../../flexric/src/sm/rc_sm/ie/asn/ENB-UE-X2AP-ID.c
  ../../../flexric/src/sm/rc_sm/ie/asn/GlobalENB-ID.c
  ../../../flexric/src/sm/rc_sm/ie/asn/GlobalGNB-ID.c
  ../../../flexric/src/sm/rc_sm/ie/asn/GlobalNgENB-ID.c
  ../../../flexric/src/sm/rc_sm/ie/asn/GlobalNGRANNodeID.c
  ../../../flexric/src/sm/rc_sm/ie/asn/GNB-CU-CP-UE-E1AP-ID.c
  ../../../flexric/src/sm/rc_sm/ie/asn/GNB-CU-UE-F1AP-ID.c
  ../../../flexric/src/sm/rc_sm/ie/asn/GNB-ID.c
  ../../../flexric/src/sm/rc_sm/ie/asn/GUAMI.c
  ../../../flexric/src/sm/rc_sm/ie/asn/GUMMEI.c
  ../../../flexric/src/sm/rc_sm/ie/asn/MME-Code.c
  ../../../flexric/src/sm/rc_sm/ie/asn/MME-Group-ID.c
  ../../../flexric/src/sm/rc_sm/ie/asn/MME-UE-S1AP-ID.c
  ../../../flexric/src/sm/rc_sm/ie/asn/NGENB-CU-UE-W1AP-ID.c
  ../../../flexric/src/sm/rc_sm/ie/asn/NgENB-ID.c
  ../../../flexric/src/sm/rc_sm/ie/asn/NG-RANnodeUEXnAPID.c
  ../../../flexric/src/sm/rc_sm/ie/asn/PLMNIdentity.c
  ../../../flexric/src/sm/rc_sm/ie/asn/RANUEID.c
  ../../../flexric/src/sm/rc_sm/ie/asn/UEID-ENB.c
  ../../../flexric/src/sm/rc_sm/ie/asn/UEID-EN-GNB.c
  ../../../flexric/src/sm/rc_sm/ie/asn/UEID-GNB-CU-CP-E1AP-ID-Item.c
  ../../../flexric/src/sm/rc_sm/ie/asn/UEID-GNB-CU-CP-E1AP-ID-List.c
  ../../../flexric/src/sm/rc_sm/ie/asn/UEID-GNB-CU-CP-F1AP-ID-Item.c
  ../../../flexric/src/sm/rc_sm/ie/asn/UEID-GNB-CU-F1AP-ID-List.c
  ../../../flexric/src/sm/rc_sm/ie/asn/UEID-GNB-CU-UP.c
  ../../../flexric/src/sm/rc_sm/ie/asn/UEID-GNB-DU.c
  ../../../flexric/src/sm/rc_sm/ie/asn/UEID-GNB.c
  ../../../flexric/src/sm/rc_sm/ie/asn/UEID.c
  ../../../flexric/src/sm/rc_sm/ie/asn/UEID-NG-ENB-DU.c
  ../../../flexric/src/sm/rc_sm/ie/asn/UEID-NG-ENB.c
)

# Standalone helper lib for the unit test (does not pull softmodem globals).
add_library(ran_func_rc_helper STATIC
  ${UE_ID_COMMON_E2SM_SRCS}
  ../ran_func_rc_helper.c
)
target_link_libraries(ran_func_rc_helper PUBLIC
  asn1_nr_rrc nr_rrc asn1_nr_rrc_hdrs e2_time_obj kpm_ric_info_common_obj
  3gpp_derived_ie_obj e2sm_rc_ir_obj sm_common_ie_obj ds alg
  sm_common_enc_asn_obj_rc 3gpp_derived_ie_enc_asn_obj_rc
)
target_include_directories(ran_func_rc_helper PUBLIC
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/..
  ${CMAKE_SOURCE_DIR}/openair2/LAYER2/NR_MAC_gNB
  ${CMAKE_SOURCE_DIR}/openair2/LAYER2/NR_MAC_gNB/slice_prb_allocator
)
target_compile_definitions(ran_func_rc_helper PUBLIC
  ${E2AP_VERSION}
  ${KPM_VERSION}
)

add_executable(ran_func_rc_test
  ran_func_rc_test.c
  ../../../flexric/src/sm/rc_sm/ie/rc_data_ie.c
  ../../../flexric/src/sm/rc_sm/enc/rc_enc_asn_xapp_lib.c
)
target_link_libraries(ran_func_rc_test PRIVATE ran_func_rc_helper m)
add_dependencies(tests ran_func_rc_test)
add_test(NAME ran_func_rc_test COMMAND ran_func_rc_test)
