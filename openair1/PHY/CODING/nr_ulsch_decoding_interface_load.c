/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.1  (the "License"); you may not use this file
 * except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.openairinterface.org/?page_id=698
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------------------
 * For more information about the OpenAirInterface (OAI) Software Alliance:
 *      contact@openairinterface.org
 */

/*! \file nrLDPC_load.c
 * \brief: load library implementing coding/decoding algorithms
 * \author Francois TABURET
 * \date 2020
 * \version 0.1
 * \company NOKIA BellLabs France
 * \email: francois.taburet@nokia-bell-labs.com
 * \note
 * \warning
 */
#define _GNU_SOURCE 
#include <sys/types.h>
#include <stdlib.h>
#include <malloc.h>
#include "assertions.h"
#include "common/utils/LOG/log.h"
#include "PHY/CODING/nrLDPC_extern.h"
#include "common/config/config_userapi.h"
#include "common/utils/load_module_shlib.h"
#include "PHY/CODING/nr_ulsch_decoding_interface.h"

/* arguments used when called from phy simulators exec's which do not use the config module */
/* arg is used to initialize the config module so that the loader works as expected */
char *arguments_phy_simulators[64]={"ldpctest",NULL};

int load_nr_ulsch_decoding_interface(char *version, nr_ulsch_decoding_interface_t *itf)
{
  char *ptr = (char *)config_get_if();
  char libname[64] = "nr_ulsch_decoding";

  if (ptr == NULL) { // phy simulators, config module possibly not loaded
    uniqCfg = load_configmodule(1, arguments_phy_simulators, CONFIG_ENABLECMDLINEONLY);
    logInit();
  }
  /* function description array, to be used when loading the encoding/decoding shared lib */
  loader_shlibfunc_t shlib_fdesc[] = {{.fname = "nr_ulsch_decoding_init"},
                                      {.fname = "nr_ulsch_decoding_shutdown"},
                                      {.fname = "nr_ulsch_decoding_decoder"}};
  int ret;
  ret = load_module_version_shlib(libname, version, shlib_fdesc, sizeofArray(shlib_fdesc), NULL);
  AssertFatal((ret >= 0), "Error loading NR ULSCH decoding module");
  itf->nr_ulsch_decoding_init = (nr_ulsch_decoding_init_t *)shlib_fdesc[0].fptr;
  itf->nr_ulsch_decoding_shutdown = (nr_ulsch_decoding_shutdown_t *)shlib_fdesc[1].fptr;
  itf->nr_ulsch_decoding_decoder = (nr_ulsch_decoding_decoder_t *)shlib_fdesc[2].fptr;

  AssertFatal(itf->nr_ulsch_decoding_init() == 0, "error starting LDPC library %s %s\n", libname, version);

  return 0;
}

int free_nr_ulsch_decoding_interface(nr_ulsch_decoding_interface_t *interface)
{
  return interface->nr_ulsch_decoding_shutdown();
}
