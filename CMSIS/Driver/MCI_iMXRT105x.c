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
 * $Revision:    V1.5
 *
 * Driver:       Driver_MCI0
 * Configured:   pin/clock configuration via MCUXpresso Config Tools
 * Project:      CMSIS MCI Driver for NXP i.MX RT 105x Series
 * --------------------------------------------------------------------------
 * Use the following configuration settings in the middleware component
 * to connect to this driver.
 *
 *   Configuration Setting                 Value   Interface
 *   ---------------------                 -----   ----------
 *   Connect to hardware via Driver_MCI# = 0       use USDHC1
 *   Connect to hardware via Driver_MCI# = 1       use USDHC2
 * -------------------------------------------------------------------------- */

/* History:
 *  Version 1.5
 *    Added volatile qualifier to volatile variables
 *  Version 1.4
 *    Placed DMA descriptors into noncacheable section
 *  Version 1.3
 *    Updated to SDK_2.9.1
 *  Version 1.2
 *    Added define instances (DRIVER_MCI0, DRIVER_MCI1)
 *  Version 1.1
 *    Get InterruptFlags from function USDHC_GetInterruptStatusFlags ()
 *  Version 1.0
 *    Initial release
 */

/*! \page evkb_imxrt1050_usdhc CMSIS-Driver for MCI Interface

<b>CMSIS-Driver for MCI Interface Setup</b>

The CMSIS-Driver for MCI Interface requires:
  - USDHC Pins
  - USDHC Clock
 
Valid pin settings for <b>USDHC1</b> peripheral on <b>EVKB-IMXRT1050</b> evaluation board are listed in the table below:
 
|  #  | Peripheral | Signal     | Route to      | Identifier | Direction     | Pull Up/Down Config | Pull/Keeper select | Pull/Keeper enable | Drive strength | Slew rate |
|:----|:-----------|:-----------|:--------------|:-----------|:--------------|:--------------------|:-------------------|:-------------------|:---------------|:----------|
| J2  | USDHC1     | DATA,3     | GPIO_SD_B0_05 | SD1_D3     | Input/Output  | 47k Ohm Pull Up     | Pull               | Enabled            | R0             | Fast      |
| H2  | USDHC1     | DATA,2     | GPIO_SD_B0_04 | SD1_D2     | Input/Output  | 47k Ohm Pull Up     | Pull               | Enabled            | R0             | Fast      |
| K1  | USDHC1     | DATA,1     | GPIO_SD_B0_03 | SD1_D1     | Input/Output  | 47k Ohm Pull Up     | Pull               | Enabled            | R0             | Fast      |
| J1  | USDHC1     | DATA,0     | GPIO_SD_B0_02 | SD1_D0     | Input/Output  | 47k Ohm Pull Up     | Pull               | Enabled            | R0             | Fast      |
| J4  | USDHC1     | CMD        | GPIO_SD_B0_00 | SD1_CMD    | Input/Output  | 47k Ohm Pull Up     | Pull               | Enabled            | R0             | Fast      |
| J3  | USDHC1     | CLK        | GPIO_SD_B0_01 | SD1_CLK    | Output        | 100k Ohm Pull Down  | Keeper             | Disabled           | R0             | Fast      |
| D13 | GPIO2      | goio_io,28 | GPIO_B1_12    | SD1_CD     | Not specified | 100k Ohm Pull Down  | Keeper             | Enabled            | R0/6           | Slow      |
 
The rest of the settings can be at the default (reset) value.

For other boards or custom hardware, refer to the hardware schematics to reflect correct setup values.
 
This driver uses special signal identifiers in order to enable SD Card Detect and Write Protect pin functionality.
These identifiers can be optionally used and must be assigned to signal connected to card detect or write protect pin on SD card socket.
Each MCI driver instance uses its own identifiers which can be assigned to signals as follows:
 
|  MCI instance | Peripheral | Identifier | Functionality |
|:--------------|:-----------|:-----------|:--------------|
|       0       |   USDHC1   |   SD1_CD   |  Card Detect  |
|       0       |   USDHC1   |   SD1_WP   | Write Protect |
|       1       |   USDHC2   |   SD2_CD   |  Card Detect  |
|       1       |   USDHC2   |   SD2_WP   | Write Protect |
 
In the \ref config_pinclock "MCUXpresso Config Tools", make sure that the following pin and clock settings are made (enter
the values that are shown in <em>italics</em>):
 
-# Under the \b Functional \b Group entry in the toolbar, select <b>Board_InitUSDHC</b>
-# Modify Routed Pins configuration to match the settings listed in the table above
-# Click on the flag next to <b>Functional Group BOARD_InitUSDHC</b> to enable calling that function from initialization code
-# Go to <b>Tools - Clocks</b>
-# Go to <b>Views - Details</b> and configure USDHC1_CLK_ROOT to frequency below or equal to <em>198MHz</em>.
   USDHC1_CLK_ROOT source can be selected from PLL2_PFD2_CLK or PLL2_PFD0_CLK which must be configured accordingly.
-# Click on <b>Update Project</b> button to update source files
*/

