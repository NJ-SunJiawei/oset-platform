/************************************************************************
 *File name:
 *Description:  stole from srsran
 *
 *Current Version:
 *Author: modity by sunjiawei
 *Date: 2022.12
************************************************************************/


#ifndef SRSRAN_ZUC_H
#define SRSRAN_ZUC_H

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned char u8;
typedef unsigned int  u32;

/* the state registers of LFSR */
typedef struct {
  u32 LFSR_S0;
  u32 LFSR_S1;
  u32 LFSR_S2;
  u32 LFSR_S3;
  u32 LFSR_S4;
  u32 LFSR_S5;
  u32 LFSR_S6;
  u32 LFSR_S7;
  u32 LFSR_S8;
  u32 LFSR_S9;
  u32 LFSR_S10;
  u32 LFSR_S11;
  u32 LFSR_S12;
  u32 LFSR_S13;
  u32 LFSR_S14;
  u32 LFSR_S15;
  /* the registers of F */
  u32 F_R1;
  u32 F_R2;
  /* the outputs of BitReorganization */
  u32 BRC_X0;
  u32 BRC_X1;
  u32 BRC_X2;
  u32 BRC_X3;
} zuc_state_t;

void zuc_initialize(zuc_state_t* state, const u8* k, u8* iv);
void zuc_generate_keystream(zuc_state_t* state, int key_stream_len, u32* p_keystream);

#ifdef __cplusplus
}
#endif

#endif // SRSRAN_ZUC_H
