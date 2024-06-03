/* -------------------------------------------------------------------------- 
 * Copyright (c) 2018-2023 Arm Limited (or its affiliates).
 * All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *
 * $Date:        4. October 2023
 * $Revision:    V1.5
 *
 * Project:      Ethernet Media Access (MAC) Definitions for NXP iMXRT1050
 * -------------------------------------------------------------------------- */

#ifndef EMAC_IMXRT105x_H__
#define EMAC_IMXRT105x_H__

#include <string.h>

#include "Driver_ETH_MAC.h"

#include "fsl_enet.h"                   // NXP::Device:SDK Drivers:enet
#include "pin_mux.h"                    // NXP::Board Support:SDK Project Template:project_template

/* Determine Media-Independent interface mode */
#if (defined(BOARD_INITENET_ENET_TXD2_SIGNAL) && defined(BOARD_INITENET_ENET_TXD3_SIGNAL) && \
     defined(BOARD_INITENET_ENET_RXD2_SIGNAL) && defined(BOARD_INITENET_ENET_RXD3_SIGNAL))
#define EMAC_MII_MODE      ARM_ETH_INTERFACE_MII
#else
#define EMAC_MII_MODE      ARM_ETH_INTERFACE_RMII
#endif

/* EMAC Driver state flags */
#define EMAC_FLAG_INIT     (1U << 0)        // Driver initialized
#define EMAC_FLAG_POWER    (1U << 1)        // Driver power on


/* EMAC Driver Control Information */
typedef struct _EMAC_INFO {
  ARM_ETH_MAC_SignalEvent_t cb_event;       // Event callback
  volatile uint8_t          flags;          // Control and state flags
  uint8_t                   addr[6];        // Physical address
  uint32_t                  pclk;           // ENET peripheral clock
  enet_config_t             cfg;            // Configuration address
  enet_buffer_config_t      desc_cfg;       // DMA descriptor configuration
  enet_handle_t             h;              // ENET handle
} EMAC_INFO;

/* Global functions and variables exported by driver .c module */
extern ARM_DRIVER_ETH_MAC Driver_ETH_MAC0;

#endif /* EMAC_IMXRT105x_H__ */