/*! \cond */

/* Define instances */
#ifndef DRIVER_MCI0
  #define DRIVER_MCI0             1
#endif

#ifndef DRIVER_MCI1
  #define DRIVER_MCI1             0
#endif

#ifndef MCI_ADMA_DESCR_CNT
  /* Define number of ADMA descriptors */
  #define MCI_ADMA_DESCR_CNT 8U
#endif

#include "MCI_iMXRT105x.h"

#define ARM_MCI_DRV_VERSION ARM_DRIVER_VERSION_MAJOR_MINOR(1,5)  /* driver version */

/* Driver Capabilities */
#if (DRIVER_MCI0)
#define MCI0_DriverCapabilities {                                        \
  MCI0_CD_EN,                                     /* cd_state          */\
  0U,                                             /* cd_event          */\
  MCI0_WP_EN,                                     /* wp_state          */\
  0U,                                             /* vdd               */\
  0U,                                             /* vdd_1v8           */\
  0U,                                             /* vccq              */\
  0U,                                             /* vccq_1v8          */\
  0U,                                             /* vccq_1v2          */\
  MCI0_BUS_WIDTH_4,                               /* data_width_4      */\
  MCI0_BUS_WIDTH_8,                               /* data_width_8      */\
  0U,                                             /* data_width_4_ddr  */\
  0U,                                             /* data_width_8_ddr  */\
  1U,                                             /* high_speed        */\
  0U,                                             /* uhs_signaling     */\
  0U,                                             /* uhs_tuning        */\
  0U,                                             /* uhs_sdr50         */\
  0U,                                             /* uhs_sdr104        */\
  0U,                                             /* uhs_ddr50         */\
  0U,                                             /* uhs_driver_type_a */\
  0U,                                             /* uhs_driver_type_c */\
  0U,                                             /* uhs_driver_type_d */\
  1U,                                             /* sdio_interrupt    */\
  0U,                                             /* read_wait         */\
  0U,                                             /* suspend_resume    */\
  0U,                                             /* mmc_interrupt     */\
  0U,                                             /* mmc_boot          */\
  0U,                                             /* rst_n             */\
  0U,                                             /* ccs               */\
  0U,                                             /* ccs_timeout       */\
  0U                                                                     \
}
#endif

#if (DRIVER_MCI1)
#define MCI1_DriverCapabilities {                                        \
  MCI1_CD_EN,                                     /* cd_state          */\
  0U,                                             /* cd_event          */\
  MCI1_WP_EN,                                     /* wp_state          */\
  0U,                                             /* vdd               */\
  0U,                                             /* vdd_1v8           */\
  0U,                                             /* vccq              */\
  0U,                                             /* vccq_1v8          */\
  0U,                                             /* vccq_1v2          */\
  MCI1_BUS_WIDTH_4,                               /* data_width_4      */\
  MCI1_BUS_WIDTH_8,                               /* data_width_8      */\
  0U,                                             /* data_width_4_ddr  */\
  0U,                                             /* data_width_8_ddr  */\
  1U,                                             /* high_speed        */\
  0U,                                             /* uhs_signaling     */\
  0U,                                             /* uhs_tuning        */\
  0U,                                             /* uhs_sdr50         */\
  0U,                                             /* uhs_sdr104        */\
  0U,                                             /* uhs_ddr50         */\
  0U,                                             /* uhs_driver_type_a */\
  0U,                                             /* uhs_driver_type_c */\
  0U,                                             /* uhs_driver_type_d */\
  1U,                                             /* sdio_interrupt    */\
  0U,                                             /* read_wait         */\
  0U,                                             /* suspend_resume    */\
  0U,                                             /* mmc_interrupt     */\
  0U,                                             /* mmc_boot          */\
  0U,                                             /* rst_n             */\
  0U,                                             /* ccs               */\
  0U,                                             /* ccs_timeout       */\
  0U                                                                     \
}
#endif

/* Resources */
#if (DRIVER_MCI0)
static MCI_CTRL MCI0;
AT_NONCACHEABLE_SECTION (static uint32_t MCI0_AdmaT[MCI_ADMA_DESCR_CNT]);

/* MCI0: Card Detect pin */
#if (MCI0_CD_EN != 0)
static MCI_IO mci0_cd = {
  MCI0_CD_PORT,
  MCI0_CD_PIN,
  MCI0_CD_ACTIVE
};
  #define MCI0_CD_INFO  &mci0_cd
#else
  #define MCI0_CD_INFO  NULL
#endif

/* MCI0: Write Protect pin */
#if (MCI0_WP_EN != 0)
static MCI_IO mci0_wp = {
  MCI0_WP_PORT,
  MCI0_WP_PIN,
  MCI0_WP_ACTIVE
};
  #define MCI0_WP_INFO  &mci0_wp
