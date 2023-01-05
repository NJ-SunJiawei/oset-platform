/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2022.12
************************************************************************/

#ifndef CHANNEL_2C_H_
#define CHANNEL_2C_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "srsran/srsran.h"

typedef struct channel_t  channel_t;

channel_t *channel_create(srsran::channel::args_t &arg);
void channel_run(channel_t *channel_helper, 
                     cf_t* in[SRSRAN_MAX_CHANNELS],
                     cf_t* out[SRSRAN_MAX_CHANNELS],
                     uint32_t                  len,
                     const srsran_timestamp_t& t);
void channel_set_srate(channel_t *channel_helper, uint32_t srate);
void channel_set_signal_power_dBfs(channel_t *channel_helper, float power_dBfs);
int channel_destory(channel_t *channel_helper);


#ifdef __cplusplus
}
#endif

#endif
