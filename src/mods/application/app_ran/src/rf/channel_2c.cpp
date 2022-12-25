/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2022.12
************************************************************************/

#undef  OSET_LOG2_DOMAIN
#define OSET_LOG2_DOMAIN   "channel"

#include <srsran/phy/channel/channel.h>
#include "channel_2c.h"
#include "phy_util.h"

using namespace srsran;


channel_t *channel_create(srsran::channel::args_t &arg)
{
	channel *channel_helper = channel_ptr(new channel(arg, get_nof_rf_channels()));
    return (channel_t *) channel_helper;
}


void channel_run(channel_t *channel_helper, 
                     cf_t* in[SRSRAN_MAX_CHANNELS],
                     cf_t* out[SRSRAN_MAX_CHANNELS],
                     uint32_t                  len,
                     const srsran_timestamp_t& t)
{
    if (channel_helper == NULL) {
        return;
    }
   ((channel *) channel_helper)->run(in, out, len, t);
}

void channel_set_srate(channel_t *channel_helper, uint32_t srate)
{
	 if (channel_helper == NULL) {
		 return;
	 }
	((channel *) channel_helper)->set_srate(srate);
}

void channel_set_signal_power_dBfs(channel_t *channel_helper, float power_dBfs)
{
	 if (channel_helper == NULL) {
		 return;
	 }
	((channel *) channel_helper)->set_signal_power_dBfs(power_dBfs);
}


int channel_destory(channel_t *channel_helper)
{
	 if (channel_helper == NULL) {
		 return OSET_ERROR;
	 }
	 delete reinterpret_cast<channel * >(channel_helper);
	 return OSET_OK;
}