#else
  #define MCI0_WP_INFO  NULL
#endif

static MCI_RESOURCES MCI0_Resources = {
  MCI0_DriverCapabilities,
  &MCI0,
  USDHC1,
  USDHC1_IRQn,
  MCI0_CD_INFO,
  MCI0_WP_INFO,
  { kUSDHC_DmaModeAdma2,
    kUSDHC_EnBurstLenForINCR,
    &MCI0_AdmaT[0],
    MCI_ADMA_DESCR_CNT
  }
};
#endif /* DRIVER_MCI0 */

#if (DRIVER_MCI1)
static MCI_CTRL MCI1;
AT_NONCACHEABLE_SECTION (static uint32_t MCI1_AdmaT[MCI_ADMA_DESCR_CNT]);

/* MCI1: Card Detect pin */
#if (MCI1_CD_EN != 0)
static MCI_IO mci1_cd = {
  MCI1_CD_PORT,
  MCI1_CD_PIN,
  MCI1_CD_ACTIVE
};
  #define MCI1_CD_INFO  &mci1_cd
#else
  #define MCI1_CD_INFO  NULL
#endif

/* MCI1: Write Protect pin */
#if (MCI1_WP_EN != 0)
static MCI_IO mci1_wp = {
  MCI1_WP_PORT,
  MCI1_WP_PIN,
  MCI1_WP_ACTIVE
};
  #define MCI1_WP_INFO  &mci1_wp
#else
  #define MCI1_WP_INFO  NULL
#endif

static MCI_RESOURCES MCI1_Resources = {
  MCI1_DriverCapabilities,
  &MCI1,
  USDHC2,
  USDHC2_IRQn,
  MCI1_CD_INFO,
  MCI1_WP_INFO,
  { kUSDHC_DmaModeAdma2,
    kUSDHC_EnBurstLenForINCR,
    &MCI1_AdmaT[0],
    MCI_ADMA_DESCR_CNT
  }
};
#endif /* DRIVER_MCI1 */

#if ((DRIVER_MCI0 != 0) || (DRIVER_MCI1 != 0))

/* Callback functions */
void CardInserted  (USDHC_Type *base, void *userData);
void CardRemoved   (USDHC_Type *base, void *userData);
void SDIOInterrupt (USDHC_Type *base, void *userData);
void SDIOBlockGap  (USDHC_Type *base, void *userData);
void TransferComplete(USDHC_Type *base, usdhc_handle_t *handle, status_t status, void *userData);

static const usdhc_transfer_callback_t MCI_Cb = {
  CardInserted,
  CardRemoved,
  SDIOInterrupt,
  SDIOBlockGap,
  TransferComplete,
  NULL /* ReTuning */
};


/* Driver Version */
static const ARM_DRIVER_VERSION DriverVersion = {
  ARM_MCI_API_VERSION,
  ARM_MCI_DRV_VERSION
};


/**
  \fn          ARM_DRV_VERSION GetVersion (void)
  \brief       Get driver version.
  \return      \ref ARM_DRV_VERSION
*/
static ARM_DRIVER_VERSION GetVersion (void) {
  return DriverVersion;
}


/**
  \fn          ARM_MCI_CAPABILITIES MCI_GetCapabilities (MCI_RESOURCES *mci)
  \brief       Get driver capabilities.
  \return      \ref ARM_MCI_CAPABILITIES
*/
static ARM_MCI_CAPABILITIES GetCapabilities (MCI_RESOURCES *mci) {
  return (mci->capab);
}


/**
  \fn            int32_t Initialize (ARM_MCI_SignalEvent_t cb_event, MCI_RESOURCES *mci)
  \brief         Initialize the Memory Card Interface
  \param[in]     cb_event  Pointer to \ref ARM_MCI_SignalEvent
  \return        \ref execution_status
*/
static int32_t Initialize (ARM_MCI_SignalEvent_t cb_event, MCI_RESOURCES *mci) {

  if (mci->ctrl->flags & MCI_INIT)  { return ARM_DRIVER_OK; }

  mci->ctrl->cb_event = cb_event;

  /* Clear status */
  mci->ctrl->status.command_active   = 0U;
  mci->ctrl->status.command_timeout  = 0U;
  mci->ctrl->status.command_error    = 0U;
  mci->ctrl->status.transfer_active  = 0U;
  mci->ctrl->status.transfer_timeout = 0U;
  mci->ctrl->status.transfer_error   = 0U;
  mci->ctrl->status.sdio_interrupt   = 0U;
  mci->ctrl->status.ccs              = 0U;
  
  /* Register command control structure */
  mci->ctrl->xfer.command = &mci->ctrl->cmd;
  
  /* Register instance resources (callback argument) */
  mci->ctrl->h.userData = (void *)(uint32_t)mci;

  mci->ctrl->flags = MCI_INIT;

  return ARM_DRIVER_OK;
}


