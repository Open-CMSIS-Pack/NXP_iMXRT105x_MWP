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
 * $Revision:    V1.6
 *
 * Driver:       Driver_ETH_MAC0
 * Configured:   pin/clock configuration via MCUXpresso Config Tools
 * Project:      CMSIS Ethernet Media Access (MAC) Driver for 
 *               NXP i.MX RT 105x Series
 * --------------------------------------------------------------------------
 * Use the following configuration settings in the middleware component
 * to connect to this driver.
 *
 *   Configuration Setting                     Value
 *   ---------------------                     -----
 *   Connect to hardware via Driver_ETH_MAC# = 0
 * -------------------------------------------------------------------------- */

/* History:
 *  Version 1.6
 *    Added volatile qualifier to volatile variables
 *  Version 1.5
 *    Updated to SDK_2.14.0 (Deprecated ENET_SetCallback function)
 *  Version 1.4
 *    Fixed SendFrame function for high-speed data transfers
 *  Version 1.3
 *    Placed frame buffer descriptors into noncacheable section
 *  Version 1.2
 *    Updated to SDK_2.9.1
 *  Version 1.1
 *    Corrected compiler warnings
 *  Version 1.0
 *    Initial release
 */

/*! \page evkb_imxrt1050_enet CMSIS-Driver for EMAC Interface

<b>CMSIS-Driver for EMAC Interface Setup</b>

The CMSIS-Driver for EMAC Interface requires:
  - ENET Pins
  - ENET Clock
 
Valid pin settings for <b>ENET</b> peripheral on <b>EVKB-IMXRT1050</b> evaluation board are listed in the table below:
 
|  #  | Peripheral | Signal     | Route to      | Direction     | Software Input On | Pull/Keeper enable | Slew rate |
|:----|:-----------|:-----------|:--------------|:--------------|:------------------|:-------------------|:----------|
| A7  | ENET       | MDC        | GPIO_EMC_40   | Output        | Disabled          | Disabled           | Fast      |
| C7  | ENET       | MDIO       | GPIO_EMC_41   | Input/Output  | Disabled          | Disabled           | Fast      |
| B13 | ENET       | REF_CLK    | GPIO_B1_10    | Output        | Enabled           | Disabled           | Fast      |
| E12 | ENET       | RX_DATA,0  | GPIO_B1_04    | Input         | Disabled          | Disabled           | Fast      |
| D12 | ENET       | RX_DATA,1  | GPIO_B1_05    | Input         | Disabled          | Disabled           | Fast      |
| C12 | ENET       | RX_EN      | GPIO_B1_06    | Input         | Disabled          | Disabled           | Fast      |
| C13 | ENET       | RX_ER      | GPIO_B1_11    | Input         | Disabled          | Disabled           | Fast      |
| B12 | ENET       | TX_DATA,0  | GPIO_B1_07    | Output        | Disabled          | Disabled           | Fast      |
| A12 | ENET       | TX_DATA,1  | GPIO_B1_08    | Output        | Disabled          | Disabled           | Fast      |
| A13 | ENET       | TX_EN      | GPIO_B1_09    | Output        | Disabled          | Disabled           | Fast      |
| G13 | GPIO1      | goio_io,10 | GPIO_AD_B0_10 | Output        | Disabled          | Disabled           | Fast      |
| F14 | GPIO1      | gpio_io, 9 | GPIO_AD_B0_09 | Not specified | Disabled          | Enabled            | Slow      |
 
The rest of the settings can be at the default (reset) value.

For other boards or custom hardware, refer to the hardware schematics to reflect correct setup values.

This driver uses special signal identifiers in order to properly detect Media Interface mode (MII or RMII).
These identifiers are required when using MII mode and must be assigned to signals as follows:

|  Signal   |  Identifier |
|:----------|:------------|
| RX_DATA,2 |  ENET_RXD2  |
| RX_DATA,3 |  ENET_RXD3  |
| TX_DATA,2 |  ENET_TXD2  |
| TX_DATA,3 |  ENET_TXD3  |

 
In the \ref config_pinclock "MCUXpresso Config Tools", make sure that the following pin and clock settings are made (enter
the values that are shown in <em>italics</em>):
 
-# Under the \b Functional \b Group entry in the toolbar, select <b>Board_InitENET</b>
-# Modify Routed Pins configuration to match the settings listed in the table above
-# Click on the flag next to <b>Functional Group BOARD_InitENET</b> to enable calling that function from initialization code
-# Go to <b>Tools - Clocks</b>
-# Go to <b>Views - Details</b> and set ENET_125M_CLK value to <em>50MHz</em> and ENET_25M_REF_CLK value
   to <em>25MHz</em>. Configure PLL6 (ENET_PLL) by setting PLL Power down to <em>No</em> and PLL6 bypass to <em>PLL6 (ENET)
   output</em>.
-# Click on <b>Update Project</b> button to update source files
*/

