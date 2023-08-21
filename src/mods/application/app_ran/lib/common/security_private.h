/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.04
************************************************************************/

#ifndef SECURITY_PRIVATE_H_
#define SECURITY_PRIVATE_H_

#include "lib/common/common.h"

#ifdef __cplusplus
extern "C" {
#endif

#define AKA_RAND_LEN 16
#define AKA_AUTN_LEN 16
#define AKA_AUTS_LEN 14
#define RES_MAX_LEN 16
#define MAC_LEN 8
#define IK_LEN 16
#define CK_LEN 16
#define AK_LEN 6
#define SQN_LEN 6

#define KEY_LEN 32


typedef enum {
  CIPHERING_ALGORITHM_ID_EEA0 = 0,
  CIPHERING_ALGORITHM_ID_128_EEA1,
  CIPHERING_ALGORITHM_ID_128_EEA2,
  CIPHERING_ALGORITHM_ID_128_EEA3,
  CIPHERING_ALGORITHM_ID_N_ITEMS,
} CIPHERING_ALGORITHM_ID_ENUM;
static const char ciphering_algorithm_id_text[CIPHERING_ALGORITHM_ID_N_ITEMS][20] = {"EEA0",
                                                                                     "128-EEA1",
                                                                                     "128-EEA2",
                                                                                     "128-EEA3"};

typedef enum {
  INTEGRITY_ALGORITHM_ID_EIA0 = 0,
  INTEGRITY_ALGORITHM_ID_128_EIA1,
  INTEGRITY_ALGORITHM_ID_128_EIA2,
  INTEGRITY_ALGORITHM_ID_128_EIA3,
  INTEGRITY_ALGORITHM_ID_N_ITEMS,
} INTEGRITY_ALGORITHM_ID_ENUM;
static const char integrity_algorithm_id_text[INTEGRITY_ALGORITHM_ID_N_ITEMS][20] = {"EIA0",
                                                                                     "128-EIA1",
                                                                                     "128-EIA2",
                                                                                     "128-EIA3"};

typedef enum {
  CIPHERING_ALGORITHM_ID_NR_NEA0 = 0,
  CIPHERING_ALGORITHM_ID_NR_128_NEA1,
  CIPHERING_ALGORITHM_ID_NR_128_NEA2,
  CIPHERING_ALGORITHM_ID_NR_128_NEA3,
  CIPHERING_ALGORITHM_ID_NR_N_ITEMS,
} CIPHERING_ALGORITHM_ID_NR_ENUM;
static const char ciphering_algorithm_id_nr_text[CIPHERING_ALGORITHM_ID_N_ITEMS][20] = {"NEA0",
                                                                                        "128-NEA1",
                                                                                        "128-NEA2",
                                                                                        "128-NEA3"};
typedef enum {
  INTEGRITY_ALGORITHM_ID_NR_NIA0 = 0,
  INTEGRITY_ALGORITHM_ID_NR_128_NIA1,
  INTEGRITY_ALGORITHM_ID_NR_128_NIA2,
  INTEGRITY_ALGORITHM_ID_NR_128_NIA3,
  INTEGRITY_ALGORITHM_ID_NR_N_ITEMS,
} INTEGRITY_ALGORITHM_ID_NR_ENUM;
static const char integrity_algorithm_id_nr_text[INTEGRITY_ALGORITHM_ID_N_ITEMS][20] = {"NIA0",
                                                                                        "128-NIA1",
                                                                                        "128-NIA2",
                                                                                        "128-NIA3"};
typedef enum {
  SECURITY_DIRECTION_UPLINK   = 0,
  SECURITY_DIRECTION_DOWNLINK = 1,
  SECURITY_DIRECTION_N_ITEMS,
} security_direction_t;
static const char security_direction_text[INTEGRITY_ALGORITHM_ID_N_ITEMS][20] = {"Uplink", "Downlink"};

typedef  uint8_t as_key_t[32];
struct k_enb_context_t {
  as_key_t k_enb;
  as_key_t nh;
  bool     is_first_ncc;
  uint32_t ncc;
};

struct k_gnb_context_t {
  as_key_t k_gnb;
  as_key_t sk_gnb;
};

struct as_security_config_t {
  as_key_t                    k_rrc_int;
  as_key_t                    k_rrc_enc;
  as_key_t                    k_up_int;
  as_key_t                    k_up_enc;
  INTEGRITY_ALGORITHM_ID_ENUM integ_algo;
  CIPHERING_ALGORITHM_ID_ENUM cipher_algo;
};

struct nr_as_security_config_t {
  as_key_t                       k_nr_rrc_int;
  as_key_t                       k_nr_rrc_enc;
  as_key_t                       k_nr_up_int;
  as_key_t                       k_nr_up_enc;
  INTEGRITY_ALGORITHM_ID_NR_ENUM integ_algo;
  CIPHERING_ALGORITHM_ID_NR_ENUM cipher_algo;
};


#ifdef __cplusplus
}
#endif

#endif
