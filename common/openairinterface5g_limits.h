#ifndef OPENAIRINTERFACE5G_LIMITS_H_
#define OPENAIRINTERFACE5G_LIMITS_H_

#        define NUMBER_OF_eNB_MAX 1
#        define NUMBER_OF_gNB_MAX 1
#        define NUMBER_OF_RU_MAX 2
#        define NUMBER_OF_NR_RU_MAX 2
#        define NUMBER_OF_UCI_MAX 16
#        define NUMBER_OF_ULSCH_MAX 8
#        define NUMBER_OF_DLSCH_MAX 8 
#        define NUMBER_OF_SRS_MAX 16
#        define NUMBER_OF_NR_ULSCH_MAX 8
#        define NUMBER_OF_NR_DLSCH_MAX 8
#        define NUMBER_OF_NR_UCI_MAX 16
#        define NUMBER_OF_NR_SRS_MAX 16
#        define NUMBER_OF_NR_CSIRS_MAX 16
#        define NUMBER_OF_SCH_STATS_MAX 16

#        define NUMBER_OF_NR_SCH_STATS_MAX 16

#        define NUMBER_OF_NR_PUCCH_MAX 16
#        define NUMBER_OF_NR_PDCCH_MAX 16

#define MAX_MANAGED_ENB_PER_MOBILE  2
#define MAX_MANAGED_GNB_PER_MOBILE  2

#        ifndef PHYSIM
#            ifndef UE_EXPANSION
#                    define NUMBER_OF_UE_MAX 40
#                    define NUMBER_OF_NR_UE_MAX 4
#                    define NUMBER_OF_CONNECTED_eNB_MAX 1
#                    define NUMBER_OF_CONNECTED_gNB_MAX 1
#            else
#                    define NUMBER_OF_UE_MAX 256
#                    define NUMBER_OF_CONNECTED_eNB_MAX 1
#                    define NUMBER_OF_CONNECTED_gNB_MAX 1
#            endif
#        else
#                    define NUMBER_OF_UE_MAX 4
#                    define NUMBER_OF_NR_UE_MAX 4
#                    define NUMBER_OF_CONNECTED_eNB_MAX 1
#                    define NUMBER_OF_CONNECTED_gNB_MAX 1
#        endif

#endif /* OPENAIRINTERFACE5G_LIMITS_H_ */