/*! \cond */

#include "EMAC_iMXRT105x.h"


/* Receive/transmit checksum offload enabled by default */
#ifndef EMAC_CHECKSUM_OFFLOAD
  #define EMAC_CHECKSUM_OFFLOAD   1
#endif


#define ARM_ETH_MAC_DRV_VERSION ARM_DRIVER_VERSION_MAJOR_MINOR(1,6) /* driver version */

/* EMAC Memory Buffer configuration */
#define EMAC_BUF_SIZE           1536U   /* ETH Receive/Transmit buffer size   */
#define EMAC_RX_BUF_CNT         4U      /* 0x1800 for Rx (4*1536=6K)          */
#define EMAC_TX_BUF_CNT         2U      /* 0x0C00 for Tx (2*1536=3K)          */


/* Driver Version */
static const ARM_DRIVER_VERSION DriverVersion = {
  ARM_ETH_MAC_API_VERSION,
  ARM_ETH_MAC_DRV_VERSION
};

/* Driver Capabilities */
static const ARM_ETH_MAC_CAPABILITIES DriverCapabilities = {
  (EMAC_CHECKSUM_OFFLOAD != 0) ? 1U : 0U, /* checksum_offload_rx_ip4  */
  (EMAC_CHECKSUM_OFFLOAD != 0) ? 1U : 0U, /* checksum_offload_rx_ip6  */
  (EMAC_CHECKSUM_OFFLOAD != 0) ? 1U : 0U, /* checksum_offload_rx_udp  */
  (EMAC_CHECKSUM_OFFLOAD != 0) ? 1U : 0U, /* checksum_offload_rx_tcp  */
  (EMAC_CHECKSUM_OFFLOAD != 0) ? 1U : 0U, /* checksum_offload_rx_icmp */
  (EMAC_CHECKSUM_OFFLOAD != 0) ? 1U : 0U, /* checksum_offload_tx_ip4  */
  (EMAC_CHECKSUM_OFFLOAD != 0) ? 1U : 0U, /* checksum_offload_tx_ip6  */
  (EMAC_CHECKSUM_OFFLOAD != 0) ? 1U : 0U, /* checksum_offload_tx_udp  */
  (EMAC_CHECKSUM_OFFLOAD != 0) ? 1U : 0U, /* checksum_offload_tx_tcp  */
  (EMAC_CHECKSUM_OFFLOAD != 0) ? 1U : 0U, /* checksum_offload_tx_icmp */
  EMAC_MII_MODE,                          /* media_interface          */
  0U,                                     /* mac_address              */
  1U,                                     /* event_rx_frame           */
  1U,                                     /* event_tx_frame           */
  1U,                                     /* event_wakeup             */
  0U                                      /* precision_timer          */
#if (defined(ARM_ETH_MAC_API_VERSION) && (ARM_ETH_MAC_API_VERSION >= 0x201U))
, 0U                                      /* reserved bits            */
#endif
};


/* Frame buffers */
static __ALIGNED(64) uint8_t Rx_Buf[EMAC_RX_BUF_CNT][EMAC_BUF_SIZE];
static __ALIGNED(64) uint8_t Tx_Buf[EMAC_TX_BUF_CNT][EMAC_BUF_SIZE];

/* Frame buffer descriptors */
AT_NONCACHEABLE_SECTION_ALIGN (static volatile enet_rx_bd_struct_t Rx_Desc[EMAC_RX_BUF_CNT], 64U);
AT_NONCACHEABLE_SECTION_ALIGN (static volatile enet_tx_bd_struct_t Tx_Desc[EMAC_TX_BUF_CNT], 64U);

