#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#include "common/utils/assertions.h"

#include "f1ap_messages_types.h"
#include "F1AP_F1AP-PDU.h"

#include "f1ap_lib_extern.h"

void exit_function(const char *file, const char *function, const int line, const char *s, const int assert)
{
  printf("detected error at %s:%d:%s: %s\n", file, line, function, s);
  abort();
}

static F1AP_F1AP_PDU_t *f1ap_encode_decode(const F1AP_F1AP_PDU_t *enc_pdu)
{
  //xer_fprint(stdout, &asn_DEF_F1AP_F1AP_PDU, enc_pdu);

  DevAssert(enc_pdu != NULL);
  char errbuf[1024];
  size_t errlen = sizeof(errbuf);
  int ret = asn_check_constraints(&asn_DEF_F1AP_F1AP_PDU, enc_pdu, errbuf, &errlen);
  AssertFatal(ret == 0, "asn_check_constraints() failed: %s\n", errbuf);

  uint8_t msgbuf[16384];
  asn_enc_rval_t enc = aper_encode_to_buffer(&asn_DEF_F1AP_F1AP_PDU, NULL, enc_pdu, msgbuf, sizeof(msgbuf));
  AssertFatal(enc.encoded > 0, "aper_encode_to_buffer() failed\n");

  F1AP_F1AP_PDU_t *dec_pdu = NULL;
  asn_codec_ctx_t st = {.max_stack_size = 100 * 1000};
  asn_dec_rval_t dec = aper_decode(&st, &asn_DEF_F1AP_F1AP_PDU, (void **)&dec_pdu, msgbuf, enc.encoded, 0, 0);
  AssertFatal(dec.code == RC_OK, "aper_decode() failed\n");

  //xer_fprint(stdout, &asn_DEF_F1AP_F1AP_PDU, dec_pdu);
  return dec_pdu;
}

static void f1ap_msg_free(F1AP_F1AP_PDU_t *pdu)
{
  ASN_STRUCT_FREE(asn_DEF_F1AP_F1AP_PDU, pdu);
}

static void test_initial_ul_rrc_message_transfer(void)
{
  f1ap_plmn_t plmn = { .mcc = 208, .mnc = 95, .mnc_digit_length = 2 };
  uint8_t rrc[] = "RRC Container";
  uint8_t du2cu[] = "DU2CU Container";
  f1ap_initial_ul_rrc_message_t orig = {
    .gNB_DU_ue_id = 12,
    .plmn = plmn,
    .nr_cellid = 135,
    .crnti = 0x1234,
    .rrc_container = rrc,
    .rrc_container_length = sizeof(rrc),
    .du2cu_rrc_container = du2cu,
    .du2cu_rrc_container_length = sizeof(du2cu),
    .transaction_id = 2,
  };

  F1AP_F1AP_PDU_t *f1enc = encode_initial_ul_rrc_message_transfer(&orig);
  F1AP_F1AP_PDU_t *f1dec = f1ap_encode_decode(f1enc);
  f1ap_msg_free(f1enc);

  f1ap_initial_ul_rrc_message_t decoded;
  bool ret = decode_initial_ul_rrc_message_transfer(f1dec, &decoded);
  AssertFatal(ret, "decode_initial_ul_rrc_message_transfer(): could not decode message\n");
  f1ap_msg_free(f1dec);

  ret = eq_initial_ul_rrc_message_transfer(&orig, &decoded);
  AssertFatal(ret, "eq_initial_ul_rrc_message_transfer(): decoded message doesn't match\n");
  free_initial_ul_rrc_message_transfer(&decoded);

  f1ap_initial_ul_rrc_message_t cp = cp_initial_ul_rrc_message_transfer(&orig);
  ret = eq_initial_ul_rrc_message_transfer(&orig, &cp);
  AssertFatal(ret, "eq_initial_ul_rrc_message_transfer(): copied message doesn't match\n");
  free_initial_ul_rrc_message_transfer(&cp);
}

int main()
{
  test_initial_ul_rrc_message_transfer();
  return 0;
}