/**
  \fn            int32_t Uninitialize (MCI_RESOURCES *mci) {
  \brief         De-initialize Memory Card Interface.
  \return        \ref execution_status
*/
static int32_t Uninitialize (MCI_RESOURCES *mci) {

  mci->ctrl->flags = 0U;

  return ARM_DRIVER_OK;
}


/**
  \fn            int32_t PowerControl (ARM_POWER_STATE state, MCI_RESOURCES *mci)
  \brief         Control Memory Card Interface Power.
  \param[in]     state   Power state \ref ARM_POWER_STATE
  \return        \ref execution_status
*/
static int32_t PowerControl (ARM_POWER_STATE state, MCI_RESOURCES *mci) {
  usdhc_config_t cfg;

  switch (state) {
    case ARM_POWER_OFF:
      /* Disable SDHC interrupt in NVIC */
      NVIC_DisableIRQ((IRQn_Type)mci->irqn);

      /* Clear flags */
      mci->ctrl->flags = MCI_POWER;

      /* Clear status */
      mci->ctrl->status.command_active   = 0U;
      mci->ctrl->status.command_timeout  = 0U;
      mci->ctrl->status.command_error    = 0U;
      mci->ctrl->status.transfer_active  = 0U;
      mci->ctrl->status.transfer_timeout = 0U;
      mci->ctrl->status.transfer_error   = 0U;
      mci->ctrl->status.sdio_interrupt   = 0U;
      mci->ctrl->status.ccs              = 0U;

      /* Disable peripheral interrupts */
      USDHC_DisableInterruptSignal (mci->reg, kUSDHC_AllInterruptFlags);

      /* Reset SDHC peripheral */
      USDHC_Reset(mci->reg, kUSDHC_ResetAll, 100);

      /* Uninitialize peripheral */
      USDHC_Deinit(mci->reg);
      break;

    case ARM_POWER_FULL:
      if ((mci->ctrl->flags & MCI_POWER) == 0) {
        /* Clear response variable */
        mci->ctrl->response = NULL;

        /* Setup default peripheral configuration */
        cfg.dataTimeout         = 0xFU;
        cfg.endianMode          = kUSDHC_EndianModeLittle;
        cfg.readWatermarkLevel  = 128U;
        cfg.writeWatermarkLevel = 128U;
        cfg.readBurstLen        = 16U;
        cfg.writeBurstLen       = 16U;

        /* Enable clock, reset and initialize peripheral */
        USDHC_Init (mci->reg, &cfg);

        /* Create data transfer handle and enable interrupts in NVIC */
        USDHC_TransferCreateHandle (mci->reg, &mci->ctrl->h, &MCI_Cb, (void *)mci->ctrl);

        mci->ctrl->flags |= MCI_POWER;
      }
      break;

    case ARM_POWER_LOW:
    default:
      return ARM_DRIVER_ERROR_UNSUPPORTED;
  }
  return ARM_DRIVER_OK;
}


/**
  \fn            int32_t CardPower (uint32_t voltage, MCI_RESOURCES *mci)
  \brief         Set Memory Card supply voltage.
  \param[in]     voltage  Memory Card supply voltage
  \return        \ref execution_status
*/
static int32_t CardPower (uint32_t voltage, MCI_RESOURCES *mci) {
  (void)voltage;

  if ((mci->ctrl->flags & MCI_POWER) == 0U) { return ARM_DRIVER_ERROR; }
  return ARM_DRIVER_ERROR_UNSUPPORTED;
}


/**
  \fn            int32_t ReadCD (MCI_RESOURCES *mci) {
  \brief         Read Card Detect (CD) state.
  \return        1:card detected, 0:card not detected, or error
*/
static int32_t ReadCD (MCI_RESOURCES *mci) {

#if ((MCI0_CD_EN != 0) || (MCI0_CD_EN != 0))
  if (mci->ctrl->flags & MCI_POWER) {
    if (GPIO_PinRead(mci->cd->port, mci->cd->pin) == mci->cd->active) {
      return (1);
    }
  }
#endif
  return (0);
}


/**
  \fn            int32_t ReadWP (MCI_RESOURCES *mci) {
  \brief         Read Write Protect (WP) state.
  \return        1:write protected, 0:not write protected, or error
*/
static int32_t ReadWP (MCI_RESOURCES *mci) {

#if ((MCI0_WP_EN != 0) || (MCI1_WP_EN != 0))
  if (mci->ctrl->flags & MCI_POWER) {
    if (GPIO_PinRead(mci->wp->port, mci->wp->pin) == mci->wp->active) {
      return (1);
    }
  }
#endif
  return (0);
}