/* Intermediate tx frame buffer */
static uint8_t  Tx_IntBuf[EMAC_BUF_SIZE];
static uint32_t Tx_IntLen;

/* EMAC control structure */
static EMAC_INFO Emac = { 0 };

/* ENET interrupt handler callback function */
static void ENET_IRQCallback (ENET_Type *base, enet_handle_t *handle, enet_event_t event, enet_frame_info_t *frameInfo, void *userData);

/**
  \fn          ARM_DRIVER_VERSION ARM_ETH_MAC_GetVersion (void)
  \brief       Get driver version.
  \return      \ref ARM_DRIVER_VERSION
*/
static ARM_DRIVER_VERSION GetVersion (void) {
  return DriverVersion;
}


/**
  \fn          ARM_ETH_MAC_CAPABILITIES GetCapabilities (void)
  \brief       Get driver capabilities.
  \return      \ref ARM_ETH_MAC_CAPABILITIES
*/
static ARM_ETH_MAC_CAPABILITIES GetCapabilities (void) {
  return DriverCapabilities;
}


/**
  \fn          int32_t Initialize (ARM_ETH_MAC_SignalEvent_t cb_event)
  \brief       Initialize Ethernet MAC Device.
  \param[in]   cb_event  Pointer to \ref ARM_ETH_MAC_SignalEvent
  \return      \ref execution_status
*/
static int32_t Initialize (ARM_ETH_MAC_SignalEvent_t cb_event) {

  if (Emac.flags & EMAC_FLAG_INIT) { return ARM_DRIVER_OK; }

  /* Setup Tx/Rx descriptors */
  Emac.desc_cfg.txBdStartAddrAlign = &Tx_Desc[0];
  Emac.desc_cfg.txBufferAlign      = &Tx_Buf[0][0];
  Emac.desc_cfg.txBdNumber         =  EMAC_TX_BUF_CNT;
  Emac.desc_cfg.txBuffSizeAlign    =  EMAC_BUF_SIZE;
  Emac.desc_cfg.txMaintainEnable   = true;

  Emac.desc_cfg.rxBdStartAddrAlign = &Rx_Desc[0];
  Emac.desc_cfg.rxBufferAlign      = &Rx_Buf[0][0];
  Emac.desc_cfg.rxBdNumber         =  EMAC_RX_BUF_CNT;
  Emac.desc_cfg.rxBuffSizeAlign    =  EMAC_BUF_SIZE;
  Emac.desc_cfg.rxMaintainEnable   = true;

  /* Get ethernet peripheral clock */
  Emac.pclk = CLOCK_GetFreq (kCLOCK_IpgClk);

  /* Register driver callback function */
  Emac.cb_event = cb_event;
  Emac.flags    = EMAC_FLAG_INIT;

  return ARM_DRIVER_OK;
}

/**
  \fn          int32_t Uninitialize (void)
  \brief       De-initialize Ethernet MAC Device.
  \return      \ref execution_status
*/
static int32_t Uninitialize (void) {

  Emac.flags = 0U;

  return ARM_DRIVER_OK;
}

