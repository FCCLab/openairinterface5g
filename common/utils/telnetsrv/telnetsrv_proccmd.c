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

/*! \file common/utils/telnetsrv/telnetsrv_proccmd.c
 * \brief: implementation of telnet commands related to this linux process
 * \author Francois TABURET
 * \date 2017
 * \version 0.1
 * \company NOKIA BellLabs France
 * \email: francois.taburet@nokia-bell-labs.com
 * \note
 * \warning
 */
#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/sysinfo.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <string.h>
#include <stdarg.h>
#include <dirent.h>

#define READCFG_DYNLOAD

#define TELNETSERVERCODE
#include "telnetsrv.h"
#define TELNETSRV_PROCCMD_MAIN
#include "common/utils/LOG/log.h"
#include "common/config/config_userapi.h"
#include "openair1/PHY/phy_extern.h"
#include "telnetsrv_proccmd.h"
#include "openair2/LAYER2/NR_MAC_gNB/nr_mac_gNB.h"
#include "openair2/LAYER2/NR_MAC_gNB/gNB_scheduler_types.h"
#include "openair2/LAYER2/NR_MAC_gNB/mac_proto.h"
#include "openair2/F1AP/f1ap_ids.h"
#include "openair2/LAYER2/NR_MAC_gNB/slice_prb_allocator/slice_prb_allocator.h"

void decode_procstat(char *record, int debug, telnet_printfunc_t prnt, webdatadef_t *tdata)
{
char prntline[160];
char *procfile_fields;
char *strtokptr;
char *lptr;
int fieldcnt;
char toksep[2];

fieldcnt = 0;
procfile_fields = strtok_r(record, " ", &strtokptr);
lptr = prntline;
/*http://man7.org/linux/man-pages/man5/proc.5.html gives the structure of the stat file */
int priority = 0;
int nice = 0;
while (procfile_fields != NULL && fieldcnt < 42) {
  long int policy;
  if (strlen(procfile_fields) == 0)
    continue;
  fieldcnt++;
  sprintf(toksep, " ");
  switch (fieldcnt) {
    case 1: /* id */
      if (tdata != NULL) {
        tdata->lines[tdata->numlines].val[0] = strdup(procfile_fields);
      }
      lptr += sprintf(lptr, "%9.9s ", procfile_fields);
      sprintf(toksep, ")");
      break;
    case 2: /* name */
      if (tdata != NULL) {
        tdata->lines[tdata->numlines].val[1] = strdup(procfile_fields);
      }
      lptr += sprintf(lptr, "%20.20s ", procfile_fields + 1);
      break;
    case 3: // thread state
      lptr += sprintf(lptr, "  %c   ", procfile_fields[0]);
      break;
    case 14: // time in user mode
    case 15: // time in kernel mode
      lptr += sprintf(lptr, "%9.9s ", procfile_fields);
      break;
    case 18: // priority column index 2 in tdata, -2 to -100 (1, min to 99, highest prio)
      priority = strtol(procfile_fields, NULL, 0);
    case 19: // nice	  column index 3 in tdata  0 to 39 (-20, highest prio, to 19)
      if (tdata != NULL) {
        tdata->lines[tdata->numlines].val[fieldcnt - 16] = strdup(procfile_fields);
      }
      lptr += sprintf(lptr, "%3.3s ", procfile_fields);
      nice = strtol(procfile_fields, NULL, 0);
      break;
    case 23: // vsize
      lptr += sprintf(lptr, "%9.9s ", procfile_fields);
      break;
    case 39: // processor
      if (tdata != NULL) {
        tdata->lines[tdata->numlines].val[4] = strdup(procfile_fields);
      }
      lptr += sprintf(lptr, " %2.2s  ", procfile_fields);
      break;
    case 41: // policy
      lptr += sprintf(lptr, "%3.3s ", procfile_fields);
      policy = strtol(procfile_fields, NULL, 0);
      char strschedp[64];
      switch (policy) {
        case SCHED_FIFO:
          snprintf(strschedp, sizeof(strschedp), "%s ", "rt:fifo");
          priority = priority + 1; // in /proc file system priority 1 to 99 mapped to -2 to -100
          break;
        case SCHED_OTHER:
          snprintf(strschedp, sizeof(strschedp), "%s ", "other");
          priority = nice - NICE_MIN; // linux nice is -20 to 19
          break;
        case SCHED_IDLE:
          snprintf(strschedp, sizeof(strschedp), "%s ", "idle");
          priority = 2 * (NICE_MAX - NICE_MIN + 1);
          break;
        case SCHED_BATCH:
          snprintf(strschedp, sizeof(strschedp), "%s ", "batch");
          priority = (NICE_MAX - NICE_MIN + 1) + nice - NICE_MIN;
          break;
        case SCHED_RR:
          snprintf(strschedp, sizeof(strschedp), "%s ", "rt:rr");
          priority = priority - 99;
          break;
#ifdef SCHED_DEADLINE
        case SCHED_DEADLINE:
          snprintf(strschedp, sizeof(strschedp), "%s ", "rt:deadline");
          break;
#endif
        default:
          snprintf(strschedp, sizeof(strschedp), "%s ", "????");
          break;
      }
      lptr += sprintf(lptr, "%s ", strschedp);
      if (tdata != NULL) {
        tdata->lines[tdata->numlines].val[5] = strdup(strschedp);
        tdata->lines[tdata->numlines].val[6] = malloc(10);
        snprintf(tdata->lines[tdata->numlines].val[6], 9, "%i", priority);
      }
      break;
    default:
      break;
  } /* switch on fieldcnr */
  procfile_fields = strtok_r(NULL, toksep, &strtokptr);
} /* while on proc_fields != NULL */
prnt("%s\n", prntline);
if (tdata != NULL) {
  tdata->numlines++;
}
} /*decode_procstat */

