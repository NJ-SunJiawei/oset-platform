/**
 * Copyright 2013-2022 Software Radio Systems Limited
 *
 * This file is part of srsRAN.
 *
 * srsRAN is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * srsRAN is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * A copy of the GNU Affero General Public License can be found in
 * the LICENSE file in the top-level directory of this distribution
 * and at http://www.gnu.org/licenses/.
 *
 */

/******************************************************************************
 * File:        phy_logger.h
 * Description: Interface for logging output
 *****************************************************************************/

#ifndef SRSRAN_PHY_LOGGER_H
#define SRSRAN_PHY_LOGGER_H

#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "oset-core.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus
typedef enum {LOG_LEVEL_ERROR_S= 3, LOG_LEVEL_INFO_S = 6, LOG_LEVEL_DEBUG_S = 7,} phy_logger_level_t;

#define srsran_phy_log_print(log_level, fmt, ...) \
	    oset_log2_printf(OSET_CHANNEL_ID_LOG, OSET_LOG2_DOMAIN, __FILE__, __OSET_FUNC__, __LINE__, NULL, log_level, fmt ,##__VA_ARGS__)

void srsran_phy_log_register_handler(bool oset_print);

#ifdef __cplusplus
}
#endif // C++

#endif // SRSRAN_PHY_LOGGER_H