/**
  \fn          int32_t PowerControl (ARM_POWER_STATE state)
  \brief       Control Ethernet MAC Device Power.
  \param[in]   state  Power state
  \return      \ref execution_status
*/
static int32_t PowerControl (ARM_POWER_STATE state) {

  switch (state) {
    case ARM_POWER_OFF:
      ENET_Deinit (ENET);

      /* Disable ENET interrupts in NVIC */
      NVIC_DisableIRQ (ENET_IRQn);

      Emac.flags = EMAC_FLAG_INIT;
      break;

    case ARM_POWER_LOW:
      return ARM_DRIVER_ERROR_UNSUPPORTED;

    case ARM_POWER_FULL:
      if (Emac.flags & EMAC_FLAG_POWER) {
        /* Driver already powered */
        break;
      }

      /* Setup MAC configuration */
      #if (EMAC_CHECKSUM_OFFLOAD == 0U)
      Emac.cfg.macSpecialConfig = kENET_ControlStoreAndFwdDisable;
      #else
      Emac.cfg.macSpecialConfig = 0U;
      #endif

      Emac.cfg.interrupt     = kENET_TxFrameInterrupt | kENET_RxFrameInterrupt | kENET_WakeupInterrupt;
      Emac.cfg.rxMaxFrameLen = EMAC_BUF_SIZE;

      #if (EMAC_MII_MODE == ARM_ETH_INTERFACE_MII)
      Emac.cfg.miiMode   = kENET_MiiMode;
      #else
      Emac.cfg.miiMode   = kENET_RmiiMode;
      #endif
      Emac.cfg.miiSpeed  = kENET_MiiSpeed100M;
      Emac.cfg.miiDuplex = kENET_MiiFullDuplex;

      Emac.cfg.rxAccelerConfig      = 0U;
      Emac.cfg.txAccelerConfig      = 0U;
      Emac.cfg.pauseDuration        = 0U;
      Emac.cfg.rxFifoEmptyThreshold = 0U;
      Emac.cfg.rxFifoFullThreshold  = 0U;
      Emac.cfg.txFifoWatermark      = 0U;
      Emac.cfg.ringNum              = 1U;
      Emac.cfg.callback             = ENET_IRQCallback;

      /* Initialize and apply configuration */
      ENET_Init (ENET, &Emac.h, &Emac.cfg, &Emac.desc_cfg, Emac.addr, Emac.pclk);

      /* Enable ENET peripheral interrupts in NVIC */
      NVIC_EnableIRQ (ENET_IRQn);

      Emac.flags |= EMAC_FLAG_POWER;
      break;

    default:
      return ARM_DRIVER_ERROR_UNSUPPORTED;
  }

  return ARM_DRIVER_OK;
}

/**
  \fn          int32_t GetMacAddress (ARM_ETH_MAC_ADDR *ptr_addr)
  \brief       Get Ethernet MAC Address.
  \param[in]   ptr_addr  Pointer to address
  \return      \ref execution_status
*/
static int32_t GetMacAddress (ARM_ETH_MAC_ADDR *ptr_addr) {

  if (ptr_addr == NULL) {
    return ARM_DRIVER_ERROR_PARAMETER;
  }

  ENET_GetMacAddr (ENET, (uint8_t *)ptr_addr);

  return ARM_DRIVER_OK;
}

/**
  \fn          int32_t SetMacAddress (const ARM_ETH_MAC_ADDR *ptr_addr)
  \brief       Set Ethernet MAC Address.
  \param[in]   ptr_addr  Pointer to address
  \return      \ref execution_status
*/
static int32_t SetMacAddress (const ARM_ETH_MAC_ADDR *ptr_addr) {

  if (ptr_addr == NULL) {
    return ARM_DRIVER_ERROR_PARAMETER;
  }

  if ((Emac.flags & EMAC_FLAG_POWER) == 0) {
    /* Driver not yet powered */
    return ARM_DRIVER_ERROR;
  }

  memcpy (Emac.addr, ptr_addr, 6);

  ENET_SetMacAddr (ENET, (uint8_t *)ptr_addr);

  return ARM_DRIVER_OK;
}

/**
  \fn          int32_t SetAddressFilter (const ARM_ETH_MAC_ADDR *ptr_addr,
                                               uint32_t          num_addr)
  \brief       Configure Address Filter.
  \param[in]   ptr_addr  Pointer to addresses
  \param[in]   num_addr  Number of addresses to configure
  \return      \ref execution_status
*/
static int32_t SetAddressFilter (const ARM_ETH_MAC_ADDR *ptr_addr, uint32_t num_addr) {
  uint32_t cnt;

  if ((ptr_addr == NULL) && (num_addr != 0U)) {
    return ARM_DRIVER_ERROR_PARAMETER;
  }

  if ((Emac.flags & EMAC_FLAG_POWER) == 0U) {
    /* Driver not yet powered */
    return ARM_DRIVER_ERROR;
  }

  ENET->GALR = 0U;
  ENET->GAUR = 0U;

  if (num_addr != 0U) {
    for (cnt = 0U; cnt < num_addr; cnt++) {
      /* Add multicast group to hash table */
      ENET_AddMulticastGroup (ENET, (uint8_t *)&ptr_addr[cnt]);
    }
  }

  return ARM_DRIVER_OK;
}

