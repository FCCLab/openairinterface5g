/*! \file PHY/CODING/nrLDPC_decoder_offload_xdma/nrLDPC_decoder_offload_xdma.h
 * \briefFPGA accelerator integrated into OAI (for one and multi code block)
 * \author Sendren Xu, SY Yeh(fdragon), Hongming, Terng-Yin Hsu
 * \date 2022-05-31
 * \version 5.0
 * \email: summery19961210@gmail.com
 */

#include <stdint.h>

/**
    \brief LDPC input parameter
    \param Zc shifting size
    \param Rows
    \param baseGraph base graph
    \param CB_num number of code block
    \param numChannelLlrs input soft bits length, Zc x 66 - length of filler bits
    \param numFillerBits filler bits length
*/

typedef struct {
  unsigned char max_schedule;
  unsigned char SetIdx;
  int Zc;
  unsigned char numCB;
  unsigned char BG;
  unsigned char max_iter;
  int nRows;
  int numChannelLls;
  int numFillerBits;
} DecIFConf;

int nrLDPC_decoder_FPGA_8038(int8_t *buf_in, int8_t *buf_out, DecIFConf dec_conf);
int nrLDPC_decoder_FPGA_PYM(int8_t *buf_in, int8_t *buf_out, DecIFConf dec_conf);
// int nrLDPC_decoder_FPGA_PYM();
