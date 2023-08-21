/************************************************************************
 *File name:
 *Description:  stole from srsran
 *
 *Current Version:
 *Author: modity by sunjiawei
 *Date: 2022.12
************************************************************************/

#ifndef SRSRAN_SECURITY_H
#define SRSRAN_SECURITY_H

/******************************************************************************
 * Common security header - wraps ciphering/integrity check algorithms.
 *****************************************************************************/

#include "lib/common/security_private.h"
#include <vector>
#include <array>

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * Key Generation
 *****************************************************************************/

int kdf_common(const uint8_t fc, const uint8_t* key, const size_t key_len, const std::vector<uint8_t>& P, uint8_t* output);
int kdf_common(const uint8_t                       fc,
					const uint8_t*				   key,
					const size_t				   key_len,
					const std::vector<uint8_t>&    P0,
					const std::vector<uint8_t>&    P1,
					uint8_t*                       output);
int kdf_common(const uint8_t                       fc,
					const uint8_t*				   key,
					const size_t				   key_len,
					const std::vector<uint8_t>&    P0,
					const std::vector<uint8_t>&    P1,
					const std::vector<uint8_t>&    P3,
					uint8_t*                       output);

uint8_t security_generate_k_asme(const uint8_t* ck,
                                 const uint8_t* ik,
                                 const uint8_t* ak_xor_sqn,
                                 const uint16_t mcc,
                                 const uint16_t mnc,
                                 uint8_t*       k_asme);

uint8_t security_generate_k_ausf(const uint8_t* ck,
                                 const uint8_t* ik,
                                 const uint8_t* ak_xor_sqn,
                                 const char*    serving_network_name,
                                 uint8_t*       k_ausf);

uint8_t security_generate_k_amf(const uint8_t* k_seaf,
                                const char*    supi_,
                                const uint8_t* abba_,
                                const uint32_t abba_len,
                                uint8_t*       k_amf);

uint8_t security_generate_k_seaf(const uint8_t* k_ausf, const char* serving_network_name, uint8_t* k_seaf);

uint8_t security_generate_k_gnb(const uint8_t* k_amf, const uint32_t nas_count, uint8_t* k_gnb);

uint8_t security_generate_k_enb(const uint8_t* k_asme, const uint32_t nas_count, uint8_t* k_enb);

uint8_t security_generate_k_nb_star_common(uint8_t        fc,
                                           const uint8_t* k_enb,
                                           const uint32_t pci_,
                                           const uint32_t earfcn_,
                                           uint8_t*       k_enb_star);

uint8_t
security_generate_k_enb_star(const uint8_t* k_enb, const uint32_t pci, const uint32_t earfcn, uint8_t* k_enb_star);

uint8_t
security_generate_k_gnb_star(const uint8_t* k_gnb, const uint32_t pci_, const uint32_t dl_arfcn_, uint8_t* k_gnb_star);

uint8_t security_generate_nh(const uint8_t* k_asme, const uint8_t* sync, uint8_t* nh);

uint8_t security_generate_k_nas(const uint8_t*                    k_asme,
                                const CIPHERING_ALGORITHM_ID_ENUM enc_alg_id,
                                const INTEGRITY_ALGORITHM_ID_ENUM int_alg_id,
                                uint8_t*                          k_nas_enc,
                                uint8_t*                          k_nas_int);

uint8_t security_generate_k_nas_5g(const uint8_t*                    k_amf,
                                   const CIPHERING_ALGORITHM_ID_ENUM enc_alg_id,
                                   const INTEGRITY_ALGORITHM_ID_ENUM int_alg_id,
                                   uint8_t*                          k_nas_enc,
                                   uint8_t*                          k_nas_int);

uint8_t security_generate_k_rrc(const uint8_t*                    k_enb,
                                const CIPHERING_ALGORITHM_ID_ENUM enc_alg_id,
                                const INTEGRITY_ALGORITHM_ID_ENUM int_alg_id,
                                uint8_t*                          k_rrc_enc,
                                uint8_t*                          k_rrc_int);

uint8_t security_generate_k_up(const uint8_t*                    k_enb,
                               const CIPHERING_ALGORITHM_ID_ENUM enc_alg_id,
                               const INTEGRITY_ALGORITHM_ID_ENUM int_alg_id,
                               uint8_t*                          k_up_enc,
                               uint8_t*                          k_up_int);