/**
  \fn            int32_t SendCommand (uint32_t  cmd,
                                      uint32_t  arg,
                                      uint32_t  flags,
                                      uint32_t *response, MCI_RESOURCES *mci)
  \brief         Send Command to card and get the response.
  \param[in]     cmd       Memory Card command
  \param[in]     arg       Command argument
  \param[in]     flags     Command flags
  \param[out]    response  Pointer to buffer for response
  \return        \ref execution_status
*/
static int32_t SendCommand (uint32_t cmd, uint32_t arg, uint32_t flags, uint32_t *response, MCI_RESOURCES *mci) {
  usdhc_adma_config_t *dma_cfg;

  if (((flags & MCI_RESPONSE_EXPECTED_Msk) != 0U) && (response == NULL)) {
    return ARM_DRIVER_ERROR_PARAMETER;
  }
  if ((mci->ctrl->flags & MCI_SETUP) == 0U) {
    return ARM_DRIVER_ERROR;
  }
  if (mci->ctrl->status.command_active) {
    return ARM_DRIVER_ERROR_BUSY;
  }

  if (flags & ARM_MCI_CARD_INITIALIZE) {
    USDHC_SetCardActive (mci->reg, 1000);
  }

  mci->ctrl->flags |= MCI_CMD;

  mci->ctrl->status.command_active   = 1U;
  mci->ctrl->status.command_timeout  = 0U;
  mci->ctrl->status.command_error    = 0U;
  mci->ctrl->status.transfer_timeout = 0U;
  mci->ctrl->status.transfer_error   = 0U;
  mci->ctrl->status.ccs              = 0U;

  mci->ctrl->cmd.index    = cmd & 0xFF;
  mci->ctrl->cmd.argument = arg;
  mci->ctrl->cmd.type     = kCARD_CommandTypeNormal;

  if (mci->ctrl->cmd.index == 12) {
    /* STOP_TRANSMISSION is the abort command */
    mci->ctrl->cmd.type = kCARD_CommandTypeAbort;
  }

  mci->ctrl->response =  response;
  mci->ctrl->flags   &= ~MCI_RESP_LONG;

  switch (flags & ARM_MCI_RESPONSE_Msk) {
    case ARM_MCI_RESPONSE_NONE:
      /* No response expected */
      mci->ctrl->cmd.responseType = kCARD_ResponseTypeNone;
      break;

    case ARM_MCI_RESPONSE_SHORT:
      /* Short response expected */
      if (flags & ARM_MCI_RESPONSE_CRC) {
        /* CRC error check when R1, R6, R7 */
        mci->ctrl->cmd.responseType = kCARD_ResponseTypeR1;
      } else {
        /* No CRC error check when R3 or R4 */
        mci->ctrl->cmd.responseType = kCARD_ResponseTypeR3;
      }
      break;

    case ARM_MCI_RESPONSE_SHORT_BUSY:
      /* Short response with busy expected, R1b or R5b */
      mci->ctrl->cmd.responseType = kCARD_ResponseTypeR1b;
      break;

    case ARM_MCI_RESPONSE_LONG:
      mci->ctrl->flags |= MCI_RESP_LONG;
      /* Long response expected */
      mci->ctrl->cmd.responseType = kCARD_ResponseTypeR2;
      break;

    default:
      return ARM_DRIVER_ERROR;
  }

  if (flags & ARM_MCI_TRANSFER_DATA) {
    mci->ctrl->status.transfer_active = 1U;
    /* Set data setup info */
    mci->ctrl->xfer.data = &mci->ctrl->data;
    mci->ctrl->cmd.flags = kUSDHC_DataPresentFlag;
  } else {
    mci->ctrl->xfer.data = NULL;
    mci->ctrl->cmd.flags = 0U;
  }
  
  dma_cfg = (usdhc_adma_config_t *)((uint32_t)&mci->dma);

  if (kStatus_Success != USDHC_TransferNonBlocking (mci->reg, &mci->ctrl->h, dma_cfg, &mci->ctrl->xfer)) {
    return ARM_DRIVER_ERROR;
  }

  return ARM_DRIVER_OK;
}


/**
  \fn            int32_t SetupTransfer (uint8_t *data,
                                        uint32_t block_count,
                                        uint32_t block_size,
                                        uint32_t mode, MCI_RESOURCES *mci) {
  \brief         Setup read or write transfer operation.
  \param[in,out] data         Pointer to data block(s) to be written or read
  \param[in]     block_count  Number of blocks
  \param[in]     block_size   Size of a block in bytes
  \param[in]     mode         Transfer mode
  \return        \ref execution_status
*/
static int32_t SetupTransfer (uint8_t *data, uint32_t block_count, uint32_t block_size, uint32_t mode, MCI_RESOURCES *mci) {
  uint32_t data_addr = (uint32_t)data; /* DMA might require 4-byte aligned address */

  if ((data == NULL) || (block_count == 0U) || (block_size == 0U)) { return ARM_DRIVER_ERROR_PARAMETER; }

  if ((mci->ctrl->flags & MCI_SETUP) == 0U) {
    return ARM_DRIVER_ERROR;
  }
  if (mci->ctrl->status.transfer_active) {
    return ARM_DRIVER_ERROR_BUSY;
  }
  if (mode & ARM_MCI_TRANSFER_STREAM) {
    /* Stream or SDIO multibyte data transfer not supported by peripheral */
    return ARM_DRIVER_ERROR;
  }

  mci->ctrl->flags |= MCI_DATA;

  mci->ctrl->data.enableAutoCommand12 = false;
  mci->ctrl->data.enableIgnoreError   = false;
  mci->ctrl->data.blockSize           = block_size;
  mci->ctrl->data.blockCount          = block_count;

  if (mode & ARM_MCI_TRANSFER_WRITE) {
    /* Direction: From controller to card */
    mci->ctrl->data.rxData = NULL;
    mci->ctrl->data.txData = (const uint32_t *)data_addr;
  }
  else {
    /* Direction: From card to controller */
    mci->ctrl->data.rxData = (uint32_t *)data_addr;
    mci->ctrl->data.txData = NULL;
  }

  return (ARM_DRIVER_OK);
}


