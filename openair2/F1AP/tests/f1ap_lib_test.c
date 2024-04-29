#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#include "common/utils/assertions.h"

#include "f1ap_messages_types.h"
#include "F1AP_F1AP-PDU.h"

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

int main()
{
  return 0;
}
