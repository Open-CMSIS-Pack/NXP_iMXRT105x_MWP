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
 * $Revision:    V1.9
 *
 * Driver:       Driver_CAN1/2
 * Configured:   pin/clock configuration via MCUXpresso Config Tools v7.0
 * Project:      CMSIS CAN Driver for NXP i.MX RT 105x Series
 * --------------------------------------------------------------------------
 * Use the following configuration settings in the middleware component
 * to connect to this driver.
 *
 *   Configuration Setting                 Value   CAN Interface
 *   ---------------------                 -----   -------------
 *   Connect to hardware via Driver_CAN# = 1       use FLEXCAN1
 *   Connect to hardware via Driver_CAN# = 2       use FLEXCAN2
 * --------------------------------------------------------------------------
 * Defines used for driver configuration (at compile time):
 *   CAN_CLOCK_TOLERANCE:               defines maximum allowed clock tolerance in 1/1024 steps
 *     - default value:                 15  (approx. 1.5 %)
 *
 *   CAN1 controller configuration:
 *   CAN1_RX_FIFO_EN:                   Rx FIFO enable
 *                                      (default=enabled)
 *   CAN1_RX_FIFO_ID_FILT_ELEM_NUM:     Number of receive FIFO ID filter elements
 *                                      (default=64, min=8, max=128)
 *
 *   CAN2 controller configuration:
 *   CAN2_RX_FIFO_EN:                   Rx FIFO enable
 *                                      (default=enabled)
 *   CAN2_RX_FIFO_ID_FILT_ELEM_NUM:     Number of receive FIFO ID filter elements
 *                                      (default=64, min=8, max=128)
 * Notes:
 *  - ARM_CAN_OBJ_RX_RTR_TX_DATA object type not supported
 *  - DMA not supported
 * -------------------------------------------------------------------------- */

/* History:
 *  Version 1.9
 *    Added volatile qualifier to volatile variables
 *  Version 1.8
 *    Updated StartReceive function to work with FSL_FLEXCAN driver v2.9.2 or newer
 *  Version 1.7
 *    Corrected SetMode function to update unit state
 *  Version 1.6
 *    Corrected ObjectSetFilter function (uninitialized 'mask' variable value)
 *  Version 1.5
 *    Added support for maskable filtering
 *    Corrected index handling for Rx mailbox objects
 *    Corrected filter removal
 *  Version 1.4
 *    Added define instances (DRIVER_CAN1, DRIVER_CAN2)
 *  Version 1.3
 *    Added signaling of overrun events
 *  Version 1.2
 *    Changed Tx mailbox handling to support Errata 5829 correction in 
 *    fsl_flaxcan.c module
 *    (Tx mailbox is mapped to fsl flexcan as 1 higher than actual, because 
 *     first tx mailbox is reserved for errata workaround)
 *  Version 1.1
 *    Corrected SetMode function (endless wait on FRZACK bit)
 *  Version 1.0
 *    Initial release
 */

/*! \page evkb_imxrt1050_can CMSIS-Driver for CAN Interface

<b>CMSIS-Driver for CAN Interface Setup</b>

The CMSIS-Driver for CAN Interface requires:
  - CAN Pins
  - CAN Clock
 
Valid pin settings for <b>CAN</b> peripheral on <b>EVKB-IMXRT1050</b> evaluation board are listed in the table below:
 
|  #  | Peripheral | Signal     | Route to      | Direction     | Software Input On | Speed      |
|:----|:-----------|:-----------|:--------------|:--------------|:------------------|:-----------|
| H14 | CAN2       | TX         | GPIO_AD_B0_14 | Output        |     Enabled       | low(50MHz) |
| L10 | CAN2       | RX         | GPIO_AD_B0_15 | Input         |     Enabled       | low(50MHz) |
 
The rest of the settings can be at the default (reset) value.

For other boards or custom hardware, refer to the hardware schematics to reflect correct setup values.
 
In the \ref config_pinclock "MCUXpresso Config Tools", make sure that the following pin and clock settings are made (enter
the values that are shown in <em>italics</em>):
 
-# Under the \b Functional \b Group entry in the toolbar, select <b>Board_InitCAN</b>
-# Modify Routed Pins configuration to match the settings listed in the table above
-# Click on the flag next to <b>Functional Group BOARD_InitCAN</b> to enable calling that function from initialization code
-# Go to <b>Tools - Clocks</b>
-# Go to <b>Views - Clocks Diagram</b> and check that \b CAN clock is set to <em>40 MHz</em>
-# Click on <b>Update Project</b> button to update source files

\note
Driver limitations:
  - ARM_CAN_OBJ_RX_RTR_TX_DATA object type not supported
  - DMA not supported
*/

/*! \cond */

#include <stdint.h>
#include <string.h>

#include "Driver_CAN.h"

#include "RTE_Components.h"

#include "fsl_clock.h"
#include "fsl_flexcan.h"

/* Define instances */
#ifndef DRIVER_CAN1
  #define DRIVER_CAN1             1
#endif

#ifndef DRIVER_CAN2
  #define DRIVER_CAN2             0
#endif

// Driver configuration through compile-time definitions ------

// Maximum allowed clock tolerance in 1/1024 steps
#ifndef CAN_CLOCK_TOLERANCE
#define CAN_CLOCK_TOLERANCE            (15U)    // 15/1024 approx. 1.5 %
#endif

// CAN1 configuration
#ifndef CAN1_RX_FIFO_EN
#define CAN1_RX_FIFO_EN                (1U)
#endif
#if    (CAN1_RX_FIFO_EN == 1U)
#ifndef CAN1_RX_FIFO_ID_FILT_ELEM_NUM
#define CAN1_RX_FIFO_ID_FILT_ELEM_NUM  (64U)
#endif
#if    (CAN1_RX_FIFO_ID_FILT_ELEM_NUM <  8U)
#error  Not enough Rx FIFO ID Filter Elements defined for CAN1, minimum is 8 !!!
#elif  (CAN1_RX_FIFO_ID_FILT_ELEM_NUM > 128U)
#error  Too many Rx FIFO ID Filter Elements defined for CAN1, maximum is 128 !!!
#endif
#endif

// CAN2 configuration
#ifndef CAN2_RX_FIFO_EN         
#define CAN2_RX_FIFO_EN                (1U)
#endif
#if    (CAN2_RX_FIFO_EN == 1U)
#ifndef CAN2_RX_FIFO_ID_FILT_ELEM_NUM
#define CAN2_RX_FIFO_ID_FILT_ELEM_NUM  (64U)
#endif
#if    (CAN2_RX_FIFO_ID_FILT_ELEM_NUM <  8U)
#error  Not enough Rx FIFO ID Filter Elements defined for CAN2, minimum is 8 !!!
#elif  (CAN2_RX_FIFO_ID_FILT_ELEM_NUM > 128U)
#error  Too many Rx FIFO ID Filter Elements defined for CAN2, maximum is 128 !!!
#endif
#endif

// Compile-time calculations
#define CAN1_RX_FIFO_MBX_NUM           (CAN1_RX_FIFO_EN*(6U+((CAN1_RX_FIFO_ID_FILT_ELEM_NUM+3U)/4U)))
#if    (CAN1_RX_FIFO_MBX_NUM != 0)
#define CAN1_RX_FIFO_OBJ_NUM           (1U)
#define CAN1_RX_MBX_OBJ_OFS            (CAN1_RX_FIFO_MBX_NUM - 1U)
#else
#define CAN1_RX_FIFO_OBJ_NUM           (0U)
#define CAN1_RX_MBX_OBJ_OFS            (0U)
#endif
#define CAN1_MBX_OBJ_NUM               (64U-CAN1_RX_FIFO_MBX_NUM)
#define CAN1_TOT_OBJ_NUM               (CAN1_RX_FIFO_OBJ_NUM+CAN1_MBX_OBJ_NUM)

#define CAN2_RX_FIFO_MBX_NUM           (CAN2_RX_FIFO_EN*(6U+((CAN2_RX_FIFO_ID_FILT_ELEM_NUM+3U)/4U)))
#if    (CAN2_RX_FIFO_MBX_NUM != 0)
#define CAN2_RX_FIFO_OBJ_NUM           (1U)
#define CAN2_RX_MBX_OBJ_OFS            (CAN2_RX_FIFO_MBX_NUM - 1U)
#else
#define CAN2_RX_FIFO_OBJ_NUM           (0U)
#define CAN2_RX_MBX_OBJ_OFS            (0U)
#endif
#define CAN2_MBX_OBJ_NUM               (64U-CAN2_RX_FIFO_MBX_NUM)
#define CAN2_TOT_OBJ_NUM               (CAN2_RX_FIFO_OBJ_NUM+CAN2_MBX_OBJ_NUM)


// CAN Driver ******************************************************************

#define ARM_CAN_DRV_VERSION ARM_DRIVER_VERSION_MAJOR_MINOR(1,9) // CAN driver version