/**
  \fn            int32_t AbortTransfer (MCI_RESOURCES *mci)
  \brief         Abort current read/write data transfer.
  \return        \ref execution_status
*/
static int32_t AbortTransfer (MCI_RESOURCES *mci) {

  if ((mci->ctrl->flags & MCI_SETUP) == 0U) { return ARM_DRIVER_ERROR; }
 
  /* Disable interrupts */
  mci->reg->INT_SIGNAL_EN = 0;

  /* Software reset for DAT line */
  mci->reg->SYS_CTRL |= USDHC_SYS_CTRL_RSTD(1);

  mci->ctrl->status.command_active  = 0U;
  mci->ctrl->status.transfer_active = 0U;
  mci->ctrl->status.sdio_interrupt  = 0U;
  mci->ctrl->status.ccs             = 0U;

  /* Reset data transfer handle and re-enable interrupts */
  USDHC_TransferCreateHandle (mci->reg, &mci->ctrl->h, &MCI_Cb, (void *)mci->ctrl);

  return ARM_DRIVER_OK;
}


/**
  \fn            int32_t Control (uint32_t control, uint32_t arg, MCI_RESOURCES *mci)
  \brief         Control MCI Interface.
  \param[in]     control  Operation
  \param[in]     arg      Argument of operation (optional)
  \return        \ref execution_status
*/
static int32_t Control (uint32_t control, uint32_t arg, MCI_RESOURCES *mci) {
  uint32_t pclk;

  if ((mci->ctrl->flags & MCI_POWER) == 0U) { return ARM_DRIVER_ERROR; }

  switch (control) {
    case ARM_MCI_BUS_SPEED:
      pclk = CLOCK_GetFreq (kCLOCK_CoreSysClk);

      mci->ctrl->flags |= MCI_SETUP;
      return (int32_t)USDHC_SetSdClock (mci->reg, pclk, arg);

    case ARM_MCI_BUS_SPEED_MODE:
      switch (arg) {
        case ARM_MCI_BUS_DEFAULT_SPEED:
          /* Speed mode up to 25MHz */
        case ARM_MCI_BUS_HIGH_SPEED:
          /* Speed mode up to 50MHz */
          break;
        default: return ARM_DRIVER_ERROR_UNSUPPORTED;
      }
      break;

    case ARM_MCI_BUS_CMD_MODE:
      switch (arg) {
        case ARM_MCI_BUS_CMD_OPEN_DRAIN:
          /* Configure command line in open-drain mode */
          return ARM_DRIVER_ERROR;

        case ARM_MCI_BUS_CMD_PUSH_PULL:
          /* Configure command line in push-pull mode */
          break;

        default:
          return ARM_DRIVER_ERROR_UNSUPPORTED;
      }
      break;

    case ARM_MCI_BUS_DATA_WIDTH:
      switch (arg) {
        case ARM_MCI_BUS_DATA_WIDTH_1:
          USDHC_SetDataBusWidth (mci->reg, kUSDHC_DataBusWidth1Bit);
          break;
        case ARM_MCI_BUS_DATA_WIDTH_4:
          USDHC_SetDataBusWidth (mci->reg, kUSDHC_DataBusWidth4Bit);
          break;
        case ARM_MCI_BUS_DATA_WIDTH_8:
          USDHC_SetDataBusWidth (mci->reg, kUSDHC_DataBusWidth8Bit);
          break;
        default:
          return ARM_DRIVER_ERROR_UNSUPPORTED;
      }
      break;

    case ARM_MCI_CONTROL_CLOCK_IDLE:
      if (arg) {
        /* Clock generation enabled when idle */
        mci->reg->VEND_SPEC |= USDHC_VEND_SPEC_FRC_SDCLK_ON_MASK;
        mci->reg->SYS_CTRL |= (1<<0)/*USDHC_SYS_CTRL_IPGEN(1)*/ |
                            (1<<1)/*USDHC_SYS_CTRL_HCKEN(1)*/ |
                            (1<<2)/*USDHC_SYS_CTRL_PEREN(1)*/ | /*;*/
                            (1<<3);/*SDCLKEN*/
                            
      }
      else {
        /* Clock generation disabled when idle */
        mci->reg->VEND_SPEC &= ~USDHC_VEND_SPEC_FRC_SDCLK_ON_MASK;
      }
      break;

    case ARM_MCI_DATA_TIMEOUT:
      arg >>= 13;
      if (arg > 0x0E) {
        arg = 0x0E;
      }
      mci->reg->SYS_CTRL = (mci->reg->SYS_CTRL & ~USDHC_SYS_CTRL_DTOCV_MASK) | USDHC_SYS_CTRL_DTOCV(arg);
      break;

    case ARM_MCI_MONITOR_SDIO_INTERRUPT:
      mci->ctrl->status.sdio_interrupt = 0U;

      /* Clear SDIO Interrupt status */
      USDHC_ClearInterruptStatusFlags (mci->reg, kUSDHC_CardInterruptFlag);
      /* Enable SDIO interrupt */
      USDHC_EnableInterruptSignal (mci->reg, kUSDHC_CardInterruptFlag);
      break;

    default: return ARM_DRIVER_ERROR_UNSUPPORTED;
  }

  return ARM_DRIVER_OK;
}


