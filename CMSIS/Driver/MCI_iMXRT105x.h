/* -------------------------------------------------------------------------- 
 * Copyright (c) 2019-2023 Arm Limited (or its affiliates).
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
 * $Revision:    V1.3
 *
 * Project:      MCI Driver Definitions for NXP iMX RT
 * -------------------------------------------------------------------------- */

#ifndef MCI_IMXRT_H__
#define MCI_IMXRT_H__

#include "Driver_MCI.h"

#include "fsl_usdhc.h"                  // NXP::Device:SDK Drivers:sdhc
#include "pin_mux.h"                    // NXP::Board Support:SDK Project Template:project_template

/* Card Detect: identifier SDx_CD must exist in BOARD_INITUSDHC functional group */
#if defined(BOARD_INITUSDHC_SD1_CD_PORT)
  #define MCI0_CD_EN        1
#else
  #define MCI0_CD_EN        0
#endif
#if defined(BOARD_INITUSDHC_SD2_CD_PORT)
  #define MCI1_CD_EN        1
#else
  #define MCI1_CD_EN        0
#endif

/* Write Protect: identifier SDx_WP must exist in BOARD_INITUSDHC functional group */
#if defined(BOARD_INITUSDHC_SD1_WP_PORT)
  #define MCI0_WP_EN        1
#else
  #define MCI0_WP_EN        0
#endif
#if defined(BOARD_INITUSDHC_SD2_WP_PORT)
  #define MCI1_WP_EN        1
#else
  #define MCI1_WP_EN        0
#endif

#if ((MCI0_CD_EN | MCI1_CD_EN | MCI0_WP_EN | MCI1_WP_EN) != 0)
  #include "fsl_gpio.h"                 // NXP::Device:SDK Drivers:gpio
#endif

/* ------ MCI0 ------ */

/* Determine 1-bit or 4-bit data bus width */
#if (defined(BOARD_INITUSDHC_SD1_D1_SIGNAL) && \
     defined(BOARD_INITUSDHC_SD1_D2_SIGNAL) && \
     defined(BOARD_INITUSDHC_SD1_D3_SIGNAL))
  #define MCI0_BUS_WIDTH_4  1U
#else
  #define MCI0_BUS_WIDTH_4  0U
#endif
/* USDCH1 does not support 8-bit bus width */
#define MCI0_BUS_WIDTH_8    0U

#if (MCI0_CD_EN == 1)
#define MCI0_CD_PORT        BOARD_INITUSDHC_SD1_CD_PORT
#define MCI0_CD_PIN         BOARD_INITUSDHC_SD1_CD_PIN
#define MCI0_CD_ACTIVE      0
#endif

#if (MCI0_WP_EN == 1)
#define MCI0_WP_PORT        BOARD_INITUSDHC_SD1_WP_PORT
#define MCI0_WP_PIN         BOARD_INITUSDHC_SD1_WP_PIN
#define MCI0_WP_ACTIVE      1
#endif

/* ------ MCI1 ------ */

/* Determine 1-bit, 4-bit or 8-bit data bus width */
#if   (defined(BOARD_INITUSDHC_SD2_D1_SIGNAL) && \
       defined(BOARD_INITUSDHC_SD2_D2_SIGNAL) && \
       defined(BOARD_INITUSDHC_SD2_D3_SIGNAL) && \
       defined(BOARD_INITUSDHC_SD2_D4_SIGNAL) && \
       defined(BOARD_INITUSDHC_SD2_D5_SIGNAL) && \
       defined(BOARD_INITUSDHC_SD2_D6_SIGNAL) && \
       defined(BOARD_INITUSDHC_SD2_D7_SIGNAL))
  #define MCI1_BUS_WIDTH_4  1U
  #define MCI1_BUS_WIDTH_8  1U
#elif (defined(BOARD_INITUSDHC_SD2_D1_SIGNAL) && \
       defined(BOARD_INITUSDHC_SD2_D2_SIGNAL) && \
       defined(BOARD_INITUSDHC_SD2_D3_SIGNAL))
  #define MCI1_BUS_WIDTH_4  1U
  #define MCI1_BUS_WIDTH_8  0U
#else
  #define MCI1_BUS_WIDTH_4  0U
  #define MCI1_BUS_WIDTH_8  0U
#endif