/**
  \fn          int32_t SendFrame (const uint8_t *frame, uint32_t len, uint32_t flags)
  \brief       Send Ethernet frame.
  \param[in]   frame  Pointer to frame buffer with data to send
  \param[in]   len    Frame buffer length in bytes
  \param[in]   flags  Frame transmit flags (see ARM_ETH_MAC_TX_FRAME_...)
  \return      \ref execution_status
*/
static int32_t SendFrame (const uint8_t *frame, uint32_t len, uint32_t flags) {
  status_t stat = 0;

  if ((frame == NULL) || (len == 0U)) {
    return ARM_DRIVER_ERROR_PARAMETER;
  }

  if ((Emac.flags & EMAC_FLAG_POWER) == 0U) {
    /* Driver not yet powered */
    return ARM_DRIVER_ERROR;
  }

  if ((Tx_IntLen + len) > sizeof(Tx_IntBuf)) {
    /* Frame size invalid */
    return ARM_DRIVER_ERROR;
  }

  if ((Tx_IntLen == 0U) && ((flags & ARM_ETH_MAC_TX_FRAME_FRAGMENT) == 0)) {
    /* Frame not fragmented */
    stat = ENET_SendFrame (ENET, &Emac.h, frame, len, 0U, 0U, NULL);
  }
  else {
    memcpy (&Tx_IntBuf[Tx_IntLen], frame, len);
    Tx_IntLen += len;
    if ((flags & ARM_ETH_MAC_TX_FRAME_FRAGMENT) == 0) {
      /* Last fragment, send frame */
      stat = ENET_SendFrame (ENET, &Emac.h, Tx_IntBuf, Tx_IntLen, 0U, 0U, NULL);
      Tx_IntLen = 0U;
    }
  }
  if (stat == kStatus_ENET_TxFrameBusy) {
    return ARM_DRIVER_ERROR_BUSY;
  }

  return ARM_DRIVER_OK;
}

/**
  \fn          int32_t ReadFrame (uint8_t *frame, uint32_t len)
  \brief       Read data of received Ethernet frame.
  \param[in]   frame  Pointer to frame buffer for data to read into
  \param[in]   len    Frame buffer length in bytes
  \return      number of data bytes read or execution status
                 - value >= 0: number of data bytes read
                 - value < 0: error occurred, value is execution status as defined with \ref execution_status 
*/
static int32_t ReadFrame (uint8_t *frame, uint32_t len) {
  status_t stat;
  int32_t rval;

  if ((Emac.flags & EMAC_FLAG_POWER) == 0U) {
    /* Driver not yet powered */
    return ARM_DRIVER_ERROR;
  }
  if ((frame == NULL) && (len != 0U)) {
    return ARM_DRIVER_ERROR_PARAMETER;
  }

  stat = ENET_ReadFrame (ENET, &Emac.h, frame, len, 0U, NULL);

  if (stat == kStatus_Success) {
    rval = (int32_t)len;
  } else {
    rval = ARM_DRIVER_ERROR;
  }

  return (rval);
}

/**
  \fn          uint32_t GetRxFrameSize (void)
  \brief       Get size of received Ethernet frame.
  \return      number of bytes in received frame
*/
static uint32_t GetRxFrameSize (void) {
  status_t stat;
  uint32_t len;

  if ((Emac.flags & EMAC_FLAG_POWER) == 0U) {
    /* Driver not yet powered */
    return (0U);
  }

  do {
    stat = ENET_GetRxFrameSize (&Emac.h, &len, 0U);

    if (stat == kStatus_ENET_RxFrameError) {
      /* Release invalid frame */
      (void)ENET_ReadFrame (ENET, &Emac.h, NULL, 0U, 0U, NULL);
    }
  } while (stat == kStatus_ENET_RxFrameError);

  return (len);
}


/**
  \fn          int32_t GetRxFrameTime (ARM_ETH_MAC_TIME *time)
  \brief       Get time of received Ethernet frame.
  \param[in]   time  Pointer to time structure for data to read into
  \return      \ref execution_status
*/
static int32_t GetRxFrameTime (ARM_ETH_MAC_TIME *time) {
  (void)time;
  return ARM_DRIVER_ERROR_UNSUPPORTED;
}