/**
  \fn            ARM_MCI_STATUS GetStatus (MCI_RESOURCES *mci)
  \brief         Get MCI status.
  \return        MCI status \ref ARM_MCI_STATUS
*/
static ARM_MCI_STATUS GetStatus (MCI_RESOURCES *mci) {
  return mci->ctrl->status;
}


/**
  Unused callbacks
*/
void CardInserted (USDHC_Type *base, void *userData) { (void)base; (void)userData; }
void CardRemoved  (USDHC_Type *base, void *userData) { (void)base; (void)userData; }
void SDIOBlockGap (USDHC_Type *base, void *userData) { (void)base; (void)userData; }


/**
  SDIO interrupt callback
*/
void SDIOInterrupt (USDHC_Type *base, void *userData) {
  MCI_CTRL *ctrl = (MCI_CTRL *)userData;

  /* Disable and clear SDIO interrupt */
  USDHC_DisableInterruptSignal    (base, kUSDHC_CardInterruptFlag);
  USDHC_ClearInterruptStatusFlags (base, kUSDHC_CardInterruptFlag);

  /* SDIO interrupt */
  ctrl->status.sdio_interrupt = 1U;

  if (ctrl->cb_event != NULL) {
    ctrl->cb_event (ARM_MCI_EVENT_SDIO_INTERRUPT);
  }
}


/**
  Command or data transfer complete callback function
  
  \param[in]   base       peripheral base address
  \param[in]   handle     sdhc driver handle
  \param[in]   status     transfer execution status
  \param[in]   userdata   user specified content
*/
void TransferComplete(USDHC_Type *base, usdhc_handle_t *handle, status_t status, void *userData) {
  MCI_CTRL *ctrl = (MCI_CTRL *)userData;
  uint32_t event = 0U;

  (void)base;

  if((handle->data != NULL) && (status == kStatus_USDHC_SendCommandSuccess)) {
    return;
  }

  switch (status) {
    case kStatus_USDHC_SendCommandSuccess:
    case kStatus_USDHC_TransferDataComplete:
      if (ctrl->response) {
        /* Response is expected */
        if (ctrl->flags & MCI_RESP_LONG) {
          ctrl->response[0] = handle->command->response[0];
          ctrl->response[1] = handle->command->response[1];
          ctrl->response[2] = handle->command->response[2];
          ctrl->response[3] = handle->command->response[3];
        } else {
          ctrl->response[0] = handle->command->response[0];
        }
      }

      if (ctrl->flags & MCI_CMD) {
        ctrl->flags &= ~MCI_CMD;
        /* Command event expected */
        ctrl->status.command_active = 0U;

        event |= ARM_MCI_EVENT_COMMAND_COMPLETE;
      }

      if (ctrl->flags & MCI_DATA) {
        ctrl->flags &= ~MCI_DATA;
        /* Transfer event expected */
        ctrl->status.transfer_active = 0U;

        event |= ARM_MCI_EVENT_TRANSFER_COMPLETE;
      }
      break;

    case kStatus_USDHC_SendCommandFailed:
      event = ARM_MCI_EVENT_COMMAND_ERROR;

      if (USDHC_GetInterruptStatusFlags(base) & kUSDHC_DataTimeoutFlag) {
        event |= ARM_MCI_EVENT_COMMAND_TIMEOUT;
      }
      break;

    case kStatus_USDHC_TransferDataFailed:
      event = ARM_MCI_EVENT_TRANSFER_ERROR;

      if (USDHC_GetInterruptStatusFlags(base) & kUSDHC_DataTimeoutFlag) {
        event |= ARM_MCI_EVENT_TRANSFER_TIMEOUT;
      }
      break;
  }

  if (event && (ctrl->cb_event != NULL)) {
    ctrl->cb_event (event);
  }
}
#endif

