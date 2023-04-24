/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2022.12
************************************************************************/

#ifndef PHY_H_
#define PHY_H_

#include "gnb_config_parser.h"
#include "phy/phy_nr_config.h"
#include "phy/slot_worker.h"
#include "phy/txrx.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  uint32_t				 nof_phy_threads;//Ensure tti sequence just alloc 1
  uint32_t				 nof_prach_workers;
}phy_work_args_t;


typedef struct {
	//oset_thread_mutex_t	 *grant_mutex;
	cvector_vector_t(phy_cell_cfg_nr_t) cell_list_nr;//std::vector<phy_cell_cfg_nr_t>
	//oset_apr_mutex_t	     *cell_gain_mutex;
	srsran_cfr_cfg_t        cfr_config;
	//srsran_refsignal_dmrs_pusch_cfg_t dmrs_pusch_cfg;//???4G

	phy_args_t              params;
}phy_common;

typedef struct phy_manager_s{
	oset_apr_memory_pool_t *app_pool;
	oset_apr_mutex_t	   *mutex;
	oset_apr_thread_cond_t *cond;

	//txrx				   tx_rx;
	//srsran_prach_cfg_t     prach_cfg; //???4G
	common_cfg_t	       common_cfg;  //from rrc layer config
	phy_common			   workers_common;
	phy_work_args_t        worker_args;
	slot_worker_args_t     slot_args;
	oset_thread_pool_t     *th_pools;
}phy_manager_t;
phy_manager_t *phy_manager_self(void);

int phy_init(void);
int phy_destory(void);


#ifdef __cplusplus
}
#endif

#endif