// Driver Version
static const ARM_DRIVER_VERSION can_driver_version = { ARM_CAN_API_VERSION, ARM_CAN_DRV_VERSION };

// Driver Capabilities
static const ARM_CAN_CAPABILITIES can_driver_capabilities[2] = {
#if (DRIVER_CAN1 == 1U)
  {                                     // CAN1 driver capabilities
    CAN1_TOT_OBJ_NUM,                   // Number of CAN Objects available
    1U,                                 // Supports reentrant calls to ARM_CAN_MessageSend, ARM_CAN_MessageRead, ARM_CAN_ObjectConfigure and abort message sending used by ARM_CAN_Control.
    0U,                                 // Does not support CAN with flexible data-rate mode (CAN_FD)
    0U,                                 // Does not support restricted operation mode
    1U,                                 // Supports bus monitoring mode
    1U,                                 // Supports internal loopback mode
    0U,                                 // Supports external loopback mode
    0U                                  // Reserved field = 0
  },
#else
  { 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U },
#endif
#if (DRIVER_CAN2 == 1U)
  {                                     // CAN2 driver capabilities
    CAN2_TOT_OBJ_NUM,                   // Number of CAN Objects available
    1U,                                 // Supports reentrant calls to ARM_CAN_MessageSend, ARM_CAN_MessageRead, ARM_CAN_ObjectConfigure and abort message sending used by ARM_CAN_Control.
    0U,                                 // Does not support CAN with flexible data-rate mode (CAN_FD)
    0U,                                 // Does not support restricted operation mode
    1U,                                 // Supports bus monitoring mode
    1U,                                 // Supports internal loopback mode
    0U,                                 // Supports external loopback mode
    0U                                  // Reserved field = 0
  }
#else
  { 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U }
#endif
};

// Object Capabilities
static const ARM_CAN_OBJ_CAPABILITIES can_object_capabilities_rx_fifo = { 
                                        // Rx FIFO object capabilities
  0U,                                   // Object does not support transmission
  1U,                                   // Object supports reception
  0U,                                   // Object does not support RTR reception and automatic Data transmission
  0U,                                   // Object does not support RTR transmission and automatic Data reception
  1U,                                   // Object allows assignment of multiple filters to it
  1U,                                   // Object supports exact identifier filtering
  0U,                                   // Object does not support range identifier filtering
  1U,                                   // Object supports mask identifier filtering
  6U,                                   // Object can buffer 6 messages
  0U                                    // Reserved field = 0
};
static const ARM_CAN_OBJ_CAPABILITIES can_object_capabilities_mbx = {
                                        // Mailbox object capabilities
  1U,                                   // Object supports transmission
  1U,                                   // Object supports reception
  0U,                                   // Object does not support RTR reception and automatic Data transmission
  0U,                                   // Object does not support RTR transmission and automatic Data reception
  0U,                                   // Object does not allow assignment of multiple filters to it
  1U,                                   // Object supports exact identifier filtering
  0U,                                   // Object does not support range identifier filtering
  1U,                                   // Object supports mask identifier filtering
  1U,                                   // Object can buffer 1 messages
  0U                                    // Reserved field = 0
};

typedef struct _CAN_DRV_CONFIG {        // Structure containing compile-time configuration of driver
  uint8_t TOT_OBJ_NUM;                  // Number of total objects (Rx FIFO object + Mailbox objects)
  uint8_t RX_FIFO_OBJ_NUM;              // Number of Rx FIFO objects
  uint8_t RX_MBX_OBJ_OFS;               // Offset of Mailbox because of FIFO usage of Mailboxes
  uint8_t RX_FIFO_MAX_FILT_NUM;         // Maximum number of filter IDs for Rx FIFO
} CAN_DRV_CONFIG_t;

// Compile time configuration of drivers
const  CAN_DRV_CONFIG_t CAN_DRV_CONFIG [2] = {
#if (DRIVER_CAN1 == 1U)
  { CAN1_TOT_OBJ_NUM,
    CAN1_RX_FIFO_OBJ_NUM,
    CAN1_RX_MBX_OBJ_OFS,
    CAN1_RX_FIFO_ID_FILT_ELEM_NUM
  },
#else
  { 0U, 0U, 0U, 0U },
#endif
#if (DRIVER_CAN2 == 1U)
  { CAN2_TOT_OBJ_NUM,
    CAN2_RX_FIFO_OBJ_NUM,
    CAN2_RX_MBX_OBJ_OFS,
    CAN2_RX_FIFO_ID_FILT_ELEM_NUM
  }
#else
  { 0U, 0U, 0U, 0U }
#endif
};

// Runtime variables
static volatile uint8_t            can_driver_powered    [2] = {   0U,   0U };
static volatile uint8_t            can_driver_initialized[2] = {   0U,   0U };
static volatile ARM_CAN_STATUS     can_status            [2];
static ARM_CAN_SignalUnitEvent_t   CAN_SignalUnitEvent   [2] = { NULL, NULL };
static ARM_CAN_SignalObjectEvent_t CAN_SignalObjectEvent [2] = { NULL, NULL };

static volatile uint32_t           can_obj_tx            [2][2];
static volatile uint32_t           can_obj_rx            [2][2];

static CAN_Type                   *can_base              [2] = { CAN1, CAN2 };
static uint8_t                     can_id_filter_num     [2];
static flexcan_config_t            flexcan_config        [2];
static flexcan_timing_config_t     timing_config         [2];
static flexcan_handle_t            flexcan_handle        [2];
#if (DRIVER_CAN1 == 1U)
static uint32_t                    can1_id_filter_table  [CAN1_RX_FIFO_ID_FILT_ELEM_NUM];
static volatile flexcan_frame_t    can1_frame            [CAN1_TOT_OBJ_NUM];
#if (CAN1_RX_FIFO_OBJ_NUM != 0U)
static flexcan_fifo_transfer_t     can1_fifo_transfer;
#endif
static flexcan_mb_transfer_t       can1_mbx_transfer     [CAN1_MBX_OBJ_NUM];
#endif
#if (DRIVER_CAN2 == 1U)
static uint32_t                    can2_id_filter_table  [CAN2_RX_FIFO_ID_FILT_ELEM_NUM];
static volatile flexcan_frame_t    can2_frame            [CAN2_TOT_OBJ_NUM];
#if (CAN2_RX_FIFO_OBJ_NUM != 0U)
static flexcan_fifo_transfer_t     can2_fifo_transfer;
#endif
static flexcan_mb_transfer_t       can2_mbx_transfer     [CAN2_MBX_OBJ_NUM];
#endif

// Local module functions
static uint32_t CAN_GetClock (void);
static void     IRQ_Callback (CAN_Type *base, flexcan_handle_t *handle, status_t status, uint32_t result, void *userData);


// CAN Driver Functions

/**
  \fn          ARM_DRIVER_VERSION CAN_GetVersion (void)
  \brief       Get driver version.
  \return      ARM_DRIVER_VERSION
*/
static ARM_DRIVER_VERSION CAN_GetVersion (void) { return can_driver_version; }

/**
  \fn          ARM_CAN_CAPABILITIES CAN1_GetCapabilities (void)
  \fn          ARM_CAN_CAPABILITIES CAN2_GetCapabilities (void)
  \brief       Get driver capabilities.
  \return      ARM_CAN_CAPABILITIES
*/
#if (DRIVER_CAN1 == 1U)
static ARM_CAN_CAPABILITIES CAN1_GetCapabilities (void) { return can_driver_capabilities[0U]; }
#endif
#if (DRIVER_CAN2 == 1U)
static ARM_CAN_CAPABILITIES CAN2_GetCapabilities (void) { return can_driver_capabilities[1U]; }
#endif
/**
  \fn          int32_t CANx_Initialize (ARM_CAN_SignalUnitEvent_t   cb_unit_event,
                                        ARM_CAN_SignalObjectEvent_t cb_object_event,
                                        uint8_t                     x)
  \brief       Initialize CAN interface and register signal (callback) functions.
  \param[in]   cb_unit_event   Pointer to ARM_CAN_SignalUnitEvent callback function
  \param[in]   cb_object_event Pointer to ARM_CAN_SignalObjectEvent callback function
  \param[in]   x               Controller number (0..1)
  \return      execution status
*/
static int32_t CANx_Initialize (ARM_CAN_SignalUnitEvent_t   cb_unit_event,
                                ARM_CAN_SignalObjectEvent_t cb_object_event,
                                uint8_t                     x) {

  if (can_driver_initialized[x] != 0U) { return ARM_DRIVER_OK;    }

  CAN_SignalUnitEvent   [x] = cb_unit_event;
  CAN_SignalObjectEvent [x] = cb_object_event;

  can_driver_initialized[x] = 1U;

  return ARM_DRIVER_OK;
}
#if (DRIVER_CAN1 == 1U)
static int32_t CAN1_Initialize (ARM_CAN_SignalUnitEvent_t cb_unit_event, ARM_CAN_SignalObjectEvent_t cb_object_event) { return CANx_Initialize (cb_unit_event, cb_object_event, 0U); }
#endif
#if (DRIVER_CAN2 == 1U)
static int32_t CAN2_Initialize (ARM_CAN_SignalUnitEvent_t cb_unit_event, ARM_CAN_SignalObjectEvent_t cb_object_event) { return CANx_Initialize (cb_unit_event, cb_object_event, 1U); }
#endif