uint8_t security_generate_k_nr_rrc(const uint8_t*                    k_gnb,
                                   const CIPHERING_ALGORITHM_ID_ENUM enc_alg_id,
                                   const INTEGRITY_ALGORITHM_ID_ENUM int_alg_id,
                                   uint8_t*                          k_rrc_enc,
                                   uint8_t*                          k_rrc_int);

uint8_t security_generate_k_nr_up(const uint8_t*                    k_gnb,
                                  const CIPHERING_ALGORITHM_ID_ENUM enc_alg_id,
                                  const INTEGRITY_ALGORITHM_ID_ENUM int_alg_id,
                                  uint8_t*                          k_up_enc,
                                  uint8_t*                          k_up_int);

uint8_t security_generate_sk_gnb(const uint8_t* k_enb, const uint16_t scg_count, uint8_t* sk_gnb);

uint8_t security_generate_res_star(const uint8_t* ck,
                                   const uint8_t* ik,
                                   const char*    serving_network_name,
                                   const uint8_t* rand,
                                   const uint8_t* res,
                                   const size_t   res_len,
                                   uint8_t*       res_star);
/******************************************************************************
 * Integrity Protection
 *****************************************************************************/
uint8_t security_128_eia1(const uint8_t* key,
                          uint32_t       count,
                          uint32_t       bearer,
                          uint8_t        direction,
                          uint8_t*       msg,
                          uint32_t       msg_len,
                          uint8_t*       mac);

uint8_t security_128_eia2(const uint8_t* key,
                          uint32_t       count,
                          uint32_t       bearer,
                          uint8_t        direction,
                          uint8_t*       msg,
                          uint32_t       msg_len,
                          uint8_t*       mac);

uint8_t security_128_eia3(const uint8_t* key,
                          uint32_t       count,
                          uint32_t       bearer,
                          uint8_t        direction,
                          uint8_t*       msg,
                          uint32_t       msg_len,
                          uint8_t*       mac);

uint8_t security_md5(const uint8_t* input, size_t len, uint8_t* output);

/******************************************************************************
 * Encryption / Decryption
 *****************************************************************************/
uint8_t security_128_eea1(uint8_t* key,
                          uint32_t count,
                          uint8_t  bearer,
                          uint8_t  direction,
                          uint8_t* msg,
                          uint32_t msg_len,
                          uint8_t* msg_out);

uint8_t security_128_eea2(uint8_t* key,
                          uint32_t count,
                          uint8_t  bearer,
                          uint8_t  direction,
                          uint8_t* msg,
                          uint32_t msg_len,
                          uint8_t* msg_out);

uint8_t security_128_eea3(uint8_t* key,
                          uint32_t count,
                          uint8_t  bearer,
                          uint8_t  direction,
                          uint8_t* msg,
                          uint32_t msg_len,
                          uint8_t* msg_out);

/******************************************************************************
 * Authentication
 *****************************************************************************/
uint8_t compute_opc(uint8_t* k, uint8_t* op, uint8_t* opc);

uint8_t security_milenage_f1(uint8_t* k, uint8_t* op, uint8_t* rand, uint8_t* sqn, uint8_t* amf, uint8_t* mac_a);

uint8_t security_milenage_f1_star(uint8_t* k, uint8_t* op, uint8_t* rand, uint8_t* sqn, uint8_t* amf, uint8_t* mac_s);

uint8_t
security_milenage_f2345(uint8_t* k, uint8_t* op, uint8_t* rand, uint8_t* res, uint8_t* ck, uint8_t* ik, uint8_t* ak);

uint8_t security_milenage_f5_star(uint8_t* k, uint8_t* op, uint8_t* rand, uint8_t* ak);

int security_xor_f2345(uint8_t* k, uint8_t* rand, uint8_t* res, uint8_t* ck, uint8_t* ik, uint8_t* ak);
int security_xor_f1(uint8_t* k, uint8_t* rand, uint8_t* sqn, uint8_t* amf, uint8_t* mac_a);


#ifdef __cplusplus
}
#endif

#endif // SRSRAN_SECURITY_H