void read_statfile(char *fname, int debug, telnet_printfunc_t prnt, webdatadef_t *tdata)
{
FILE *procfile;
char arecord[1024];

    procfile=fopen(fname,"r");
    if (procfile == NULL)
       {
       prnt("Error: Couldn't open %s %i %s\n",fname,errno,strerror(errno));
       return;
       }    
    if ( fgets(arecord,sizeof(arecord),procfile) == NULL)
       {
       prnt("Error: Nothing read from %s %i %s\n",fname,errno,strerror(errno));
       fclose(procfile);
       return;
       }    
    fclose(procfile);
    decode_procstat(arecord, debug, prnt, tdata);
}

int nullprnt(char *fmt, ...)
{
  return 0;
}

void proccmd_get_threaddata(char *buf, int debug, telnet_printfunc_t fprnt, webdatadef_t *tdata)
{
char aname[256];

  DIR *proc_dir;
  struct dirent *entry;
  telnet_printfunc_t prnt = (fprnt != NULL) ? fprnt : (telnet_printfunc_t)nullprnt;
  if (tdata != NULL) {
    tdata->numcols = 7;
    snprintf(tdata->columns[0].coltitle, sizeof(tdata->columns[0].coltitle), "thread id");
    tdata->columns[0].coltype = TELNET_VARTYPE_STRING | TELNET_CHECKVAL_RDONLY | TELNET_VAR_NEEDFREE;
    snprintf(tdata->columns[1].coltitle, sizeof(tdata->columns[1].coltitle), "thread name");
    tdata->columns[1].coltype = TELNET_VARTYPE_STRING | TELNET_CHECKVAL_RDONLY | TELNET_VAR_NEEDFREE;
    snprintf(tdata->columns[2].coltitle, sizeof(tdata->columns[2].coltitle), "priority");
    tdata->columns[2].coltype = TELNET_VARTYPE_STRING | TELNET_CHECKVAL_RDONLY | TELNET_VAR_NEEDFREE;
    snprintf(tdata->columns[3].coltitle, sizeof(tdata->columns[3].coltitle), "nice");
    tdata->columns[3].coltype = TELNET_VARTYPE_STRING | TELNET_CHECKVAL_RDONLY | TELNET_VAR_NEEDFREE;
    snprintf(tdata->columns[4].coltitle, sizeof(tdata->columns[4].coltitle), "core");
    tdata->columns[4].coltype = TELNET_VARTYPE_STRING | TELNET_VAR_NEEDFREE;
    snprintf(tdata->columns[5].coltitle, sizeof(tdata->columns[5].coltitle), "sched policy");
    tdata->columns[5].coltype = TELNET_VARTYPE_STRING | TELNET_CHECKVAL_RDONLY | TELNET_VAR_NEEDFREE;
    snprintf(tdata->columns[5].coltitle, sizeof(tdata->columns[5].coltitle), "sched policy");
    tdata->columns[6].coltype = TELNET_VARTYPE_STRING | TELNET_VAR_NEEDFREE;
    snprintf(tdata->columns[6].coltitle, sizeof(tdata->columns[6].coltitle), "oai priority");
    tdata->numlines = 0;
  }

  int nprocs = get_nprocs();
  prnt("System has %d cores", nprocs);

  prnt("\n  id          name            state   USRmod    KRNmod  prio nice   vsize   proc pol \n\n");
  snprintf(aname, sizeof(aname), "/proc/%d/stat", getpid());
  read_statfile(aname, debug, prnt, NULL);
  prnt("\n");
  snprintf(aname, sizeof(aname), "/proc/%d/task", getpid());
  proc_dir = opendir(aname);
  if (proc_dir == NULL) {
    prnt("Error: Couldn't open %s %i %s\n", aname, errno, strerror(errno));
    return;
  }

  while ((entry = readdir(proc_dir)) != NULL) {
    if (entry->d_name[0] != '.') {
      snprintf(aname, sizeof(aname), "/proc/%d/task/%.*s/stat", getpid(), (int)(sizeof(aname) - 24), entry->d_name);
      read_statfile(aname, debug, prnt, tdata);
    }
  } /* while entry != NULL */
  closedir(proc_dir);
} /* proccmd_get_threaddata */

void print_threads(char *buf, int debug, telnet_printfunc_t prnt)
{
  proccmd_get_threaddata(buf, debug, prnt, NULL);
}

#define FLAG_PRINT_DEBUG_DUMP(flag)                                              \
  logsdata->lines[i].val[0] = (char *)flag_name[i];                              \
  logsdata->lines[i].val[1] = g_log->debug_mask.DEBUG_##flag ? "true" : "false"; \
  logsdata->lines[i].val[1] = g_log->dump_mask.DEBUG_##flag ? "true" : "false";  \
  i++;