/**
  \fn          int32_t CANx_Uninitialize (uint8_t x)
  \brief       De-initialize CAN interface.
  \param[in]   x      Controller number (0..1)
  \return      execution status
*/
static int32_t CANx_Uninitialize (uint8_t x) {

  can_driver_initialized[x] = 0U;

  return ARM_DRIVER_OK;
}
#if (DRIVER_CAN1 == 1U)
static int32_t CAN1_Uninitialize (void) { return CANx_Uninitialize (0U); }
#endif
#if (DRIVER_CAN2 == 1U)
static int32_t CAN2_Uninitialize (void) { return CANx_Uninitialize (1U); }
#endif

/**
  \fn          int32_t CANx_PowerControl (ARM_POWER_STATE state, uint8_t x)
  \brief       Control CAN interface power.
  \param[in]   state  Power state
                 - ARM_POWER_OFF :  power off: no operation possible
                 - ARM_POWER_LOW :  low power mode: retain state, detect and signal wake-up events
                 - ARM_POWER_FULL : power on: full operation at maximum performance
  \param[in]   x      Controller number (0..1)
  \return      execution status
*/
static int32_t CANx_PowerControl (ARM_POWER_STATE state, uint8_t x) {
  flexcan_rx_fifo_config_t rx_fifo_config;

  switch (state) {
    case ARM_POWER_OFF:
      can_driver_powered[x] = 0U;
      FLEXCAN_Deinit(can_base[x]);
      can_obj_tx[x][0] = 0U;
      can_obj_tx[x][1] = 0U;
      can_obj_rx[x][0] = 0U;
      can_obj_rx[x][1] = 0U;
      can_status[x].unit_state      = 0U;
      can_status[x].last_error_code = 0U;
      can_status[x].tx_error_count  = 0U;
      can_status[x].rx_error_count  = 0U;
      break;

    case ARM_POWER_FULL:
      if (can_driver_initialized[x] == 0U) { return ARM_DRIVER_ERROR; }
      if (can_driver_powered[x]     != 0U) { return ARM_DRIVER_OK;    }

      // Reset variables
      memset((void *)&can_status[x], 0, sizeof(ARM_CAN_STATUS));

      // Load initial default values of timing configuration
      timing_config[x].preDivider = CAN_GetClock() / (flexcan_config[x].baudRate * (1U + (3U + 1U) + (2U + 1U) + (1U + 1U)));
      timing_config[x].phaseSeg1  = 3U;         // Phase segment 1 =     4 Tq
      timing_config[x].phaseSeg2  = 2U;         // Phase segment 2 =     3 Tq
      timing_config[x].propSeg    = 1U;         // Propagation segment = 2 Tq
      timing_config[x].rJumpwidth = 1U;         // SJW =                 2 Tq

      // Load FlexCAN module default Configuration
      //
      // clkSrc            = kFLEXCAN_ClkSrcOsc;
      // baudRate          = 1000000U;
      // maxMbNum          = 16;
      // enableLoopBack    = false;
      // enableSelfWakeup  = false;
      // enableIndividMask = false;
      // enableDoze        = false;
      //
      FLEXCAN_GetDefaultConfig(&flexcan_config[x]);

      // Update configuration to use all available mailboxes
      flexcan_config[x].maxMbNum = 64U;

      // Enable individual masking
      flexcan_config[x].enableIndividMask = 1U;

      // Initialize FlexCAN fsl driver
      FLEXCAN_Init(can_base[x], &flexcan_config[x], CAN_GetClock());

      // Set initial FIFO configuration if FIFO is enabled
      if (CAN_DRV_CONFIG[x].RX_FIFO_OBJ_NUM != 0U) {
        if (x == 0U) {
#if (DRIVER_CAN1 == 1U)
          memset((void *)can1_id_filter_table, 0, sizeof(can1_id_filter_table));
          rx_fifo_config.idFilterTable = can1_id_filter_table;
#else
          return ARM_DRIVER_ERROR;
#endif
        } else if (x == 1U) {
#if (DRIVER_CAN2 == 1U)
          memset((void *)can2_id_filter_table, 0, sizeof(can2_id_filter_table));
          rx_fifo_config.idFilterTable = can2_id_filter_table;
#else
          return ARM_DRIVER_ERROR;
#endif
        }
        rx_fifo_config.idFilterNum  = CAN_DRV_CONFIG[x].RX_FIFO_MAX_FILT_NUM;
        rx_fifo_config.idFilterType = kFLEXCAN_RxFifoFilterTypeA;
        rx_fifo_config.priority     = kFLEXCAN_RxFifoPrioLow;
        can_id_filter_num[x] = 0U;

        FLEXCAN_SetRxFifoConfig(can_base[x], &rx_fifo_config, true);

        FLEXCAN_SetRxFifoGlobalMask(can_base[x], 0xFFFFFFFFUL);
      }
      FLEXCAN_SetRxMbGlobalMask(can_base[x], 0xFFFFFFFFUL);

      // Create FlexCAN handle structure and set callback function
      FLEXCAN_TransferCreateHandle(can_base[x], &flexcan_handle[x], IRQ_Callback, NULL);

      can_driver_powered[x] = 1U;
      break;

    case ARM_POWER_LOW:
      return ARM_DRIVER_ERROR_UNSUPPORTED;

    default:
      return ARM_DRIVER_ERROR_UNSUPPORTED;
  }

  return ARM_DRIVER_OK;
}
#if (DRIVER_CAN1 == 1U)
static int32_t CAN1_PowerControl (ARM_POWER_STATE state) { return CANx_PowerControl (state, 0U); }
#endif
#if (DRIVER_CAN2 == 1U)
static int32_t CAN2_PowerControl (ARM_POWER_STATE state) { return CANx_PowerControl (state, 1U); }
#endif

/**
  \fn          uint32_t CAN_GetClock (void)
  \brief       Retrieve CAN base clock frequency.
  \return      base clock frequency
*/
static uint32_t CAN_GetClock (void) {
  uint32_t val;

  switch ((CCM->CSCMR2 & CCM_CSCMR2_CAN_CLK_SEL_MASK) >> CCM_CSCMR2_CAN_CLK_SEL_SHIFT) {
    case 0:
      val = CLOCK_GetFreq(kCLOCK_Usb1PllClk) / 8;
      break;
    case 1:
      val = 24000000U;
      break;
    case 2:
      val = CLOCK_GetFreq(kCLOCK_Usb1PllClk) / 6;
      break;
    default:
      val = 0U;
      break;
  }
  val = val / (((CCM->CSCMR2 & CCM_CSCMR2_CAN_CLK_PODF_MASK) >> CCM_CSCMR2_CAN_CLK_PODF_SHIFT) + 1U);

  return val;
}