/**
  \fn          int32_t GetTxFrameTime (ARM_ETH_MAC_TIME *time)
  \brief       Get time of transmitted Ethernet frame.
  \param[in]   time  Pointer to time structure for data to read into
  \return      \ref execution_status
*/
static int32_t GetTxFrameTime (ARM_ETH_MAC_TIME *time) {
  (void)time;
  return ARM_DRIVER_ERROR_UNSUPPORTED;
}

/**
  \fn          int32_t Control (uint32_t control, uint32_t arg)
  \brief       Control Ethernet Interface.
  \param[in]   control  operation
  \param[in]   arg      argument of operation (optional)
  \return      \ref execution_status
*/
static int32_t Control (uint32_t control, uint32_t arg) {
  uint32_t i;

  if ((Emac.flags & EMAC_FLAG_POWER) == 0U) {
    /* Driver not powered */
    return ARM_DRIVER_ERROR;
  }

  switch (control) {
    case ARM_ETH_MAC_CONFIGURE:

      /* Configure link speed */
      switch (arg & ARM_ETH_MAC_SPEED_Msk) {
        case ARM_ETH_MAC_SPEED_10M:
          Emac.cfg.miiSpeed = kENET_MiiSpeed10M;
          break;

        case ARM_ETH_SPEED_100M:
          Emac.cfg.miiSpeed = kENET_MiiSpeed100M;
          break;

        case ARM_ETH_SPEED_1G:
          return ARM_DRIVER_ERROR_UNSUPPORTED;

        default: return ARM_DRIVER_ERROR;
      }

      /* Configure duplex mode */
      switch (arg & ARM_ETH_MAC_DUPLEX_Msk) {
        case ARM_ETH_MAC_DUPLEX_HALF:
          Emac.cfg.miiDuplex = kENET_MiiHalfDuplex;
          break;

        case ARM_ETH_MAC_DUPLEX_FULL:
          Emac.cfg.miiDuplex = kENET_MiiFullDuplex;
          break;

        default: return ARM_DRIVER_ERROR;
      }

      /* Configure MII/RMII mode */
      #if (EMAC_MII_MODE == ARM_ETH_INTERFACE_MII)
      Emac.cfg.miiMode = kENET_MiiMode;
      #else
      Emac.cfg.miiMode = kENET_RmiiMode;
      #endif

      /* Configure loopback mode */
      if (arg & ARM_ETH_MAC_LOOPBACK) {
        Emac.cfg.macSpecialConfig |=  kENET_ControlMIILoopEnable;
      } else {
        Emac.cfg.macSpecialConfig &= ~kENET_ControlMIILoopEnable & 0xFFFF;
      }

      #if (EMAC_CHECKSUM_OFFLOAD)
      /* Enable/Disable rx checksum verification */
      if (arg & ARM_ETH_MAC_CHECKSUM_OFFLOAD_RX) {
        Emac.cfg.rxAccelerConfig = kENET_RxAccelIpCheckEnabled | kENET_RxAccelProtoCheckEnabled | kENET_RxAccelMacCheckEnabled;
      } else {
        Emac.cfg.rxAccelerConfig = 0U;
      }

      /* Enable/Disable tx checksum generation */
      if (arg & ARM_ETH_MAC_CHECKSUM_OFFLOAD_TX) {
        Emac.cfg.txAccelerConfig = kENET_TxAccelIpCheckEnabled | kENET_TxAccelProtoCheckEnabled;
      } else {
        Emac.cfg.txAccelerConfig = 0U;
      }
      #else
      if (arg & (ARM_ETH_MAC_CHECKSUM_OFFLOAD_RX | ARM_ETH_MAC_CHECKSUM_OFFLOAD_TX)) {
        /* Checksum offload is disabled */
        return ARM_DRIVER_ERROR_UNSUPPORTED;
      }
      #endif

      /* Enable/Disable broadcast frame receive */
      if (arg & ARM_ETH_MAC_ADDRESS_BROADCAST) {
        Emac.cfg.macSpecialConfig &= ~kENET_ControlRxBroadCastRejectEnable & 0xFFFF;
      } else {
        Emac.cfg.macSpecialConfig |=  kENET_ControlRxBroadCastRejectEnable;
      }

      /* Enable/Disable multicast frame receive */
       if (arg & ARM_ETH_MAC_ADDRESS_MULTICAST) {
        ENET->GALR = 0xFFFFFFFFU;
        ENET->GAUR = 0xFFFFFFFFU;
      } else {
        ENET->GALR = 0U;
        ENET->GAUR = 0U;
      }

      /* Enable/Disable promiscuous mode (no filtering) */
      if (arg & ARM_ETH_MAC_ADDRESS_ALL) {
        Emac.cfg.macSpecialConfig |=  kENET_ControlPromiscuousEnable;
      } else {
        Emac.cfg.macSpecialConfig &= ~kENET_ControlPromiscuousEnable & 0xFFFF;
      }

      /* Apply configuration */
      ENET_Init (ENET, &Emac.h, &Emac.cfg, &Emac.desc_cfg, Emac.addr, Emac.pclk);

      /* Disable Rx and Tx interrupts */
      ENET_DisableInterrupts (ENET, kENET_RxFrameInterrupt | kENET_TxFrameInterrupt);
      break;

    case ARM_ETH_MAC_CONTROL_TX:
      /* Enable/disable MAC transmitter */
      if (arg != 0U) {
        ENET_EnableInterrupts (ENET, kENET_TxFrameInterrupt);
      } else {
        ENET_DisableInterrupts (ENET, kENET_TxFrameInterrupt);
      }
      break;

    case ARM_ETH_MAC_CONTROL_RX:
      /* Enable/disable MAC receiver */
      if (arg != 0U) {
        ENET_EnableInterrupts (ENET, kENET_RxFrameInterrupt);
      } else {
        ENET_DisableInterrupts (ENET, kENET_RxFrameInterrupt);
      }

      /* Trigger receive process */
      if (arg != 0U) {
        ENET_ActiveRead (ENET);
      }
      break;

    case ARM_ETH_MAC_FLUSH:
      /* Flush tx and rx buffers */
      if (arg == ARM_ETH_MAC_FLUSH_RX) {
        /* Disable RX interrupts */
        ENET_DisableInterrupts (ENET, kENET_RxFrameInterrupt);

        for (i = 0U; i < EMAC_RX_BUF_CNT; i++) {
          Rx_Desc[i].control |= ENET_BUFFDESCRIPTOR_RX_EMPTY_MASK;
        }

        /* Enable RX interrupts */
        ENET_EnableInterrupts (ENET, kENET_RxFrameInterrupt);
      }
      else {
        /* Disable TX interrupts */
        ENET_DisableInterrupts (ENET, kENET_TxFrameInterrupt);

        for (i = 0; i < EMAC_TX_BUF_CNT; i++) {
          Tx_Desc[i].control &= ~ENET_BUFFDESCRIPTOR_TX_READY_MASK;
        }

        /* Enable TX interrupts */
        ENET_EnableInterrupts (ENET, kENET_TxFrameInterrupt);
      }
      break;
    
    case ARM_ETH_MAC_SLEEP:
      if (arg != 0U) {
        ENET_EnableInterrupts (ENET, kENET_WakeupInterrupt);

        /* Enter sleep mode and wait for magic packet */
        ENET_EnableSleepMode (ENET, true);
      }
      else {
        ENET_DisableInterrupts (ENET, kENET_WakeupInterrupt);

        /* Exit sleep mode */
        ENET_EnableSleepMode (ENET, false);
      }
      break;

    case ARM_ETH_MAC_VLAN_FILTER:
      /* VLAN filtering is not supported */
    default:
      return ARM_DRIVER_ERROR_UNSUPPORTED;
  }
  return ARM_DRIVER_OK;
}