#if (MCI1_CD_EN == 1)
#define MCI1_CD_PORT        BOARD_INITUSDHC_SD2_CD_PORT
#define MCI1_CD_PIN         BOARD_INITUSDHC_SD2_CD_PIN
#define MCI1_CD_ACTIVE      0
#endif

#if (MCI1_WP_EN == 1)
#define MCI1_WP_PORT        BOARD_INITUSDHC_SD2_WP_PORT
#define MCI1_WP_PIN         BOARD_INITUSDHC_SD2_WP_PIN
#define MCI1_WP_ACTIVE      1
#endif

/* ------ Common ------ */

/* Peripheral reset timeout in loop cycles */
#define MCI_RESET_TIMEOUT   1000000

/* Peripheral interrupt enable bitmask */
#define SDHC_IRQ_EN_MASK   (SDHC_IRQSTATEN_CCSEN_MASK   | \
                            SDHC_IRQSTATEN_TCSEN_MASK   | \
                            SDHC_IRQSTATEN_BWRSEN_MASK  | \
                            SDHC_IRQSTATEN_BRRSEN_MASK  | \
                            SDHC_IRQSTATEN_CTOESEN_MASK | \
                            SDHC_IRQSTATEN_CCESEN_MASK  | \
                            SDHC_IRQSTATEN_CEBESEN_MASK | \
                            SDHC_IRQSTATEN_CIESEN_MASK  | \
                            SDHC_IRQSTATEN_DTOESEN_MASK | \
                            SDHC_IRQSTATEN_DCESEN_MASK  | \
                            SDHC_IRQSTATEN_DEBESEN_MASK | \
                            SDHC_IRQSTATEN_DMAESEN_MASK )

/* Peripheral interrupt clear bitmask */
#define SDHC_IRQ_CLR_MASK  (0x117F01FF)


/* Driver flag definitions */
#define MCI_INIT      ((uint8_t)0x01)   /* MCI initialized            */
#define MCI_POWER     ((uint8_t)0x02)   /* MCI powered on             */
#define MCI_SETUP     ((uint8_t)0x04)   /* MCI configured             */
#define MCI_RESP_LONG ((uint8_t)0x08)   /* Long response expected     */
#define MCI_CMD       ((uint8_t)0x10)   /* Command response expected  */
#define MCI_DATA      ((uint8_t)0x20)   /* Transfer response expected */

#define MCI_RESPONSE_EXPECTED_Msk (ARM_MCI_RESPONSE_SHORT      | \
                                   ARM_MCI_RESPONSE_SHORT_BUSY | \
                                   ARM_MCI_RESPONSE_LONG)

typedef struct MCI_Io {
  GPIO_Type *port;
  uint32_t   pin;
  uint32_t   active;
} MCI_IO;

/* MCI Driver State Definition */
typedef struct MCI_Ctrl {
  ARM_MCI_SignalEvent_t     cb_event;   /* Driver event callback function     */
  ARM_MCI_STATUS volatile   status;     /* Driver status                      */
  uint32_t volatile        *response;   /* Pointer to response buffer         */
  usdhc_transfer_t          xfer;       /* Transfer control structures        */
  usdhc_handle_t            h;          /* SDHC driver handle                 */
  usdhc_data_t              data;
  usdhc_command_t           cmd;
  uint8_t volatile          flags;      /* Driver state flags                 */
  uint8_t                   rsvd[3];    /* Reserved */
} MCI_CTRL;

typedef const struct MCI_Resources {
  ARM_MCI_CAPABILITIES      capab;      /* Driver capabilities                 */
  MCI_CTRL                 *ctrl;       /* Run-Time control structure          */
  USDHC_Type               *reg;        /* USDHC peripheral register interface */
  uint32_t/*IRQn_Type*/     irqn;       /* USDHC peripheral IRQ number (NVIC)  */
  MCI_IO                   *cd;         /* Card Detect pin config info         */
  MCI_IO                   *wp;         /* Write Protect pin config info       */
  usdhc_adma_config_t       dma;        /* DMA config info                     */
} MCI_RESOURCES;

/* Exported drivers */
#if (DRIVER_MCI0)
  extern ARM_DRIVER_MCI Driver_MCI0;
#endif
#if (DRIVER_MCI1)
  extern ARM_DRIVER_MCI Driver_MCI1;
#endif

#endif /* MCI_IMXRT_H__ */