/**
  \fn          int32_t CANx_SetBitrate (ARM_CAN_BITRATE_SELECT select, uint32_t bitrate, uint32_t bit_segments, uint8_t x)
  \brief       Set bitrate for CAN interface.
  \param[in]   select       Bitrate selection
                 - ARM_CAN_BITRATE_NOMINAL : nominal (flexible data-rate arbitration) bitrate
                 - ARM_CAN_BITRATE_FD_DATA : flexible data-rate data bitrate
  \param[in]   bitrate      Bitrate
  \param[in]   bit_segments Bit segments settings
  \param[in]   x            Controller number (0..1)
  \return      execution status
*/
static int32_t CANx_SetBitrate (ARM_CAN_BITRATE_SELECT select, uint32_t bitrate, uint32_t bit_segments, uint8_t x) {
  uint32_t sjw, prop_seg, phase_seg1, phase_seg2, pclk, tq_num, presdiv;

  if (select != ARM_CAN_BITRATE_NOMINAL) {
    return ARM_CAN_INVALID_BITRATE_SELECT;
  }

  prop_seg   = (bit_segments & ARM_CAN_BIT_PROP_SEG_Msk  ) >> ARM_CAN_BIT_PROP_SEG_Pos;
  phase_seg1 = (bit_segments & ARM_CAN_BIT_PHASE_SEG1_Msk) >> ARM_CAN_BIT_PHASE_SEG1_Pos;
  phase_seg2 = (bit_segments & ARM_CAN_BIT_PHASE_SEG2_Msk) >> ARM_CAN_BIT_PHASE_SEG2_Pos;
  sjw        = (bit_segments & ARM_CAN_BIT_SJW_Msk       ) >> ARM_CAN_BIT_SJW_Pos;

  if (  prop_seg               < 1U                                     ) { return ARM_CAN_INVALID_BIT_PROP_SEG;   }
  if (  phase_seg1             < 1U                                     ) { return ARM_CAN_INVALID_BIT_PHASE_SEG1; }
  if (((prop_seg + phase_seg1) < 4U) || ((prop_seg + phase_seg1) >  16U)) { return ARM_CAN_INVALID_BIT_PROP_SEG;   }
  if (( phase_seg2             < 2U) || ( phase_seg2             >   8U)) { return ARM_CAN_INVALID_BIT_PHASE_SEG2; }
  if (( sjw                    < 1U) || ( sjw                    >   4U)) { return ARM_CAN_INVALID_BIT_SJW;        }

  tq_num     = 1U + prop_seg + phase_seg1 + phase_seg2;
  pclk       = CAN_GetClock ();
  presdiv    = (pclk / tq_num) / bitrate;

  if (pclk == 0U)   { return ARM_DRIVER_ERROR; }
  if (presdiv < 1U) { return ARM_DRIVER_ERROR; }


  // Check selected clock bitrate is within expected tolerance
  if (pclk > (presdiv * tq_num * bitrate)) {
    if ((((pclk - (presdiv * tq_num * bitrate)) * 1024U) / pclk) > CAN_CLOCK_TOLERANCE) { return ARM_CAN_INVALID_BITRATE; }
  } else if (pclk < (presdiv * tq_num * bitrate)) {
    if (((((presdiv * tq_num * bitrate) - pclk) * 1024U) / pclk) > CAN_CLOCK_TOLERANCE) { return ARM_CAN_INVALID_BITRATE; }
  }

  // Set all parameters in required structure
  timing_config[x].preDivider = presdiv    - 1U;
  timing_config[x].phaseSeg1  = phase_seg1 - 1U;
  timing_config[x].phaseSeg2  = phase_seg2 - 1U;
  timing_config[x].propSeg    = prop_seg   - 1U;
  timing_config[x].rJumpwidth = sjw        - 1U;

  // Update actual timing characteristics
  FLEXCAN_SetTimingConfig(can_base[x], &timing_config[x]);

  return ARM_DRIVER_OK;
}
#if (DRIVER_CAN1 == 1U)
static int32_t CAN1_SetBitrate (ARM_CAN_BITRATE_SELECT select, uint32_t bitrate, uint32_t bit_segments) { return CANx_SetBitrate (select, bitrate, bit_segments, 0U); }
#endif
#if (DRIVER_CAN2 == 1U)
static int32_t CAN2_SetBitrate (ARM_CAN_BITRATE_SELECT select, uint32_t bitrate, uint32_t bit_segments) { return CANx_SetBitrate (select, bitrate, bit_segments, 1U); }
#endif

/**
  \fn          int32_t CANx_SetMode (ARM_CAN_MODE mode, uint8_t x)
  \brief       Set operating mode for CAN interface.
  \param[in]   mode   Operating mode
                 - ARM_CAN_MODE_INITIALIZATION :    initialization mode
                 - ARM_CAN_MODE_NORMAL :            normal operation mode
                 - ARM_CAN_MODE_RESTRICTED :        restricted operation mode
                 - ARM_CAN_MODE_MONITOR :           bus monitoring mode
                 - ARM_CAN_MODE_LOOPBACK_INTERNAL : loopback internal mode
                 - ARM_CAN_MODE_LOOPBACK_EXTERNAL : loopback external mode
  \param[in]   x      Controller number (0..1)
  \return      execution status
*/
static int32_t CANx_SetMode (ARM_CAN_MODE mode, uint8_t x) {
  uint32_t event;
  bool     exit_freeze, loopback;

  exit_freeze = true;
  loopback    = 0U;
  event       = 0U;

  // Enter freeze mode, as all configuration change is allowed only in freeze mode
  can_base[x]->MCR |= CAN_MCR_HALT_MASK | CAN_MCR_FRZ_MASK;
  while (!(can_base[x]->MCR & CAN_MCR_FRZACK_MASK));

  switch (mode) {
    case ARM_CAN_MODE_INITIALIZATION:
      exit_freeze = false;
      can_status[x].unit_state = ARM_CAN_UNIT_STATE_INACTIVE;
      event                    = ARM_CAN_EVENT_UNIT_BUS_OFF;
      break;

    case ARM_CAN_MODE_NORMAL:
      can_status[x].unit_state = ARM_CAN_UNIT_STATE_ACTIVE;
      event                    = ARM_CAN_EVENT_UNIT_ACTIVE;
      break;

    case ARM_CAN_MODE_MONITOR:
      can_base[x]->CTRL1 |= CAN_CTRL1_LOM_MASK;
      can_status[x].unit_state = ARM_CAN_UNIT_STATE_PASSIVE;
      event                    = ARM_CAN_EVENT_UNIT_PASSIVE;
      break;

    case ARM_CAN_MODE_LOOPBACK_INTERNAL:
      loopback = true;
      can_status[x].unit_state = ARM_CAN_UNIT_STATE_PASSIVE;
      event                    = ARM_CAN_EVENT_UNIT_PASSIVE;
      break;

    case ARM_CAN_MODE_RESTRICTED:
      return ARM_DRIVER_ERROR_PARAMETER;

    case ARM_CAN_MODE_LOOPBACK_EXTERNAL:
      return ARM_DRIVER_ERROR_PARAMETER;

    default:
      return ARM_DRIVER_ERROR_PARAMETER;
  }

  flexcan_config[x].enableLoopBack = loopback;
  if (loopback == true) {
    can_base[x]->CTRL1 |=  CAN_CTRL1_LPB_MASK;
    can_base[x]->MCR   &= ~CAN_MCR_SRXDIS_MASK; // Enable self-reception
  } else {
    can_base[x]->CTRL1 &= ~CAN_CTRL1_LPB_MASK;
    can_base[x]->MCR   |=  CAN_MCR_SRXDIS_MASK; // Disable self-reception
  }

  if (exit_freeze == true) {
    // Exit freeze mode
    can_base[x]->MCR &= ~(CAN_MCR_HALT_MASK | CAN_MCR_FRZ_MASK);
    while (can_base[x]->MCR & CAN_MCR_FRZACK_MASK);
  }

  if ((CAN_SignalUnitEvent[x] != NULL) && (event != 0U)) { CAN_SignalUnitEvent[x](event); }

  return ARM_DRIVER_OK;
}
#if (DRIVER_CAN1 == 1U)
static int32_t CAN1_SetMode (ARM_CAN_MODE mode) { return CANx_SetMode (mode, 0U); }
#endif
#if (DRIVER_CAN2 == 1U)
static int32_t CAN2_SetMode (ARM_CAN_MODE mode) { return CANx_SetMode (mode, 1U); }
#endif

/**
  \fn          ARM_CAN_OBJ_CAPABILITIES CANx_ObjectGetCapabilities (uint32_t obj_idx, uint8_t x)
  \brief       Retrieve capabilities of an object.
  \param[in]   obj_idx  Object index
  \param[in]   x        Controller number (0..1)
  \return      ARM_CAN_OBJ_CAPABILITIES
*/
ARM_CAN_OBJ_CAPABILITIES CANx_ObjectGetCapabilities (uint32_t obj_idx, uint8_t x) {
  ARM_CAN_OBJ_CAPABILITIES obj_cap_null;

  if (obj_idx >= CAN_DRV_CONFIG[x].TOT_OBJ_NUM) {
    memset ((void *)&obj_cap_null, 0, sizeof(ARM_CAN_OBJ_CAPABILITIES));
    return obj_cap_null;
  }

  if ((obj_idx == 0U) && (CAN_DRV_CONFIG[x].RX_FIFO_OBJ_NUM != 0U)) {
    // Rx FIFO capabilities
    return can_object_capabilities_rx_fifo;
  } else {
    return can_object_capabilities_mbx;
  }
}
#if (DRIVER_CAN1 == 1U)
ARM_CAN_OBJ_CAPABILITIES CAN1_ObjectGetCapabilities (uint32_t obj_idx) { return CANx_ObjectGetCapabilities (obj_idx, 0U); }
#endif
#if (DRIVER_CAN2 == 1U)
ARM_CAN_OBJ_CAPABILITIES CAN2_ObjectGetCapabilities (uint32_t obj_idx) { return CANx_ObjectGetCapabilities (obj_idx, 1U); }
#endif