int proccmd_websrv_getdata(char *cmdbuff, int debug, void *data, telnet_printfunc_t prnt)
{
  webdatadef_t *logsdata = (webdatadef_t *)data;
  const mapping *const log_level_names = log_level_names_ptr();
  const mapping *const log_options = log_option_names_ptr();
  if (strncmp(cmdbuff, "set", 3) == 0) {
    telnet_printfunc_t printfunc = (prnt != NULL) ? prnt : (telnet_printfunc_t)printf;
    if (strcasestr(cmdbuff, "loglvl") != NULL) {
      int level = map_str_to_int(log_level_names, logsdata->lines[0].val[1]);
      int enabled = (strcmp(logsdata->lines[0].val[2], "true") == 0) ? 1 : 0;
      int loginfile = (strcmp(logsdata->lines[0].val[3], "true") == 0) ? 1 : 0;
      set_log(logsdata->numlines, level);
      if (enabled == 0)
        set_log(logsdata->numlines, OAILOG_DISABLE);
      if (loginfile == 1) {
        set_component_filelog(logsdata->numlines);
      } else {
        close_component_filelog(logsdata->numlines);
      }
      printfunc("%s log level %s is %s, output to %s\n",
                logsdata->lines[0].val[0],
                logsdata->lines[0].val[1],
                enabled ? "enabled" : "disabled",
                loginfile ? g_log->log_rarely_used[logsdata->numlines].filelog_name : "stdout");
    }
    if (strcasestr(cmdbuff, "logopt") != NULL) {
      int optbit = map_str_to_int(log_options, logsdata->lines[0].val[0]);
      if (optbit < 0) {
        printfunc("option %s unknown\n", logsdata->lines[0].val[0]);
      } else {
        if (strcmp(logsdata->lines[0].val[1], "true") == 0) {
          SET_LOG_OPTION(optbit);
        } else {
          CLEAR_LOG_OPTION(optbit);
        }
        printfunc("%s log option %s\n", logsdata->lines[0].val[0], (strcmp(logsdata->lines[0].val[1], "true") == 0) ? "enabled" : "disabled");
      }
    }
    if (strcasestr(cmdbuff, "dbgopt") != NULL) {
      if (strcmp(logsdata->lines[0].val[1], "true") == 0)
        if (!set_log_debug(logsdata->lines[0].val[0], strcmp(logsdata->lines[0].val[2], "true") == 0))
          printfunc("debug option %s unknown\n", logsdata->lines[0].val[0]);
      if (strcmp(logsdata->lines[0].val[2], "true") == 0)
        if (!set_log_dump(logsdata->lines[0].val[0], strcmp(logsdata->lines[0].val[2], "true") == 0))
          printfunc("debug option %s unknown\n", logsdata->lines[0].val[0]);
      printfunc("%s debug %s dump %s\n",
                logsdata->lines[0].val[0],
                (strcmp(logsdata->lines[0].val[1], "true") == 0) ? "enabled" : "disabled",
                (strcmp(logsdata->lines[0].val[2], "true") == 0) ? "enabled" : "disabled");
    }
    if (strcasestr(cmdbuff, "threadsched") != NULL) {
      unsigned int tid = strtoll(logsdata->lines[0].val[0], NULL, 0);
      unsigned int core = strtoll(logsdata->lines[0].val[4], NULL, 0);
      int priority = strtoll(logsdata->lines[0].val[6], NULL, 0);
      printfunc("Thread %s id %u set affinity to core %u\n", logsdata->lines[0].val[0], tid, core);
      set_affinity(0, (pid_t)tid, core);
      set_sched(0, (pid_t)tid, priority);
    }
  } else { /* end of set, => show */
    if (strcasestr(cmdbuff, "loglvl") != NULL) {
      logsdata->numcols = 4;
      logsdata->numlines = 0;
      snprintf(logsdata->columns[0].coltitle, TELNET_CMD_MAXSIZE, "component");
      logsdata->columns[0].coltype = TELNET_VARTYPE_STRING | TELNET_CHECKVAL_RDONLY;
      snprintf(logsdata->columns[1].coltitle, TELNET_CMD_MAXSIZE, "level");
      logsdata->columns[1].coltype = TELNET_VARTYPE_STRING | TELNET_CHECKVAL_LOGLVL;
      snprintf(logsdata->columns[2].coltitle, TELNET_CMD_MAXSIZE, "enabled");
      logsdata->columns[2].coltype = TELNET_CHECKVAL_BOOL;
      snprintf(logsdata->columns[3].coltitle, TELNET_CMD_MAXSIZE, "in file");
      logsdata->columns[3].coltype = TELNET_CHECKVAL_BOOL;

      for (int i = 0; i < MAX_LOG_COMPONENTS; i++) {
        if (g_log->log_component[i].name != NULL) {
          logsdata->numlines++;
          logsdata->lines[i].val[0] = (char *)(g_log->log_component[i].name);

          logsdata->lines[i].val[1] = map_int_to_str(
              log_level_names,
              (g_log->log_component[i].level >= 0) ? g_log->log_component[i].level : g_log->log_rarely_used[i].savedlevel);
          logsdata->lines[i].val[2] = (g_log->log_component[i].level >= 0) ? "true" : "false";
          logsdata->lines[i].val[3] = (g_log->log_component[i].filelog > 0) ? "true" : "false";
        }
      }
    }
    if (strcasestr(cmdbuff, "dbgopt") != NULL) {
      webdatadef_t *logsdata = (webdatadef_t *)data;
      logsdata->numcols = 3;
      logsdata->numlines = 0;
      snprintf(logsdata->columns[0].coltitle, TELNET_CMD_MAXSIZE, "module");
      logsdata->columns[0].coltype = TELNET_VARTYPE_STRING | TELNET_CHECKVAL_RDONLY;
      snprintf(logsdata->columns[1].coltitle, TELNET_CMD_MAXSIZE, "debug");
      logsdata->columns[1].coltype = TELNET_CHECKVAL_BOOL;
      snprintf(logsdata->columns[2].coltitle, TELNET_CMD_MAXSIZE, "dump");
      logsdata->columns[2].coltype = TELNET_CHECKVAL_BOOL;

      int i = 0;
      FOREACH_FLAG(FLAG_PRINT_DEBUG_DUMP);
      logsdata->numlines += i;
    }

    if (strcasestr(cmdbuff, "logopt") != NULL) {
      webdatadef_t *logsdata = (webdatadef_t *)data;
      logsdata->numcols = 2;
      logsdata->numlines = 0;
      snprintf(logsdata->columns[0].coltitle, TELNET_CMD_MAXSIZE, "option");
      logsdata->columns[0].coltype = TELNET_VARTYPE_STRING | TELNET_CHECKVAL_RDONLY;
      snprintf(logsdata->columns[1].coltitle, TELNET_CMD_MAXSIZE, "enabled");
      logsdata->columns[1].coltype = TELNET_CHECKVAL_BOOL;

      for (int i = 0; log_options[i].name != NULL; i++) {
        logsdata->numlines++;
        logsdata->lines[i].val[0] = (char *)log_options[i].name;
        logsdata->lines[i].val[1] = (g_log->flag & log_options[i].value) ? "true" : "false";
      }
    }
    if (strcasestr(cmdbuff, "threadsched") != NULL) {
      proccmd_get_threaddata(cmdbuff, debug, prnt, (webdatadef_t *)data);
    }
  } // show

  return 0;
}