#if (DRIVER_MCI0)
/* MCI0 driver wrapper functions */
static ARM_MCI_CAPABILITIES MCI0_GetCapabilities (void) {
  return GetCapabilities (&MCI0_Resources);
}
static int32_t MCI0_Initialize (ARM_MCI_SignalEvent_t cb_event) {
  return Initialize (cb_event, &MCI0_Resources);
}
static int32_t MCI0_Uninitialize (void) {
  return Uninitialize (&MCI0_Resources);
}
static int32_t MCI0_PowerControl (ARM_POWER_STATE state) {
  return PowerControl (state, &MCI0_Resources);
}
static int32_t MCI0_CardPower (uint32_t voltage) {
  return CardPower (voltage, &MCI0_Resources);
}
static int32_t MCI0_ReadCD (void) {
  return ReadCD (&MCI0_Resources);
}
static int32_t MCI0_ReadWP (void) {
  return ReadWP (&MCI0_Resources);
}
static int32_t MCI0_SendCommand (uint32_t cmd, uint32_t arg, uint32_t flags, uint32_t *response) {
  return SendCommand (cmd, arg, flags, response, &MCI0_Resources);
}
static int32_t MCI0_SetupTransfer (uint8_t *data, uint32_t block_count, uint32_t block_size, uint32_t mode) {
  return SetupTransfer (data, block_count, block_size, mode, &MCI0_Resources);
}
static int32_t MCI0_AbortTransfer (void) {
  return AbortTransfer (&MCI0_Resources);
}
static int32_t MCI0_Control (uint32_t control, uint32_t arg) {
  return Control (control, arg, &MCI0_Resources);
}
static ARM_MCI_STATUS MCI0_GetStatus (void) {
  return GetStatus (&MCI0_Resources);
}

/* MCI Driver Control Block */
ARM_DRIVER_MCI Driver_MCI0 = {
  GetVersion,
  MCI0_GetCapabilities,
  MCI0_Initialize,
  MCI0_Uninitialize,
  MCI0_PowerControl,
  MCI0_CardPower,
  MCI0_ReadCD,
  MCI0_ReadWP,
  MCI0_SendCommand,
  MCI0_SetupTransfer,
  MCI0_AbortTransfer,
  MCI0_Control,
  MCI0_GetStatus
};
#endif /* DRIVER_MCI0 */

#if (DRIVER_MCI1)
/* MCI1 driver wrapper functions */
static ARM_MCI_CAPABILITIES MCI1_GetCapabilities (void) {
  return GetCapabilities (&MCI1_Resources);
}
static int32_t MCI1_Initialize (ARM_MCI_SignalEvent_t cb_event) {
  return Initialize (cb_event, &MCI1_Resources);
}
static int32_t MCI1_Uninitialize (void) {
  return Uninitialize (&MCI1_Resources);
}
static int32_t MCI1_PowerControl (ARM_POWER_STATE state) {
  return PowerControl (state, &MCI1_Resources);
}
static int32_t MCI1_CardPower (uint32_t voltage) {
  return CardPower (voltage, &MCI1_Resources);
}
static int32_t MCI1_ReadCD (void) {
  return ReadCD (&MCI1_Resources);
}
static int32_t MCI1_ReadWP (void) {
  return ReadWP (&MCI1_Resources);
}
static int32_t MCI1_SendCommand (uint32_t cmd, uint32_t arg, uint32_t flags, uint32_t *response) {
  return SendCommand (cmd, arg, flags, response, &MCI1_Resources);
}
static int32_t MCI1_SetupTransfer (uint8_t *data, uint32_t block_count, uint32_t block_size, uint32_t mode) {
  return SetupTransfer (data, block_count, block_size, mode, &MCI1_Resources);
}
static int32_t MCI1_AbortTransfer (void) {
  return AbortTransfer (&MCI1_Resources);
}
static int32_t MCI1_Control (uint32_t control, uint32_t arg) {
  return Control (control, arg, &MCI1_Resources);
}
static ARM_MCI_STATUS MCI1_GetStatus (void) {
  return GetStatus (&MCI1_Resources);
}

/* MCI Driver Control Block */
ARM_DRIVER_MCI Driver_MCI1 = {
  GetVersion,
  MCI1_GetCapabilities,
  MCI1_Initialize,
  MCI1_Uninitialize,
  MCI1_PowerControl,
  MCI1_CardPower,
  MCI1_ReadCD,
  MCI1_ReadWP,
  MCI1_SendCommand,
  MCI1_SetupTransfer,
  MCI1_AbortTransfer,
  MCI1_Control,
  MCI1_GetStatus
};
#endif /* DRIVER_MCI1 */

/*! \endcond */