/**
  \fn          int32_t CANx_ObjectSetFilter (uint32_t obj_idx, ARM_CAN_FILTER_OPERATION operation, uint32_t id, uint32_t arg, uint8_t x)
  \brief       Add or remove filter for message reception.
  \param[in]   obj_idx      Object index of object that filter should be or is assigned to
  \param[in]   operation    Operation on filter
                 - ARM_CAN_FILTER_ID_EXACT_ADD :       add    exact id filter
                 - ARM_CAN_FILTER_ID_EXACT_REMOVE :    remove exact id filter
                 - ARM_CAN_FILTER_ID_RANGE_ADD :       add    range id filter
                 - ARM_CAN_FILTER_ID_RANGE_REMOVE :    remove range id filter
                 - ARM_CAN_FILTER_ID_MASKABLE_ADD :    add    maskable id filter
                 - ARM_CAN_FILTER_ID_MASKABLE_REMOVE : remove maskable id filter
  \param[in]   id           ID or start of ID range (depending on filter type)
  \param[in]   arg          Mask or end of ID range (depending on filter type)
  \param[in]   x            Controller number (0..1)
  \return      execution status
*/
static int32_t CANx_ObjectSetFilter (uint32_t obj_idx, ARM_CAN_FILTER_OPERATION operation, uint32_t id, uint32_t arg, uint8_t x) {
  flexcan_rx_fifo_config_t rx_fifo_config;
  flexcan_rx_mb_config_t   rx_mb_config;
  uint32_t                *ptr_filter_table, id_entry, mbx_idx, mask;
  uint8_t                  i;

  if (obj_idx >= CAN_DRV_CONFIG[x].TOT_OBJ_NUM) { return ARM_DRIVER_ERROR_PARAMETER; }
  if (can_driver_powered[x] == 0U)              { return ARM_DRIVER_ERROR;           }

  if ((operation == ARM_CAN_FILTER_ID_RANGE_ADD)    || 
      (operation == ARM_CAN_FILTER_ID_RANGE_REMOVE)) {
    // Range filters are not supported by HW
    return ARM_DRIVER_ERROR_UNSUPPORTED;
  }

  if ((obj_idx == 0U) && (CAN_DRV_CONFIG[x].RX_FIFO_OBJ_NUM != 0U)) {   // Rx FIFO object
    if ((id & ARM_CAN_ID_IDE_Msk) != 0U) {      // Extended Identifier
      id_entry = (id & 0x1FFFFFFFU) | (1U << 29);
    } else {                                    // Standard Identifier
      id_entry = (id & 0x7FFU) << CAN_ID_STD_SHIFT;
    }
    id_entry <<= 1U;                            // Filter ID entry contains shifted message ID
    if (x == 0U) {
#if (DRIVER_CAN1 == 1U)
      ptr_filter_table             = can1_id_filter_table;
      rx_fifo_config.idFilterTable = can1_id_filter_table;
#else
      return ARM_DRIVER_ERROR;
#endif
    } else {
#if (DRIVER_CAN2 == 1U)
      ptr_filter_table             = can2_id_filter_table;
      rx_fifo_config.idFilterTable = can2_id_filter_table;
#else
      return ARM_DRIVER_ERROR;
#endif
    }
    rx_fifo_config.idFilterType = kFLEXCAN_RxFifoFilterTypeA;
    rx_fifo_config.priority     = kFLEXCAN_RxFifoPrioLow;
    if ((operation == ARM_CAN_FILTER_ID_EXACT_ADD)    ||                // Add exact or
        (operation == ARM_CAN_FILTER_ID_MASKABLE_ADD)) {                // maskable filter
      if (can_id_filter_num[x] >= CAN_DRV_CONFIG[x].RX_FIFO_MAX_FILT_NUM) {
        // If no space to add new ID filter
        return ARM_DRIVER_ERROR;
      }
      if ((operation == ARM_CAN_FILTER_ID_MASKABLE_ADD) &&
          (can_id_filter_num[x] >= CAN_DRV_CONFIG[x].RX_MBX_OBJ_OFS)) {
        // If no individual mask for Rx FIFO filter table entry is available 
        return ARM_DRIVER_ERROR;
      }
      for (i = 0U; i < CAN_DRV_CONFIG[x].RX_FIFO_MAX_FILT_NUM; i++) {
        if (*ptr_filter_table == 0U) {                                  // Found empty ID filter slot
          break;
        }
        ptr_filter_table++;
      }
      *ptr_filter_table = id_entry;
      can_id_filter_num[x]++;

      if (operation == ARM_CAN_FILTER_ID_MASKABLE_ADD) {                // Add maskable filter
        if ((id & ARM_CAN_ID_IDE_Msk) != 0U) {                          // Extended Identifier
          mask =  arg & 0x1FFFFFFFU;
        } else {                                                        // Standard Identifier
          mask = (arg & 0x7FFU) << CAN_ID_STD_SHIFT;
        }
        FLEXCAN_SetRxIndividualMask(can_base[x], can_id_filter_num[x] - 1U, mask);
      }
    } else {                                                            // Remove exact or maskable filter
      if (can_id_filter_num[x] == 0U) {
        // If no ID filter exists
        return ARM_DRIVER_OK;
      }
      for (i = 0U; i < CAN_DRV_CONFIG[x].RX_FIFO_MAX_FILT_NUM; i++) {
        if (*ptr_filter_table == id_entry) {                            // Found ID filter entry for removal
          *ptr_filter_table = 0U;
          break;
        }
        ptr_filter_table++;
      }
      if (i != CAN_DRV_CONFIG[x].RX_FIFO_MAX_FILT_NUM) {                // If entry to remove was found
        // Move all entries from found entry for one entry down
        if (operation == ARM_CAN_FILTER_ID_MASKABLE_REMOVE) {
          FLEXCAN_EnterFreezeMode(can_base[x]);
        }
        for (; i < CAN_DRV_CONFIG[x].RX_FIFO_MAX_FILT_NUM; i++) {
          /* Move filter table entry */
          *ptr_filter_table = *(ptr_filter_table+1);

          if (operation == ARM_CAN_FILTER_ID_MASKABLE_REMOVE) {
            if (i <= CAN_DRV_CONFIG[x].RX_MBX_OBJ_OFS) {
              /* Move individual mask entry */
              can_base[x]->RXIMR[i] = can_base[x]->RXIMR[i+1];
            }
          }

          ptr_filter_table++;
        }
        if (operation == ARM_CAN_FILTER_ID_MASKABLE_REMOVE) {
          FLEXCAN_ExitFreezeMode(can_base[x]);
        }
        *ptr_filter_table = 0U;
      }
      can_id_filter_num[x]--;
    }
    rx_fifo_config.idFilterNum = CAN_DRV_CONFIG[x].RX_FIFO_MAX_FILT_NUM;
    if (can_id_filter_num[x] != 0U) {
      FLEXCAN_SetRxFifoConfig(can_base[x], &rx_fifo_config, true);
    } else {
      FLEXCAN_SetRxFifoConfig(can_base[x], &rx_fifo_config, false);
    }
  } else {                                                              // Mailbox object
    mbx_idx = obj_idx + CAN_DRV_CONFIG[x].RX_MBX_OBJ_OFS + 1U;
    if ((operation == ARM_CAN_FILTER_ID_EXACT_ADD)    ||                // Add exact or
        (operation == ARM_CAN_FILTER_ID_MASKABLE_ADD)) {                // maskable filter
      if ((id & ARM_CAN_ID_IDE_Msk) != 0U) {                            // Extended Identifier
        mask     =  arg & 0x1FFFFFFFU;
        id_entry =  id  & 0x1FFFFFFFU;
        rx_mb_config.format = kFLEXCAN_FrameFormatExtend;
      } else {                                                          // Standard Identifier
        mask     = (arg & 0x7FFU) << CAN_ID_STD_SHIFT;
        id_entry = (id  & 0x7FFU) << CAN_ID_STD_SHIFT;
        rx_mb_config.format = kFLEXCAN_FrameFormatStandard;
      }
      rx_mb_config.id   = id_entry;
      rx_mb_config.type = kFLEXCAN_FrameTypeData;
      FLEXCAN_SetRxMbConfig(can_base[x], mbx_idx, &rx_mb_config, true);
      if (operation == ARM_CAN_FILTER_ID_MASKABLE_ADD) {                // Add maskable filter
        FLEXCAN_SetRxIndividualMask(can_base[x], mbx_idx, mask);
      }
    } else {                                                            // Remove
      if (operation == ARM_CAN_FILTER_ID_MASKABLE_REMOVE) {             // Remove maskable filter
        FLEXCAN_SetRxIndividualMask(can_base[x], mbx_idx, 0U);
      }
      memset((void *)&rx_mb_config, 0, sizeof(rx_mb_config));
      FLEXCAN_SetRxMbConfig(can_base[x], mbx_idx, &rx_mb_config, false);
    }
  }

  return ARM_DRIVER_OK;
}
#if (DRIVER_CAN1 == 1U)
static int32_t CAN1_ObjectSetFilter (uint32_t obj_idx, ARM_CAN_FILTER_OPERATION operation, uint32_t id, uint32_t arg) { return CANx_ObjectSetFilter (obj_idx, operation, id, arg, 0U); }
#endif
#if (DRIVER_CAN2 == 1U)
static int32_t CAN2_ObjectSetFilter (uint32_t obj_idx, ARM_CAN_FILTER_OPERATION operation, uint32_t id, uint32_t arg) { return CANx_ObjectSetFilter (obj_idx, operation, id, arg, 1U); }
#endif