/**
  \fn          int32_t ControlTimer (uint32_t control, ARM_ETH_MAC_TIME *time)
  \brief       Control Precision Timer.
  \param[in]   control  operation
  \param[in]   time     Pointer to time structure
  \return      \ref execution_status
*/
static int32_t ControlTimer (uint32_t control, ARM_ETH_MAC_TIME *time) {
  (void)control;
  (void)time;
  return ARM_DRIVER_ERROR_UNSUPPORTED;
}

/**
  \fn          int32_t PHY_Read (uint8_t phy_addr, uint8_t reg_addr, uint16_t *data)
  \brief       Read Ethernet PHY Register through Management Interface.
  \param[in]   phy_addr  5-bit device address
  \param[in]   reg_addr  5-bit register address
  \param[out]  data      Pointer where the result is written to
  \return      \ref execution_status
*/
static int32_t PHY_Read (uint8_t phy_addr, uint8_t reg_addr, uint16_t *data) {
  uint32_t loop;

  if (data == NULL) {
    return ARM_DRIVER_ERROR_PARAMETER;
  }

  if ((Emac.flags & EMAC_FLAG_POWER) == 0U) {
    /* Driver not powered */
    return ARM_DRIVER_ERROR;
  }
  /* Clear MII Interrupt flag */
  ENET_ClearInterruptStatus(ENET, kENET_MiiInterrupt);

  ENET_StartSMIRead (ENET, phy_addr, reg_addr, kENET_MiiReadValidFrame);

  /* Wait until operation completed */
  loop = SystemCoreClock;
  while ((ENET_GetInterruptStatus(ENET) & kENET_MiiInterrupt) == 0U) {
    loop--;
    if (loop == 0) {
      /* Loop counter timeout */
      return ARM_DRIVER_ERROR;
    }
  }

  *data = (uint16_t)ENET_ReadSMIData (ENET);

  return ARM_DRIVER_OK;
}