#define FLAG_PRINT2_DEBUG_DUMP(flag)               \
  prnt("%02i %17.17s %5.5s   %5.5s\n",             \
       i,                                          \
       flag_name[i],                               \
       g_log->debug_mask.DEBUG_##flag ? "Y" : "N", \
       g_log->dump_mask.DEBUG_##flag ? "Y" : "N"); \
  i++;

int proccmd_show(char *buf, int debug, telnet_printfunc_t prnt)
{
  if (buf == NULL) {
    prnt("ERROR wrong softmodem SHOW command...\n");
    return 0;
  }
   if (debug > 0)
       prnt(" proccmd_show received %s\n",buf);
   if (strcasestr(buf,"thread") != NULL) {
       print_threads(buf,debug,prnt);
   }
   if (strcasestr(buf,"loglvl") != NULL) {
       prnt("\n               component level  enabled   output\n");
       const mapping *const log_level_names = log_level_names_ptr();
       for (int i = 0; i < MAX_LOG_COMPONENTS; i++) {
         if (g_log->log_component[i].name != NULL) {
           prnt("%02i %17.17s:%10.10s    %s      %s\n",
                i,
                g_log->log_component[i].name,
                map_int_to_str(
                    log_level_names,
                    (g_log->log_component[i].level >= 0) ? g_log->log_component[i].level : g_log->log_rarely_used[i].savedlevel),
                ((g_log->log_component[i].level >= 0) ? "Y" : "N"),
                ((g_log->log_component[i].filelog > 0) ? g_log->log_rarely_used[i].filelog_name : "stdout"));
         }
       }
   }
   if (strcasestr(buf,"logopt") != NULL) {
       prnt("\n               option      enabled\n");
       const mapping *const log_options = log_option_names_ptr();
       for (int i=0; log_options[i].name != NULL; i++) {
               prnt("%02i %17.17s %10.10s \n",i ,log_options[i].name, 
                     ((g_log->flag & log_options[i].value)?"Y":"N") );
       }
   }
   if (strcasestr(buf,"dbgopt") != NULL) {
       prnt("\n               module  debug dumpfile\n");
       int i = 0;
       FOREACH_FLAG(FLAG_PRINT2_DEBUG_DUMP);
   }
   if (strcasestr(buf,"ues") != NULL) {
       // Show connected UEs for gNB
       if (RC.nrmac != NULL && RC.nrmac[0] != NULL) {
           gNB_MAC_INST *mac = RC.nrmac[0];
           NR_SCHED_LOCK(&mac->sched_lock);
           NR_UEs_t *UE_info = &mac->UE_info;
           
           UE_iterator(UE_info->connected_ue_list, UE) {
               if (UE != NULL) {
                   NR_UE_sched_ctrl_t *sched_ctrl = &UE->UE_sched_ctrl;
                   NR_mac_stats_t *stats = &UE->mac_stats;
                   const int avg_rsrp = stats->num_rsrp_meas > 0 ? stats->cumul_rsrp / stats->num_rsrp_meas : 0;
                   
                   // UE header with RNTI
                   prnt("UE 0x%04x:\n", UE->rnti);
                   
                   // CU-UE-ID, in-sync, PH, PCMAX, RSRP
                   prnt("  UE RNTI %04x CU-UE-ID ", UE->rnti);
                   if (du_exists_f1_ue_data(UE->rnti)) {
                       f1_ue_data_t ued = du_get_f1_ue_data(UE->rnti);
                       prnt("%d", ued.secondary_ue);
                   } else {
                       prnt("(none)");
                   }
                   
                   bool in_sync = !sched_ctrl->ul_failure;
                   prnt(" %s PH %d dB PCMAX %d dBm", 
                        in_sync ? "in-sync" : "out-of-sync",
                        sched_ctrl->ph,
                        sched_ctrl->pcmax);
                   
                   if (stats->num_rsrp_meas) {
                       prnt(", average RSRP %d (%d meas)", avg_rsrp, stats->num_rsrp_meas);
                   }
                   prnt("\n");
                   
                   // DLSCH statistics
                   prnt("  UE %04x: dlsch_rounds ", UE->rnti);
                   prnt("%"PRIu64, stats->dl.rounds[0]);
                   for (int i = 1; i < mac->dl_bler.harq_round_max; i++) {
                       prnt("/%"PRIu64, stats->dl.rounds[i]);
                   }
                   prnt(", dlsch_errors %"PRIu64", pucch0_DTX %d, BLER %.5f MCS (%d) %d CCE fail %d\n",
                        stats->dl.errors,
                        stats->pucch0_DTX,
                        sched_ctrl->dl_bler_stats.bler,
                        UE->current_DL_BWP.mcsTableIdx,
                        sched_ctrl->dl_bler_stats.mcs,
                        sched_ctrl->dl_cce_fail);
                   
                   // ULSCH statistics
                   prnt("  UE %04x: ulsch_rounds ", UE->rnti);
                   prnt("%"PRIu64, stats->ul.rounds[0]);
                   for (int i = 1; i < mac->ul_bler.harq_round_max; i++) {
                       prnt("/%"PRIu64, stats->ul.rounds[i]);
                   }
                   prnt(", ulsch_errors %"PRIu64", ulsch_DTX %d, BLER %.5f MCS (%d) %d (Qm %d deltaMCS %d dB) NPRB %d  SNR %d.%d dB CCE fail %d\n",
                        stats->ul.errors,
                        stats->ulsch_DTX,
                        sched_ctrl->ul_bler_stats.bler,
                        UE->current_UL_BWP.mcs_table,
                        sched_ctrl->ul_bler_stats.mcs,
                        nr_get_Qm_ul(sched_ctrl->ul_bler_stats.mcs, UE->current_UL_BWP.mcs_table),
                        UE->mac_stats.deltaMCS,
                        UE->mac_stats.NPRB,
                        sched_ctrl->pusch_snrx10 / 10,
                        sched_ctrl->pusch_snrx10 % 10,
                        sched_ctrl->ul_cce_fail);
                   
                   // MAC TX/RX bytes
                   prnt("  UE %04x: MAC:    TX %14"PRIu64" RX %14"PRIu64" bytes\n",
                        UE->rnti, stats->dl.total_bytes, stats->ul.total_bytes);
                   
                   // Per-LCID TX/RX bytes
                   for (int i = 0; i < seq_arr_size(&sched_ctrl->lc_config); i++) {
                       const nr_lc_config_t *c = seq_arr_at(&sched_ctrl->lc_config, i);
                       prnt("  UE %04x: LCID %d: TX %14"PRIu64" RX %14"PRIu64" bytes\n",
                            UE->rnti,
                            c->lcid,
                            stats->dl.lc_bytes[c->lcid],
                            stats->ul.lc_bytes[c->lcid]);
                   }
                   
                   // Get NSSAI from logical channels (DRBs) - these come from PDU resource setup request
                   for (int i = 0; i < seq_arr_size(&sched_ctrl->lc_config); i++) {
                      const nr_lc_config_t *c = seq_arr_at(&sched_ctrl->lc_config, i);
                      const uint8_t sst = c->nssai.sst;
                      const uint32_t sd = c->nssai.sd;
                      const int lcid = c->lcid;
                        
                      prnt("  UE %04x: Index %d/%d LCID %2d | SST: %d | SD: 0x%06x | Buffer: %-10d bytes\n",
                           UE->rnti,
                           i,
                           seq_arr_size(&sched_ctrl->lc_config),
                           lcid,
                           sst,
                           sd,
                           sched_ctrl->rlc_status[lcid].bytes_in_buffer);
                   }
                   prnt("\n");
               }
           }
           
           NR_SCHED_UNLOCK(&mac->sched_lock);
       } else {
           prnt("gNB MAC not available\n");
       }
   }
   if (strcasestr(buf,"config") != NULL) {
       prnt("Command line arguments:\n");
       for (int i=0; i < config_get_if()->argc; i++) {
            prnt("    %02i %s\n",i ,config_get_if()->argv[i]);
       }
       prnt("Config module flags ( -O <cfg source>:<xxx>:dbgl<flags>): 0x%08x\n", config_get_if()->rtflags); 

       prnt("    Print config debug msg, params values (flag %u): %s\n",CONFIG_PRINTPARAMS,
            ((config_get_if()->rtflags & CONFIG_PRINTPARAMS) ? "Y" : "N") ); 
       prnt("    Print config debug msg, memory management(flag %u): %s\n",CONFIG_DEBUGPTR,
            ((config_get_if()->rtflags & CONFIG_DEBUGPTR) ? "Y" : "N") ); 
       prnt("    Print config debug msg, command line processing (flag %u): %s\n",CONFIG_DEBUGCMDLINE,
            ((config_get_if()->rtflags & CONFIG_DEBUGCMDLINE) ? "Y" : "N") );        
       prnt("    Don't exit if param check fails (flag %u): %s\n",CONFIG_NOABORTONCHKF,
            ((config_get_if()->rtflags & CONFIG_NOABORTONCHKF) ? "Y" : "N") );      
       prnt("Config source: %s,  parameters:\n",CONFIG_GETSOURCE );
       for (int i=0; i < config_get_if()->num_cfgP; i++) {
            prnt("    %02i %s\n",i ,config_get_if()->cfgP[i]);
       }
       prnt("Softmodem components:\n");
       prnt("   %02i Ru(s)\n", RC.nb_RU);
       prnt("   %02i lte RRc(s),     %02i NbIoT RRC(s)\n",    RC.nb_inst, RC.nb_nb_iot_rrc_inst);
       prnt("   %02i lte MACRLC(s),  %02i NbIoT MACRLC(s)\n", RC.nb_macrlc_inst, RC.nb_nb_iot_macrlc_inst);
       prnt("   %02i lte L1,	    %02i NbIoT L1\n",	     RC.nb_L1_inst, RC.nb_nb_iot_L1_inst);

       for(int i=0; i<RC.nb_inst; i++) {
           prnt("    lte RRC %i:     %02i CC(s) \n",i,((RC.nb_CC == NULL)?0:RC.nb_CC[i]));
       }
       for(int i=0; i<RC.nb_L1_inst; i++) {
           prnt("    lte L1 %i:      %02i CC(s)\n",i,((RC.nb_L1_CC == NULL)?0:RC.nb_L1_CC[i]));
       }
       for(int i=0; i<RC.nb_macrlc_inst; i++) {
           prnt("    lte macrlc %i:  %02i CC(s)\n",i,((RC.nb_mac_CC == NULL)?0:RC.nb_mac_CC[i]));
       }
   }
   if (strcasestr(buf, "sch") != NULL) {
       // Display scheduler information
       if (RC.nb_nr_macrlc_inst > 0 && RC.nrmac != NULL && RC.nrmac[0] != NULL) {
           gNB_MAC_INST *mac = RC.nrmac[0];
           const char *scheduler_name = (mac->scheduler_type == SCHE_NS) ? "Network Slicing (SCHE_NS)" : "Proportional Fair (SCHE_PF)";
           
           prnt("\n=== Scheduler Information ===\n\n");               
           
           prnt("- MAC Module ID: %d\n", mac->Mod_id);
           prnt("  Scheduler Type: %s (%d)\n", scheduler_name, mac->scheduler_type);
           
           // Display pre-processor information (function pointer)
           if (mac->pre_processor_dl != NULL) {
               prnt("  DL Pre-processor: Active\n");
           } else {
               prnt("  DL Pre-processor: Not initialized\n");
           }
           
           if (mac->pre_processor_ul != NULL) {
               prnt("  UL Pre-processor: Active\n");
           } else {
               prnt("  UL Pre-processor: Not initialized\n");
           }
           
           // Display frame/slot information
           prnt("  Current Frame: %d\n", mac->frame);
           
           // Display BWP information if available
           if (mac->common_channels[0].ServingCellConfigCommon != NULL) {
               NR_ServingCellConfigCommon_t *scc = mac->common_channels[0].ServingCellConfigCommon;
               if (scc->downlinkConfigCommon && scc->downlinkConfigCommon->frequencyInfoDL) {
                   int bw = scc->downlinkConfigCommon->frequencyInfoDL->scs_SpecificCarrierList.list.array[0]->carrierBandwidth;
                   prnt("  Carrier Bandwidth: %d PRBs\n", bw);
               }
           }
           
           // Display slice scheduler information if available
           if (mac->scheduler_type == SCHE_NS && mac->slice_scheduler != NULL) {
               prnt("\n  === Network Slices ===\n");
               
               int num_slices = slice_sch_get_num_slices(mac->slice_scheduler);
               prnt("  Total Slices: %d\n", num_slices);
               
               if (num_slices > 0) {
                   int num_stats = 0;
                   const slice_statistics_t *all_stats = slice_sch_get_all_statistics(mac->slice_scheduler, &num_stats);
                   
                   if (all_stats != NULL) {
                       prnt("\n  Slice Configuration:\n");
                       prnt("  %-4s %-18s %-12s %-12s %-12s\n", 
                            "Idx", "SST/SD", "Dedicated", "Min Ratio", "Max Ratio");
                       prnt("  %s\n", "----------------------------------------------------------------------------");
                       
                       for (int s = 0; s < num_stats; ++s) {
                           uint8_t sst = all_stats[s].slice_id.sst;
                           uint32_t sd = all_stats[s].slice_id.sd;
                           
                           // Get slice configuration
                           uint8_t config_sst;
                           uint32_t config_sd;
                           float dedicated, min, max;
                           if (slice_sch_get_slice_config(mac->slice_scheduler, sst, sd, 
                                                          &config_sst, &config_sd, 
                                                          &dedicated, &min, &max) == 0) {
                               // Format SST/SD together in one column
                               char sst_sd_str[32];
                               if (sd > 0) {
                                   snprintf(sst_sd_str, sizeof(sst_sd_str), "%3d/%06x", sst, sd);
                               } else {
                                   snprintf(sst_sd_str, sizeof(sst_sd_str), "%3d/0x000000", sst);
                               }
                               
                               prnt("  %-4d %-18s %-12.3f %-12.3f %-12.3f\n", 
                                    s, sst_sd_str, dedicated, min, max);
                           }
                       }
                       
                       // Display allocation statistics if available
                       int num_active_slices = 0;
                       int total_allocated_prbs = 0;
                       if (slice_sch_get_stats(mac->slice_scheduler, &num_active_slices, &total_allocated_prbs) == 0) {
                           prnt("\n  Allocation Statistics:\n");
                           prnt("  Active Slices: %d\n", num_active_slices);
                           prnt("  Total Allocated PRBs: %d\n", total_allocated_prbs);
                           
                           int num_ranges = 0;
                           const slice_prb_range_t *ranges = slice_sch_get_allocation(mac->slice_scheduler, &num_ranges);
                           if (ranges != NULL && num_ranges > 0) {
                               prnt("\n  Current PRB Allocations:\n");
                               prnt("  %-18s %-12s %-12s %-12s\n", 
                                    "SST/SD", "Start PRB", "End PRB", "Num PRBs");
                               prnt("  %s\n", "------------------------------------------------------------");
                               
                               for (int r = 0; r < num_ranges; ++r) {
                                   if (ranges[r].num_prbs > 0) {
                                       char sst_sd_str[32];
                                       if (ranges[r].slice_id.sd > 0) {
                                           snprintf(sst_sd_str, sizeof(sst_sd_str), "%3d/%06x", 
                                                   ranges[r].slice_id.sst, ranges[r].slice_id.sd);
                                       } else {
                                           snprintf(sst_sd_str, sizeof(sst_sd_str), "%3d/0x000000", 
                                                   ranges[r].slice_id.sst);
                                       }
                                       prnt("  %-18s %-12d %-12d %-12d\n",
                                            sst_sd_str, ranges[r].start_prb, ranges[r].end_prb, ranges[r].num_prbs);
                                   }
                               }
                           }
                           
                           // Display per-slice statistics
                           prnt("\n  Per-Slice Statistics:\n");
                           prnt("  %-18s %-12s %-12s %-12s %-12s\n", 
                                "SST/SD", "Latest PRBs", "Avg PRBs", "Samples", "Allocated");
                           prnt("  %s\n", "----------------------------------------------------------------------------");
                           
                           for (int s = 0; s < num_stats; ++s) {
                               uint8_t sst = all_stats[s].slice_id.sst;
                               uint32_t sd = all_stats[s].slice_id.sd;
                               
                               // Format SST/SD together in one column
                               char sst_sd_str[32];
                               if (sd > 0) {
                                   snprintf(sst_sd_str, sizeof(sst_sd_str), "%3d/%06x", sst, sd);
                               } else {
                                   snprintf(sst_sd_str, sizeof(sst_sd_str), "%3d/0x000000", sst);
                               }
                               
                               prnt("  %-18s %-12d %-12.1f %-12d", 
                                    sst_sd_str,
                                    all_stats[s].latest_num_prbs, 
                                    all_stats[s].avg_num_prbs,
                                    all_stats[s].sample_count);
                               
                               // Check if slice has allocated PRBs
                               bool has_allocation = (all_stats[s].latest_num_prbs > 0);
                               prnt("  %s\n", has_allocation ? "Yes" : "No");
                           }
                       }
                   }
               } else {
                   prnt("  No slices configured\n");
               }
           } else if (mac->scheduler_type == SCHE_NS) {
               prnt("\n  Network Slicing enabled but slice scheduler not initialized\n");
           }
           
           prnt("\n=============================\n");
       } else {
           prnt("gNB MAC instance not available\n");
       }
   }
   return 0;
} 

int proccmd_thread(char *buf, int debug, telnet_printfunc_t prnt)
{
int bv1,bv2;   
int res;
char sv1[64];

if (buf == NULL) {
  prnt("ERROR wrong thread command...\n");
  return 0;
}
   bv1=0;
   bv2=0;
   sv1[0]=0;
   if (debug > 0)
       prnt("proccmd_thread received %s\n",buf);
   if (strcasestr(buf,"help") != NULL) {
          prnt(PROCCMD_THREAD_HELP_STRING);
          return 0;
   } 
   res=sscanf(buf,"%i %9s %i",&bv1,sv1,&bv2);
   if (debug > 0)
       prnt(" proccmd_thread: %i params = %i,%s,%i\n",res,bv1,sv1,bv2);   
   if(res != 3)
     {
     print_threads(buf, debug, prnt);
     return 0;
     }

  
   if (strcasestr(sv1,"prio") != NULL)
       {
       set_sched(0,bv1, bv2);
       }
   else if (strcasestr(sv1,"aff") != NULL)
       {
       set_affinity(0,bv1, bv2);
       }
   else
       {
       prnt("%s is not a valid thread command\n",sv1);
       }
   return 0;
} 
int proccmd_exit(char *buf, int debug, telnet_printfunc_t prnt)
{
   if (debug > 0)
       prnt("process module received %s\n",buf);
   exit_fun("telnet server received exit command\n");
   return 0;
}

int proccmd_restart(char *buf, int debug, telnet_printfunc_t prnt)
{
  if (debug > 0)
       prnt("process module received %s\n", buf);
  configmodule_interface_t *cfg = config_get_if();
  end_configmodule(cfg);
  execvpe(cfg->argv[0], cfg->argv, environ);
  return 0;
}

int proccmd_log(char *buf, int debug, telnet_printfunc_t prnt)
{
int idx1=0;
int idx2=NUM_LOG_LEVEL-1;
char *logsubcmd=NULL;

int s = sscanf(buf,"%ms %i-%i\n",&logsubcmd, &idx1,&idx2);   
   
   if (debug > 0)
       prnt( "proccmd_log received %s\n   s=%i sub command %s\n",buf,s,((logsubcmd==NULL)?"":logsubcmd));
   const mapping *const log_level_names = log_level_names_ptr();
   const mapping *const log_options = log_option_names_ptr();
   const mapping *log_maskmap = log_maskmap_ptr();
   if (s == 1 && logsubcmd != NULL) {
      if (strcasestr(logsubcmd,"online") != NULL) {
          if (strcasestr(buf,"noonline") != NULL) {
   	      set_glog_onlinelog(0);
              prnt("online logging disabled\n",buf);
          } else {
   	      set_glog_onlinelog(1);
              prnt("online logging enabled\n",buf);
          }
      }
      else if (strcasestr(logsubcmd,"show") != NULL) {
        prnt("Available log levels: \n   ");
        for (int i = 0; log_level_names[i].name != NULL; i++)
          prnt("%s ", log_level_names[i].name);
        prnt("\n\n");
        prnt("Available display options: \n   ");
        for (int i = 0; log_options[i].name != NULL; i++)
          prnt("%s ", log_options[i].name);
        prnt("\n\n");
        prnt("Available debug and dump options: \n   ");
        for (int i = 0; log_maskmap[i].name != NULL; i++)
          prnt("%s ", log_maskmap[i].name);
        prnt("\n\n");
        proccmd_show("loglvl", debug, prnt);
        proccmd_show("logopt", debug, prnt);
        proccmd_show("dbgopt", debug, prnt);
      }
      else if (strcasestr(logsubcmd,"help") != NULL) {
          prnt(PROCCMD_LOG_HELP_STRING);
      } else {
          prnt("%s: wrong log command...\n",logsubcmd);
      }
   } else if ( s == 2 && logsubcmd != NULL) {
      char *opt=NULL;
      char *logparam=NULL;
      int  l;
      int optbit;

      l=sscanf(logsubcmd,"%m[^'_']_%ms",&logparam,&opt);
      if (l == 2 && strcmp(logparam,"print") == 0){
        optbit = map_str_to_int(log_options, opt);
        if (optbit < 0) {
          prnt("option %s unknown\n", opt);
        } else {
          if (idx1 > 0)
            SET_LOG_OPTION(optbit);
          else
            CLEAR_LOG_OPTION(optbit);
          proccmd_show("logopt", debug, prnt);
        }
      }
      else if (l == 2 && strcmp(logparam,"debug") == 0){
        int ret = set_log_debug(opt, idx1 > 0);
        if (!ret)
          prnt("module %s unknown\n", opt);
        proccmd_show("dbgopt", debug, prnt);
      }  
       else if (l == 2 && strcmp(logparam,"dump") == 0){
        int ret = set_log_dump(opt, idx1 > 0);
        if (!ret)
          prnt("module %s unknown\n", opt);
        proccmd_show("dump", debug, prnt);
      }       
      if (logparam != NULL) free(logparam);
      if (opt != NULL)      free(opt); 
   } else if ( s == 3 && logsubcmd != NULL) {
      int level, enable,filelog;
      char *tmpstr=NULL;
      char *logparam=NULL;
      int l;

      level = OAILOG_DISABLE - 1;
      filelog = -1;
      enable=-1; 
      l=sscanf(logsubcmd,"%m[^'_']_%m[^'_']",&logparam,&tmpstr);
      if (debug > 0)
          prnt("l=%i, %s %s\n",l,((logparam==NULL)?"\"\"":logparam), ((tmpstr==NULL)?"\"\"":tmpstr));
      if (l ==2 ) {
         if (strcmp(logparam,"level") == 0) {
           level = map_str_to_int(log_level_names, tmpstr);
           if (level < 0) {
             prnt("level %s unknown\n", tmpstr);
             level = OAILOG_DISABLE - 1;
           }
         } else {
             prnt("%s%s unknown log sub command \n",logparam, tmpstr);
         }
      } else if (l ==1 ) {
         if (strcmp(logparam,"enable") == 0) {
            enable=1;
         } else if (strcmp(logparam,"disable") == 0) {
             level=OAILOG_DISABLE;
         } else if (strcmp(logparam,"file") == 0) {
             filelog = 1 ;
         } else if (strcmp(logparam,"nofile") == 0) {
             filelog = 0 ;
         } else {
             prnt("%s%s unknown log sub command \n",logparam, tmpstr);
         }
      } else {
        level = map_str_to_int(log_level_names, tmpstr);
        prnt("%s unknown log sub command \n", logsubcmd);
      }
      if (logparam != NULL) free(logparam);
      if (tmpstr != NULL)   free(tmpstr);
      for (int i=idx1; i<=idx2 ; i++) {
        if (level >= OAILOG_DISABLE)
           set_log(i, level);
        else if ( enable == 1)
          set_log(i, g_log->log_rarely_used[i].savedlevel);
        else if ( filelog == 1 ) {
           set_component_filelog(i);
        } else if ( filelog == 0 ) {
           close_component_filelog(i);
        } 
          
      }
     proccmd_show("loglvl",debug,prnt);
   } else {
       prnt("%s: wrong log command...\n",buf);
   }

   return 0;
} 
/*-------------------------------------------------------------------------------------*/

void add_softmodem_cmds(void)
{
   add_telnetcmd("softmodem",proc_vardef,proc_cmdarray);
}