/**
  \fn          int32_t CANx_StartReceive (uint32_t obj_idx, uint8_t x)
  \brief       Start reception on an object.
  \param[in]   obj_idx  Object index
  \param[in]   x        Controller number (0..1)
  \return      execution status
*/
static int32_t CANx_StartReceive (uint32_t obj_idx, uint8_t x) {
  volatile flexcan_frame_t         *ptr_rx_frame;
           flexcan_mb_transfer_t   *ptr_rx_mbx_transfer;
           flexcan_fifo_transfer_t *ptr_rx_fifo_transfer;
           uint32_t                 mbx_idx;

  if (x == 0U) {
#if (DRIVER_CAN1 == 1U)
    ptr_rx_frame         = &can1_frame[obj_idx];
    ptr_rx_fifo_transfer = &can1_fifo_transfer;
    ptr_rx_mbx_transfer  = &can1_mbx_transfer[obj_idx];
#else
    return kStatus_Fail;
#endif
  } else {
#if (DRIVER_CAN2 == 1U)
    ptr_rx_frame         = &can2_frame[obj_idx];
    ptr_rx_fifo_transfer = &can2_fifo_transfer;
    ptr_rx_mbx_transfer  = &can2_mbx_transfer[obj_idx];
#else
    return kStatus_Fail;
#endif
  }
  if ((obj_idx == 0U) && (CAN_DRV_CONFIG[x].RX_FIFO_OBJ_NUM != 0U)) {       // Rx FIFO object
    ptr_rx_fifo_transfer->frame = (flexcan_frame_t *)ptr_rx_frame;
#if (FSL_FLEXCAN_DRIVER_VERSION >= (MAKE_VERSION(2,9,2)))
    ptr_rx_fifo_transfer->frameNum = 1U;
#endif
    return FLEXCAN_TransferReceiveFifoNonBlocking(can_base[x], &flexcan_handle[x], ptr_rx_fifo_transfer);
  } else {                                                                  // Mailbox object
    mbx_idx = obj_idx + CAN_DRV_CONFIG[x].RX_MBX_OBJ_OFS + 1U;
    ptr_rx_mbx_transfer->frame = (flexcan_frame_t *)ptr_rx_frame;
    ptr_rx_mbx_transfer->mbIdx = mbx_idx;
    return FLEXCAN_TransferReceiveNonBlocking(can_base[x], &flexcan_handle[x], ptr_rx_mbx_transfer);
  }
}

/**
  \fn          void CANx_StopReceive (uint32_t obj_idx, uint8_t x)
  \brief       Stop reception on an object.
  \param[in]   obj_idx  Object index
  \param[in]   x        Controller number (0..1)
  \return      execution status
*/
static void CANx_StopReceive (uint32_t obj_idx, uint8_t x) {
  uint32_t mbx_idx;

  if ((obj_idx == 0U) && (CAN_DRV_CONFIG[x].RX_FIFO_OBJ_NUM != 0U)) {       // Rx FIFO object
    FLEXCAN_TransferAbortReceiveFifo(can_base[x], &flexcan_handle[x]);
  } else {                                                                  // Mailbox object
    mbx_idx = obj_idx + CAN_DRV_CONFIG[x].RX_MBX_OBJ_OFS + 1U;
    FLEXCAN_TransferAbortReceive(can_base[x], &flexcan_handle[x], mbx_idx);
  }
}

/**
  \fn          int32_t CANx_ObjectConfigure (uint32_t obj_idx, ARM_CAN_OBJ_CONFIG obj_cfg, uint8_t x)
  \brief       Configure object.
  \param[in]   obj_idx  Object index
  \param[in]   obj_cfg  Object configuration state
                 - ARM_CAN_OBJ_INACTIVE :       deactivate object
                 - ARM_CAN_OBJ_RX :             configure object for reception
                 - ARM_CAN_OBJ_TX :             configure object for transmission
                 - ARM_CAN_OBJ_RX_RTR_TX_DATA : configure object that on RTR reception automatically transmits Data Frame
                 - ARM_CAN_OBJ_TX_RTR_RX_DATA : configure object that transmits RTR and automatically receives Data Frame
  \param[in]   x        Controller number (0..1)
  \return      execution status
*/
static int32_t CANx_ObjectConfigure (uint32_t obj_idx, ARM_CAN_OBJ_CONFIG obj_cfg, uint8_t x) {
  uint32_t idx, msk;

  if (obj_idx >= CAN_DRV_CONFIG[x].TOT_OBJ_NUM) { return ARM_DRIVER_ERROR_PARAMETER; }
  if (can_driver_powered[x] == 0U)              { return ARM_DRIVER_ERROR;           }

  idx = obj_idx / 32U;
  msk = 1U << (obj_idx % 32U);

  switch (obj_cfg) {
    case ARM_CAN_OBJ_INACTIVE:
      if ((obj_idx + 1U) >= CAN_DRV_CONFIG[x].TOT_OBJ_NUM) { return ARM_DRIVER_ERROR_PARAMETER; }
      if ((can_obj_tx[x][idx] &  msk) != 0U) {
        FLEXCAN_SetTxMbConfig(can_base[x], obj_idx + CAN_DRV_CONFIG[x].RX_MBX_OBJ_OFS + 1U, false);
      }
      if ((can_obj_rx[x][idx] &  msk) != 0U) {
        CANx_StopReceive (obj_idx, x);
      }
      can_obj_tx[x][idx] &= ~msk;
      can_obj_rx[x][idx] &= ~msk;
      break;

    case ARM_CAN_OBJ_TX:
      if ((obj_idx + 1U) >= CAN_DRV_CONFIG[x].TOT_OBJ_NUM) { return ARM_DRIVER_ERROR_PARAMETER; }
      if ((can_obj_tx[x][idx] &  msk) != 0U) { break; } 
      can_obj_rx[x][idx] &= ~msk;
      can_obj_tx[x][idx] |=  msk;
      FLEXCAN_SetTxMbConfig(can_base[x], obj_idx + CAN_DRV_CONFIG[x].RX_MBX_OBJ_OFS + 1U, true);
      break;

    case ARM_CAN_OBJ_RX:
      if ((can_obj_rx[x][idx] &  msk) != 0U) { break; } 
      can_obj_tx[x][idx] &= ~msk;
      can_obj_rx[x][idx] |=  msk;
      if (CANx_StartReceive (obj_idx, x) != kStatus_Success) {
        can_obj_rx[x][idx] &= ~msk;
        return ARM_DRIVER_ERROR;
      }
      break;

    case ARM_CAN_OBJ_RX_RTR_TX_DATA:
      return ARM_DRIVER_ERROR_UNSUPPORTED;

    case ARM_CAN_OBJ_TX_RTR_RX_DATA:
      return ARM_DRIVER_ERROR_UNSUPPORTED;

    default:
      return ARM_DRIVER_ERROR;
  }

  return ARM_DRIVER_OK;
}
#if (DRIVER_CAN1 == 1U)
static int32_t CAN1_ObjectConfigure (uint32_t obj_idx, ARM_CAN_OBJ_CONFIG obj_cfg) { return CANx_ObjectConfigure (obj_idx, obj_cfg, 0U); }
#endif
#if (DRIVER_CAN2 == 1U)
static int32_t CAN2_ObjectConfigure (uint32_t obj_idx, ARM_CAN_OBJ_CONFIG obj_cfg) { return CANx_ObjectConfigure (obj_idx, obj_cfg, 1U); }
#endif