/**
  \fn          int32_t PHY_Write (uint8_t phy_addr, uint8_t reg_addr, uint16_t data)
  \brief       Write Ethernet PHY Register through Management Interface.
  \param[in]   phy_addr  5-bit device address
  \param[in]   reg_addr  5-bit register address
  \param[in]   data      16-bit data to write
  \return      \ref execution_status
*/
static int32_t PHY_Write (uint8_t phy_addr, uint8_t reg_addr, uint16_t data) {
  uint32_t loop;

  if ((Emac.flags & EMAC_FLAG_POWER) == 0U) {
    /* Driver not powered */
    return ARM_DRIVER_ERROR;
  }
  /* Clear MII Interrupt flag */
  ENET_ClearInterruptStatus(ENET, kENET_MiiInterrupt);

  ENET_StartSMIWrite (ENET, phy_addr, reg_addr, kENET_MiiWriteValidFrame, data);

  /* Wait until operation completed */
  loop = SystemCoreClock;
  while ((ENET_GetInterruptStatus(ENET) & kENET_MiiInterrupt) == 0U) {
    loop--;
    if (loop == 0) {
      /* Loop counter timeout */
      return ARM_DRIVER_ERROR;
    }
  }
  return ARM_DRIVER_OK;
}


static void ENET_IRQCallback (ENET_Type *base, enet_handle_t *handle, enet_event_t event, enet_frame_info_t *frameInfo, void *userData) {
  (void)base;
  (void)handle;
  (void)frameInfo;
  (void)userData;

  if (event == kENET_RxEvent) {
    /* Receive event */
    if (Emac.cb_event) {
      Emac.cb_event (ARM_ETH_MAC_EVENT_RX_FRAME);
    }
  }
  else if (event == kENET_TxEvent) {
    /* Transmit event */
    if (Emac.cb_event) {
      Emac.cb_event (ARM_ETH_MAC_EVENT_TX_FRAME);
    }
  }
  else {
    if (event == kENET_WakeUpEvent) {
      /* Wake up from sleep mode */
      if (Emac.cb_event) {
        Emac.cb_event (ARM_ETH_MAC_EVENT_WAKEUP);
      }
    }
  }
}


/* MAC Driver Control Block */
ARM_DRIVER_ETH_MAC Driver_ETH_MAC0 = {
  GetVersion,
  GetCapabilities,
  Initialize,
  Uninitialize,
  PowerControl,
  GetMacAddress,
  SetMacAddress,
  SetAddressFilter,
  SendFrame,
  ReadFrame,
  GetRxFrameSize,
  GetRxFrameTime,
  GetTxFrameTime,
  ControlTimer,
  Control,
  PHY_Read,
  PHY_Write
};

/*! \endcond */