/**
  \fn          int32_t CANx_MessageSend (uint32_t obj_idx, ARM_CAN_MSG_INFO *msg_info, const uint8_t *data, uint8_t size, uint8_t x)
  \brief       Send message on CAN bus.
  \param[in]   obj_idx  Object index
  \param[in]   msg_info Pointer to CAN message information
  \param[in]   data     Pointer to data buffer
  \param[in]   size     Number of data bytes to send
  \param[in]   x        Controller number (0..1)
  \return      value >= 0  number of data bytes accepted to send
  \return      value < 0   execution status
*/
static int32_t CANx_MessageSend (uint32_t obj_idx, ARM_CAN_MSG_INFO *msg_info, const uint8_t *data, uint8_t size, uint8_t x) {
  volatile flexcan_frame_t       *ptr_tx_frame;
           flexcan_mb_transfer_t *ptr_tx_mbx_transfer;

  if ((obj_idx + 1U) >= CAN_DRV_CONFIG[x].TOT_OBJ_NUM)          { return ARM_DRIVER_ERROR_PARAMETER; }
  if (can_driver_powered[x] == 0U)                              { return ARM_DRIVER_ERROR;           }
  if ((can_obj_tx[x][obj_idx/32U] & (1U<<(obj_idx%32U))) == 0U) { return ARM_DRIVER_ERROR;           }

  if (x == 0U) {
#if (DRIVER_CAN1 == 1U)
    ptr_tx_frame        = &can1_frame[obj_idx];
    ptr_tx_mbx_transfer = &can1_mbx_transfer[obj_idx];
#else
    return ARM_DRIVER_ERROR;
#endif
  } else {
#if (DRIVER_CAN2 == 1U)
    ptr_tx_frame        = &can2_frame[obj_idx];
    ptr_tx_mbx_transfer = &can2_mbx_transfer[obj_idx];
#else
    return ARM_DRIVER_ERROR;
#endif
  }

  if ((msg_info->id & ARM_CAN_ID_IDE_Msk) != 0U) {      // Extended ID
    ptr_tx_frame->id     = (msg_info->id & 0x1FFFFFFFU);
    ptr_tx_frame->format = kFLEXCAN_FrameFormatExtend;
  } else {                                              // Standard ID
    ptr_tx_frame->id     = (msg_info->id & 0x7FFU) << CAN_ID_STD_SHIFT;
    ptr_tx_frame->format = kFLEXCAN_FrameFormatStandard;
  }
  if (msg_info->rtr != 0U) {                            // RTR message
    ptr_tx_frame->type = kFLEXCAN_FrameTypeRemote;
  } else {                                              // Data message
    ptr_tx_frame->type = kFLEXCAN_FrameTypeData;
  }
  ptr_tx_frame->length = size;
  if (size > 0U) { ptr_tx_frame->dataByte0 = *data++; }
  if (size > 1U) { ptr_tx_frame->dataByte1 = *data++; }
  if (size > 2U) { ptr_tx_frame->dataByte2 = *data++; }
  if (size > 3U) { ptr_tx_frame->dataByte3 = *data++; }
  if (size > 4U) { ptr_tx_frame->dataByte4 = *data++; }
  if (size > 5U) { ptr_tx_frame->dataByte5 = *data++; }
  if (size > 6U) { ptr_tx_frame->dataByte6 = *data++; }
  if (size > 7U) { ptr_tx_frame->dataByte7 = *data  ; }

  ptr_tx_mbx_transfer->frame = (flexcan_frame_t *)ptr_tx_frame;
  ptr_tx_mbx_transfer->mbIdx = obj_idx + CAN_DRV_CONFIG[x].RX_MBX_OBJ_OFS + 1U;
  if (FLEXCAN_TransferSendNonBlocking(can_base[x], &flexcan_handle[x], ptr_tx_mbx_transfer) == kStatus_Success) {
    return ((int32_t)size);
  }

  return ARM_DRIVER_ERROR;
}
#if (DRIVER_CAN1 == 1U)
static int32_t CAN1_MessageSend (uint32_t obj_idx, ARM_CAN_MSG_INFO *msg_info, const uint8_t *data, uint8_t size) { return CANx_MessageSend (obj_idx, msg_info, data, size, 0U); }
#endif
#if (DRIVER_CAN2 == 1U)
static int32_t CAN2_MessageSend (uint32_t obj_idx, ARM_CAN_MSG_INFO *msg_info, const uint8_t *data, uint8_t size) { return CANx_MessageSend (obj_idx, msg_info, data, size, 1U); }
#endif

/**
  \fn          int32_t CANx_MessageRead (uint32_t obj_idx, ARM_CAN_MSG_INFO *msg_info, uint8_t *data, uint8_t size, uint8_t x)
  \brief       Read message received on CAN bus.
  \param[in]   obj_idx  Object index
  \param[out]  msg_info Pointer to read CAN message information
  \param[out]  data     Pointer to data buffer for read data
  \param[in]   size     Maximum number of data bytes to read
  \param[in]   x        Controller number (0..1)
  \return      value >= 0  number of data bytes read
  \return      value < 0   execution status
*/
static int32_t CANx_MessageRead (uint32_t obj_idx, ARM_CAN_MSG_INFO *msg_info, uint8_t *data, uint8_t size, uint8_t x) {
  volatile flexcan_frame_t *ptr_rx_frame;
           uint32_t         rx_size;

  if (obj_idx >= CAN_DRV_CONFIG[x].TOT_OBJ_NUM)                 { return ARM_DRIVER_ERROR_PARAMETER; }
  if (can_driver_powered[x] == 0U)                              { return ARM_DRIVER_ERROR;           }
  if ((can_obj_rx[x][obj_idx/32U] & (1U<<(obj_idx%32U))) == 0U) { return ARM_DRIVER_ERROR;           }

  // Message is already available in frame
  if ((obj_idx == 0U) && (CAN_DRV_CONFIG[x].RX_FIFO_OBJ_NUM != 0U)) {   // Rx FIFO object
    if (x == 0U) {
#if (DRIVER_CAN1 == 1U)
      ptr_rx_frame = (volatile flexcan_frame_t *)((&can1_fifo_transfer)->frame);
#else
      return ARM_DRIVER_ERROR;
#endif
    } else {
#if (DRIVER_CAN2 == 1U)
      ptr_rx_frame = (volatile flexcan_frame_t *)((&can2_fifo_transfer)->frame);
#else
      return ARM_DRIVER_ERROR;
#endif
    }
  } else {                                                              // Mailbox object
    if (x == 0U) {
#if (DRIVER_CAN1 == 1U)
      ptr_rx_frame = (volatile flexcan_frame_t *)((&can1_mbx_transfer[obj_idx])->frame);
#else
      return ARM_DRIVER_ERROR;
#endif
    } else {
#if (DRIVER_CAN2 == 1U)
      ptr_rx_frame = (volatile flexcan_frame_t *)((&can2_mbx_transfer[obj_idx])->frame);
#else
      return ARM_DRIVER_ERROR;
#endif
    }
  }

  // Extract received data and parameters into msg_info structure
  if (ptr_rx_frame->format == kFLEXCAN_FrameFormatExtend) {             // If Extended ID
    msg_info->id = ( ptr_rx_frame->id & 0x1FFFFFFFU) | ARM_CAN_ID_IDE_Msk;
  } else {                              // If Standard ID               // If Standard ID
    msg_info->id = ((ptr_rx_frame->id >> CAN_ID_STD_SHIFT) & 0x7FFU);
  }
  if (ptr_rx_frame->type == kFLEXCAN_FrameTypeRemote) {                 // If RTR message
    msg_info->rtr = 1U;
  } else {                                                              // If Data message
    msg_info->rtr = 0U;
  }
  msg_info->dlc =  ptr_rx_frame->length;                                // Get Data Length Code
  rx_size       =  ptr_rx_frame->length;
  if (rx_size > 0U) { *data++ = ptr_rx_frame->dataByte0; }
  if (rx_size > 1U) { *data++ = ptr_rx_frame->dataByte1; }
  if (rx_size > 2U) { *data++ = ptr_rx_frame->dataByte2; }
  if (rx_size > 3U) { *data++ = ptr_rx_frame->dataByte3; }
  if (rx_size > 4U) { *data++ = ptr_rx_frame->dataByte4; }
  if (rx_size > 5U) { *data++ = ptr_rx_frame->dataByte5; }
  if (rx_size > 6U) { *data++ = ptr_rx_frame->dataByte6; }
  if (rx_size > 7U) { *data   = ptr_rx_frame->dataByte7; }

  // Start new reception on object that wast just read from
  CANx_StartReceive (obj_idx, x);

  return ((int32_t)rx_size);
}
#if (DRIVER_CAN1 == 1U)
static int32_t CAN1_MessageRead (uint32_t obj_idx, ARM_CAN_MSG_INFO *msg_info, uint8_t *data, uint8_t size) { return CANx_MessageRead (obj_idx, msg_info, data, size, 0U); }
#endif
#if (DRIVER_CAN2 == 1U)
static int32_t CAN2_MessageRead (uint32_t obj_idx, ARM_CAN_MSG_INFO *msg_info, uint8_t *data, uint8_t size) { return CANx_MessageRead (obj_idx, msg_info, data, size, 1U); }
#endif

/**
  \fn          int32_t CANx_Control (uint32_t control, uint32_t arg, uint8_t x)
  \brief       Control CAN interface.
  \param[in]   control  Operation
                 - ARM_CAN_SET_FD_MODE :            set FD operation mode
                 - ARM_CAN_ABORT_MESSAGE_SEND :     abort sending of CAN message
                 - ARM_CAN_CONTROL_RETRANSMISSION : enable/disable automatic retransmission
                 - ARM_CAN_SET_TRANSCEIVER_DELAY :  set transceiver delay
  \param[in]   arg      Argument of operation
  \param[in]   x        Controller number (0..1)
  \return      execution status
*/
static int32_t CANx_Control (uint32_t control, uint32_t arg, uint8_t x) {

  switch (control & ARM_CAN_CONTROL_Msk) {
    case ARM_CAN_ABORT_MESSAGE_SEND:
      if ((arg + 1U) >= CAN_DRV_CONFIG[x].TOT_OBJ_NUM)      { return ARM_DRIVER_ERROR_PARAMETER; }
      if ((can_obj_tx[x][arg/32U] & (1U<<(arg%32U))) == 0U) { return ARM_DRIVER_ERROR;           }
      if (can_driver_powered[x] == 0U)                      { return ARM_DRIVER_ERROR;           }
      FLEXCAN_TransferAbortSend(can_base[x], &flexcan_handle[x], arg + CAN_DRV_CONFIG[x].RX_MBX_OBJ_OFS + 1U);
      break;

    case ARM_CAN_CONTROL_RETRANSMISSION:
      return ARM_DRIVER_ERROR_UNSUPPORTED;

    case ARM_CAN_SET_FD_MODE:
      return ARM_DRIVER_ERROR_UNSUPPORTED;

    case ARM_CAN_SET_TRANSCEIVER_DELAY:
      return ARM_DRIVER_ERROR_UNSUPPORTED;

    default:
      return ARM_DRIVER_ERROR_UNSUPPORTED;
  }

  return ARM_DRIVER_OK;
}
#if (DRIVER_CAN1 == 1U)
static int32_t CAN1_Control (uint32_t control, uint32_t arg) { return CANx_Control (control, arg, 0U); }
#endif
#if (DRIVER_CAN2 == 1U)
static int32_t CAN2_Control (uint32_t control, uint32_t arg) { return CANx_Control (control, arg, 1U); }
#endif

/**
  \fn          ARM_CAN_STATUS CAN1_GetStatus ()
  \fn          ARM_CAN_STATUS CAN2_GetStatus ()
  \brief       Get CAN status.
  \return      CAN status ARM_CAN_STATUS
*/
#if (DRIVER_CAN1 == 1U)
static ARM_CAN_STATUS CAN1_GetStatus (void) { return can_status[0]; }
#endif
#if (DRIVER_CAN2 == 1U)
static ARM_CAN_STATUS CAN2_GetStatus (void) { return can_status[1]; }
#endif


// Callback function called from IRQ context
static void IRQ_Callback(CAN_Type *base, flexcan_handle_t *handle, status_t status, uint32_t result, void *userData) {
  uint32_t x, esr1, last_state, obj_idx;

  if (base == can_base[0]) {
    x = 0U;
  } else {
    x = 1U;
  }

  switch (status) {
    case kStatus_FLEXCAN_ErrorStatus:
      esr1 = base->ESR1;
      last_state = can_status[x].unit_state; 

      // Register current unit state status
      switch ((esr1 & CAN_ESR1_FLTCONF_MASK) >> CAN_ESR1_FLTCONF_SHIFT) {
        case 0U:                        // Error Active
          can_status[x].unit_state = ARM_CAN_UNIT_STATE_ACTIVE;
          break;
        case 1U:                        // Error Passive
          can_status[x].unit_state = ARM_CAN_UNIT_STATE_PASSIVE;
          break;
        case 2U:                        // Bus-off
          can_status[x].unit_state = ARM_CAN_UNIT_STATE_BUS_OFF;
          break;
        case 3U:                        // Bus-off
          can_status[x].unit_state = ARM_CAN_UNIT_STATE_BUS_OFF;
          break;
      }

      // Register last error code
      if      ((esr1 & CAN_ESR1_BIT1ERR_MASK) != 0U) { can_status[x].last_error_code = ARM_CAN_LEC_BIT_ERROR;   }
      else if ((esr1 & CAN_ESR1_BIT0ERR_MASK) != 0U) { can_status[x].last_error_code = ARM_CAN_LEC_BIT_ERROR;   }
      else if ((esr1 & CAN_ESR1_ACKERR_MASK)  != 0U) { can_status[x].last_error_code = ARM_CAN_LEC_ACK_ERROR;   }
      else if ((esr1 & CAN_ESR1_CRCERR_MASK)  != 0U) { can_status[x].last_error_code = ARM_CAN_LEC_CRC_ERROR;   }
      else if ((esr1 & CAN_ESR1_FRMERR_MASK)  != 0U) { can_status[x].last_error_code = ARM_CAN_LEC_FORM_ERROR;  }
      else if ((esr1 & CAN_ESR1_STFERR_MASK)  != 0U) { can_status[x].last_error_code = ARM_CAN_LEC_STUFF_ERROR; }
      else                                           { can_status[x].last_error_code = ARM_CAN_LEC_NO_ERROR;    }

      // Estimate error counter values
      switch ((esr1 & CAN_ESR1_FLTCONF_MASK) >> CAN_ESR1_FLTCONF_SHIFT) {
        case 0U:                        // Error Active
          if ((esr1 & CAN_ESR1_TXWRN_MASK) != 0U) {
            can_status[x].tx_error_count = 96U;
          } else {
            can_status[x].tx_error_count = 0U;
          }
          if ((esr1 & CAN_ESR1_RXWRN_MASK) != 0U) {
            can_status[x].rx_error_count = 96U;
          } else {
            can_status[x].rx_error_count = 0U;
          }
          break;
        case 1U:                        // Error Passive
          can_status[x].tx_error_count = 128U;
          can_status[x].rx_error_count = 128U;
          break;
        case 2U:                        // Bus-off
          can_status[x].tx_error_count = 255U;
          can_status[x].rx_error_count = 255U;
          break;
        case 3U:                        // Bus-off
          can_status[x].tx_error_count = 255U;
          can_status[x].rx_error_count = 255U;
          break;
      }

      if      ((result & kFLEXCAN_BusOffIntFlag) != 0U) { CAN_SignalUnitEvent[x](ARM_CAN_EVENT_UNIT_BUS_OFF); }
      else if ((last_state ^ can_status[x].unit_state) != 0U) {
        // If unit bus status changed since last call
        switch (can_status[x].unit_state) {
          case ARM_CAN_UNIT_STATE_ACTIVE:
            CAN_SignalUnitEvent[x](ARM_CAN_EVENT_UNIT_ACTIVE);
            break;
          case ARM_CAN_UNIT_STATE_PASSIVE:
            CAN_SignalUnitEvent[x](ARM_CAN_EVENT_UNIT_PASSIVE);
            break;
          case ARM_CAN_UNIT_STATE_BUS_OFF:
            CAN_SignalUnitEvent[x](ARM_CAN_EVENT_UNIT_BUS_OFF);
            break;
          default:
            break;
        }
      }
      break;

    case kStatus_FLEXCAN_RxFifoIdle:
      CAN_SignalObjectEvent[x](0U, ARM_CAN_EVENT_RECEIVE);
      break;

    case kStatus_FLEXCAN_RxFifoOverflow:
      CAN_SignalObjectEvent[x](0U, ARM_CAN_EVENT_RECEIVE_OVERRUN);
      break;

    case kStatus_FLEXCAN_RxIdle:
      obj_idx = result;
      if (obj_idx > CAN_DRV_CONFIG[x].RX_MBX_OBJ_OFS) {
        obj_idx = obj_idx - CAN_DRV_CONFIG[x].RX_MBX_OBJ_OFS - 1U;
      }
      CAN_SignalObjectEvent[x](obj_idx, ARM_CAN_EVENT_RECEIVE);
      break;

    case kStatus_FLEXCAN_RxOverflow:
      obj_idx = result;
      if (obj_idx > CAN_DRV_CONFIG[x].RX_MBX_OBJ_OFS) {
        obj_idx = obj_idx - CAN_DRV_CONFIG[x].RX_MBX_OBJ_OFS - 1U;
      }
      CAN_SignalObjectEvent[x](obj_idx, ARM_CAN_EVENT_RECEIVE_OVERRUN);
      break;

    case kStatus_FLEXCAN_TxIdle:
      obj_idx = result - CAN_DRV_CONFIG[x].RX_MBX_OBJ_OFS - 1U;
      CAN_SignalObjectEvent[x](obj_idx, ARM_CAN_EVENT_SEND_COMPLETE);
      break;

    default:
      break;
  }
}


#if (DRIVER_CAN1 == 1U)
ARM_DRIVER_CAN Driver_CAN1 = {
  CAN_GetVersion,
  CAN1_GetCapabilities,
  CAN1_Initialize,
  CAN1_Uninitialize,
  CAN1_PowerControl,
  CAN_GetClock,
  CAN1_SetBitrate,
  CAN1_SetMode,
  CAN1_ObjectGetCapabilities,
  CAN1_ObjectSetFilter,
  CAN1_ObjectConfigure,
  CAN1_MessageSend,
  CAN1_MessageRead,
  CAN1_Control,
  CAN1_GetStatus
};
#endif

#if (DRIVER_CAN2 == 1U)
ARM_DRIVER_CAN Driver_CAN2 = {
  CAN_GetVersion,
  CAN2_GetCapabilities,
  CAN2_Initialize,
  CAN2_Uninitialize,
  CAN2_PowerControl,
  CAN_GetClock,
  CAN2_SetBitrate,
  CAN2_SetMode,
  CAN2_ObjectGetCapabilities,
  CAN2_ObjectSetFilter,
  CAN2_ObjectConfigure,
  CAN2_MessageSend,
  CAN2_MessageRead,
  CAN2_Control,
  CAN2_GetStatus
};
#endif

/*! \endcond */
