/*
 * Copyright (c) 2024 Arm Limited. All rights reserved.
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
 * -----------------------------------------------------------------------------
 *
 * $Date:       31. May 2024
 * $Revision:   V2.0
 *
 * Project:     USB Device Driver for NXP i.MX RT 10xx Series
 *
 * -----------------------------------------------------------------------------
 */

/* History:
 *  Version 2.0
 *    Reworked and merged USB1 and USB2 Device Drivers into a single module
 *  Version 1.2
 *    Added volatile qualifier to volatile variables
 *  Version 1.1
 *    Corrected Endpoint transfer array size
 *  Version 1.0
 *    Initial release
 */

/*! \page evkb_imxrt1050_usbd CMSIS-Driver for USB Device Interface

<b>CMSIS-Driver for USB Device Interface Setup</b>

The CMSIS-Driver for USB Device Interface enables necessary clocks and uses dedicated
USB pins so no additional configuration in the \ref config_pinclock "MCUXpresso Config Tools" is necessary.

Note:
  If USB buffers and descriptors have to be located in specific memory then variables **qh_buffer** 
  and **s_UsbDeviceEhciDtd** from **usb_device_ehci.c** module have to be positioned in that memory.
  The mentioned USB buffers and descriptors on this microcontroller **must be located in the internal SRAM memory**.
*/

/*! \cond */

#include "USBD_iMXRT10xx.h"

#include <stdint.h>
#include <string.h>

#include "USB_iMXRT10xx.h"

#include "usb_device_config.h"
#include "usb.h"
#include "board.h"
#include "usb_phy.h"
#include "usb_device.h"
#include "usb_device_dci.h"
#include "usb_device_ehci.h"

// Driver Version **************************************************************
static  const ARM_DRIVER_VERSION driver_version = { ARM_DRIVER_VERSION_MAJOR_MINOR(2,3), ARM_DRIVER_VERSION_MAJOR_MINOR(2,0) };
// *****************************************************************************

// Driver Capabilities *********************************************************
static const ARM_USBD_CAPABILITIES driver_capabilities = {
  1U,                           // VBUS detection
  1U,                           // Signal VBUS On event
  1U,                           // Signal VBUS Off event
  0U                            // Reserved (must be zero)
};
// *****************************************************************************

// Compile-time configuration **************************************************

// Check configuration
#if (USB_DEVICE_CONFIG_EHCI != 2U)
#error  USB Device driver requires USB_DEVICE_CONFIG_EHCI value set to 2 in the usb_device_config.h file!
#else
#define DRIVER_CONFIG_VALID     1
#endif

// Maximum number of endpoints can be configured in usb_device_config.h
#define USBD_MAX_ENDPOINT_NUM  (USB_DEVICE_CONFIG_ENDPOINTS)

// *****************************************************************************

#ifdef DRIVER_CONFIG_VALID              // Driver code is available only if configuration is valid

// Macros
// Macro to create section name for RW info
#ifdef  USBD_SECTION_NAME
#define USBDn_SECTION_(name,n)  __attribute__((section(name #n)))
#define USBDn_SECTION(n)        USBDn_SECTION_(USBD_SECTION_NAME,n)
#else
#define USBDn_SECTION(n)
#endif

// Macro for declaring functions (for instances)
#define FUNCS_DECLARE(n)                                                                                               \
static  int32_t                 USBD##n##_Initialize                (ARM_USBD_SignalDeviceEvent_t   cb_device_event,   \
                                                                     ARM_USBD_SignalEndpointEvent_t cb_endpoint_event);\
static  int32_t                 USBD##n##_Uninitialize              (void);                                            \
static  int32_t                 USBD##n##_PowerControl              (ARM_POWER_STATE state);                           \
static  int32_t                 USBD##n##_DeviceConnect             (void);                                            \
static  int32_t                 USBD##n##_DeviceDisconnect          (void);                                            \
static  ARM_USBD_STATE          USBD##n##_DeviceGetState            (void);                                            \
static  int32_t                 USBD##n##_DeviceRemoteWakeup        (void);                                            \
static  int32_t                 USBD##n##_DeviceSetAddress          (uint8_t  dev_addr);                               \
static  int32_t                 USBD##n##_ReadSetupPacket           (uint8_t *setup);                                  \
static  int32_t                 USBD##n##_EndpointConfigure         (uint8_t  ep_addr,                                 \
                                                                     uint8_t  ep_type,                                 \
                                                                     uint16_t ep_max_packet_size);                     \
static  int32_t                 USBD##n##_EndpointUnconfigure       (uint8_t  ep_addr);                                \
static  int32_t                 USBD##n##_EndpointStall             (uint8_t  ep_addr, bool stall);                    \
static  int32_t                 USBD##n##_EndpointTransfer          (uint8_t  ep_addr,                                 \
                                                                     uint8_t *data,                                    \
                                                                     uint32_t num);                                    \
static  uint32_t                USBD##n##_EndpointTransferGetResult (uint8_t  ep_addr);                                \
static  int32_t                 USBD##n##_EndpointTransferAbort     (uint8_t  ep_addr);                                \
static  uint16_t                USBD##n##_GetFrameNumber            (void);

// Macro for defining functions (for instances)
#define FUNCS_DEFINE(n)                                                                                                                                                                                           \
static  int32_t                 USBD##n##_Initialize                (ARM_USBD_SignalDeviceEvent_t   cb_device_event,                                                                                              \
                                                                     ARM_USBD_SignalEndpointEvent_t cb_endpoint_event) { return USBDn_Initialize                (&usbd##n##_ro_info, cb_device_event, cb_endpoint_event); }   \
static  int32_t                 USBD##n##_Uninitialize              (void)                                             { return USBDn_Uninitialize              (&usbd##n##_ro_info); }                           \
static  int32_t                 USBD##n##_PowerControl              (ARM_POWER_STATE state)                            { return USBDn_PowerControl              (&usbd##n##_ro_info, state); }                    \
static  int32_t                 USBD##n##_DeviceConnect             (void)                                             { return USBDn_DeviceConnect             (&usbd##n##_ro_info); }                           \
static  int32_t                 USBD##n##_DeviceDisconnect          (void)                                             { return USBDn_DeviceDisconnect          (&usbd##n##_ro_info); }                           \
static  ARM_USBD_STATE          USBD##n##_DeviceGetState            (void)                                             { return USBDn_DeviceGetState            (&usbd##n##_ro_info); }                           \
static  int32_t                 USBD##n##_DeviceRemoteWakeup        (void)                                             { return USBDn_DeviceRemoteWakeup        (&usbd##n##_ro_info); }                           \
static  int32_t                 USBD##n##_DeviceSetAddress          (uint8_t  dev_addr)                                { return USBDn_DeviceSetAddress          (&usbd##n##_ro_info, dev_addr); }                 \
static  int32_t                 USBD##n##_ReadSetupPacket           (uint8_t *setup)                                   { return USBDn_ReadSetupPacket           (&usbd##n##_ro_info, setup); }                    \
static  int32_t                 USBD##n##_EndpointConfigure         (uint8_t  ep_addr,                                                                                                                            \
                                                                     uint8_t  ep_type,                                                                                                                            \
                                                                     uint16_t ep_max_packet_size)                      { return USBDn_EndpointConfigure         (&usbd##n##_ro_info, ep_addr, ep_type, ep_max_packet_size); } \
static  int32_t                 USBD##n##_EndpointUnconfigure       (uint8_t  ep_addr)                                 { return USBDn_EndpointUnconfigure       (&usbd##n##_ro_info, ep_addr); }                  \
static  int32_t                 USBD##n##_EndpointStall             (uint8_t  ep_addr, bool stall)                     { return USBDn_EndpointStall             (&usbd##n##_ro_info, ep_addr, stall); }           \
static  int32_t                 USBD##n##_EndpointTransfer          (uint8_t  ep_addr,                                                                                                                            \
                                                                     uint8_t *data,                                                                                                                               \
                                                                     uint32_t num)                                     { return USBDn_EndpointTransfer          (&usbd##n##_ro_info, ep_addr, data, num); }       \
static  uint32_t                USBD##n##_EndpointTransferGetResult (uint8_t  ep_addr)                                 { return USBDn_EndpointTransferGetResult (&usbd##n##_ro_info, ep_addr); }                  \
static  int32_t                 USBD##n##_EndpointTransferAbort     (uint8_t  ep_addr)                                 { return USBDn_EndpointTransferAbort     (&usbd##n##_ro_info, ep_addr); }                  \
static  uint16_t                USBD##n##_GetFrameNumber            (void)                                             { return USBDn_GetFrameNumber            (&usbd##n##_ro_info); }

// Macro for defining driver structures (for instances)
#define USBD_DRIVER(n)                  \
ARM_DRIVER_USBD Driver_USBD##n = {      \
  USBD_GetVersion,                      \
  USBD_GetCapabilities,                 \
  USBD##n##_Initialize,                 \
  USBD##n##_Uninitialize,               \
  USBD##n##_PowerControl,               \
  USBD##n##_DeviceConnect,              \
  USBD##n##_DeviceDisconnect,           \
  USBD##n##_DeviceGetState,             \
  USBD##n##_DeviceRemoteWakeup,         \
  USBD##n##_DeviceSetAddress,           \
  USBD##n##_ReadSetupPacket,            \
  USBD##n##_EndpointConfigure,          \
  USBD##n##_EndpointUnconfigure,        \
  USBD##n##_EndpointStall,              \
  USBD##n##_EndpointTransfer,           \
  USBD##n##_EndpointTransferGetResult,  \
  USBD##n##_EndpointTransferAbort,      \
  USBD##n##_GetFrameNumber              \
};

// Endpoint related macros
#define EP_DIR(ep_addr)         (((ep_addr) >> 7) & 1U)
#define EP_NUM(ep_addr)         (ep_addr & ARM_USB_ENDPOINT_NUMBER_MASK)

// Driver status
typedef struct {
  uint8_t                       initialized  : 1;       // Initialized status: 0 - not initialized, 1 - initialized
  uint8_t                       powered      : 1;       // Power status:       0 - not powered,     1 - powered
  uint8_t                       reserved     : 6;       // Reserved (for padding)
} DriverStatus_t;

// Endpoint information
typedef struct {
  volatile uint32_t             num_transferred;        // Number of transferred bytes
} EP_Info_t;

// Instance run-time information (RW)
typedef struct {
  ARM_USBD_SignalDeviceEvent_t  cb_device_event;        // Device event callback
  ARM_USBD_SignalEndpointEvent_t cb_endpoint_event;     // Endpoint event callback
  DriverStatus_t                drv_status;             // Driver status
  volatile ARM_USBD_STATE       usbd_state;             // USB Device state
  usb_device_handle             deviceHandle;           // USB Device Handle
  volatile uint32_t             setup_received;         // Setup Packet received flag (0 - not received or read already, 1 - received and unread yet)
  volatile uint8_t              setup_packet[8];        // Setup Packet data (4-byte aligned)
  EP_Info_t                     ep_info[USBD_MAX_ENDPOINT_NUM][2];      // Endpoint information
} RW_Info_t;

// Instance compile-time information (RO)
// also contains pointer to run-time information
typedef struct {
  RW_Info_t                    *ptr_rw_info;            // Pointer to run-time information (RW)
  uint8_t                       ctrl;                   // USB EHCI Controller instance
  uint8_t                       ctrl_id;                // USB EHCI Controller identifier
  uint8_t                       irqn;                   // USB EHCI Controller interrupt number
  uint8_t                       reserved;               // Reserved (for padding)
} RO_Info_t;

// Information definitions (for instances)
static       RW_Info_t          usbd1_rw_info USBDn_SECTION(1);
static const RO_Info_t          usbd1_ro_info = { &usbd1_rw_info,
                                                   1U,
                                                   kUSB_ControllerEhci0,
                                                   USB_OTG1_IRQn,
                                                   0U
                                                };

static       RW_Info_t          usbd2_rw_info USBDn_SECTION(2);
static const RO_Info_t          usbd2_ro_info = { &usbd2_rw_info,
                                                   2U,
                                                   kUSB_ControllerEhci1,
                                                   USB_OTG2_IRQn,
                                                   0U
                                                };

// Local functions prototypes
static int32_t                  USBDn_ClockConfigure            (uint32_t ctrl_id, uint32_t enable);
static ARM_DRIVER_VERSION       USBD_GetVersion                 (void);
static ARM_USBD_CAPABILITIES    USBD_GetCapabilities            (void);
static int32_t                  USBDn_Initialize                (const RO_Info_t * const ptr_ro_info, ARM_USBD_SignalDeviceEvent_t cb_device_event, ARM_USBD_SignalEndpointEvent_t cb_endpoint_event);
static int32_t                  USBDn_Uninitialize              (const RO_Info_t * const ptr_ro_info);
static int32_t                  USBDn_PowerControl              (const RO_Info_t * const ptr_ro_info, ARM_POWER_STATE state);
static int32_t                  USBDn_DeviceConnect             (const RO_Info_t * const ptr_ro_info);
static int32_t                  USBDn_DeviceDisconnect          (const RO_Info_t * const ptr_ro_info);
static ARM_USBD_STATE           USBDn_DeviceGetState            (const RO_Info_t * const ptr_ro_info);
static int32_t                  USBDn_DeviceRemoteWakeup        (const RO_Info_t * const ptr_ro_info);
static int32_t                  USBDn_DeviceSetAddress          (const RO_Info_t * const ptr_ro_info, uint8_t  dev_addr);
static int32_t                  USBDn_ReadSetupPacket           (const RO_Info_t * const ptr_ro_info, uint8_t *setup);
static int32_t                  USBDn_EndpointConfigure         (const RO_Info_t * const ptr_ro_info, uint8_t  ep_addr, uint8_t  ep_type, uint16_t ep_max_packet_size);
static int32_t                  USBDn_EndpointUnconfigure       (const RO_Info_t * const ptr_ro_info, uint8_t  ep_addr);
static int32_t                  USBDn_EndpointStall             (const RO_Info_t * const ptr_ro_info, uint8_t  ep_addr, bool stall);
static int32_t                  USBDn_EndpointTransfer          (const RO_Info_t * const ptr_ro_info, uint8_t  ep_addr, uint8_t *data, uint32_t num);
static uint32_t                 USBDn_EndpointTransferGetResult (const RO_Info_t * const ptr_ro_info, uint8_t  ep_addr);
static int32_t                  USBDn_EndpointTransferAbort     (const RO_Info_t * const ptr_ro_info, uint8_t  ep_addr);
static uint16_t                 USBDn_GetFrameNumber            (const RO_Info_t * const ptr_ro_info);
static usb_status_t             USB_DeviceCallback              (usb_device_handle handle, uint32_t event, void *param);
static usb_status_t             USB_DeviceEndpointCb            (usb_device_handle handle, usb_device_endpoint_callback_message_struct_t *message, void *callbackParam);

// Local driver functions declarations (for instances)
FUNCS_DECLARE(1)
FUNCS_DECLARE(2)

// Auxiliary functions

/**
  \fn          int32_t USBDn_ClockConfigure (uint32_t ctrl, uint32_t enable)
  \brief       Enable or disable USB peripheral clocks.
  \param[in]   ctrl_id            USB Controller index (1 .. 2)
  \param[in]   enable             Enable/disable clocks (0 = disable, 1 = enable)
  \return      \ref execution_status
*/
static int32_t USBDn_ClockConfigure (uint32_t ctrl, uint32_t enable) {

  if ((ctrl == 0U) || (ctrl > 2U)) {
    return ARM_DRIVER_ERROR_PARAMETER;
  }
  if (enable > 1U) {
    return ARM_DRIVER_ERROR_PARAMETER;
  }

  if (ctrl == 1U) {                     // USB1
    if (enable != 0U) {                 // Enable clocks
      if (CLOCK_EnableUsbhs0PhyPllClock(kCLOCK_Usbphy480M, 480000000U) != true) {
        return ARM_DRIVER_ERROR;
      }
      if (CLOCK_EnableUsbhs0Clock      (kCLOCK_Usb480M,    480000000U) != true) {
        return ARM_DRIVER_ERROR;
      }
    } else {                            // Disable clocks
      CLOCK_DisableUsbhs0PhyPllClock();
    }
  } else {                              // USB2
    if (enable != 0U) {                 // Enable clocks
      if (CLOCK_EnableUsbhs1PhyPllClock(kCLOCK_Usbphy480M, 480000000U) != true) {
        return ARM_DRIVER_ERROR;
      }
      if (CLOCK_EnableUsbhs1Clock      (kCLOCK_Usb480M,    480000000U) != true) {
        return ARM_DRIVER_ERROR;
      }
    } else {                            // Disable clocks
      CLOCK_DisableUsbhs1PhyPllClock();
    }
  }

  return ARM_DRIVER_OK;
}

// Driver functions ************************************************************

/**
  \fn          ARM_DRIVER_VERSION USBD_GetVersion (void)
  \brief       Get driver version.
  \return      \ref ARM_DRIVER_VERSION
*/
static ARM_DRIVER_VERSION USBD_GetVersion (void) {
  return driver_version;
}

/**
  \fn          ARM_USBD_CAPABILITIES USBD_GetCapabilities (void)
  \brief       Get driver capabilities.
  \return      \ref ARM_USBD_CAPABILITIES
*/
static ARM_USBD_CAPABILITIES USBD_GetCapabilities (void) {
  return driver_capabilities;
}

/**
  \fn          int32_t USBDn_Initialize (const RO_Info_t * const        ptr_ro_info,
                                         ARM_USBD_SignalDeviceEvent_t   cb_device_event,
                                         ARM_USBD_SignalEndpointEvent_t cb_endpoint_event)
  \brief       Initialize USB Device Interface.
  \param[in]   ptr_ro_info        Pointer to USBD RO info structure (RO_Info_t)
  \param[in]   cb_device_event    Pointer to \ref ARM_USBD_SignalDeviceEvent
  \param[in]   cb_endpoint_event  Pointer to \ref ARM_USBD_SignalEndpointEvent
  \return      \ref execution_status
*/
static int32_t USBDn_Initialize (const RO_Info_t * const         ptr_ro_info,
                                 ARM_USBD_SignalDeviceEvent_t    cb_device_event,
                                 ARM_USBD_SignalEndpointEvent_t  cb_endpoint_event) {

  // Clear run-time info
  memset((void *)ptr_ro_info->ptr_rw_info, 0, sizeof(RW_Info_t));

  // Register callback functions
  ptr_ro_info->ptr_rw_info->cb_device_event   = cb_device_event;
  ptr_ro_info->ptr_rw_info->cb_endpoint_event = cb_endpoint_event;

  // Set role
  if (USB_RoleSet(ptr_ro_info->ctrl, ARM_USB_ROLE_DEVICE) != 0) {
    return ARM_DRIVER_ERROR;
  }

  // Set driver status to initialized
  ptr_ro_info->ptr_rw_info->drv_status.initialized = 1U;

  return ARM_DRIVER_OK;
}

/**
  \fn          int32_t USBDn_Uninitialize (const RO_Info_t * const ptr_ro_info)
  \brief       De-initialize USB Device Interface.
  \param[in]   ptr_ro_info     Pointer to USBD RO info structure (RO_Info_t)
  \return      \ref execution_status
*/
static int32_t USBDn_Uninitialize (const RO_Info_t * const ptr_ro_info) {

  if (ptr_ro_info->ptr_rw_info->drv_status.powered != 0U) {
    // If peripheral is powered, power off the peripheral
    (void)USBDn_PowerControl(ptr_ro_info, ARM_POWER_OFF);
  }

  // Clear role
  if (USB_RoleSet(ptr_ro_info->ctrl, ARM_USB_ROLE_NONE) != 0) {
    return ARM_DRIVER_ERROR;
  }

  // Clear run-time info
  memset((void *)ptr_ro_info->ptr_rw_info, 0, sizeof(RW_Info_t));

  return ARM_DRIVER_OK;
}

/**
  \fn          int32_t USBDn_PowerControl (const RO_Info_t * const ptr_ro_info, ARM_POWER_STATE state)
  \brief       Control USB Device Interface Power.
  \param[in]   ptr_ro_info     Pointer to USBD RO info structure (RO_Info_t)
  \param[in]   state  Power state
  \return      \ref execution_status
*/
static int32_t USBDn_PowerControl (const RO_Info_t * const ptr_ro_info, ARM_POWER_STATE state) {
  ARM_USBD_SignalDeviceEvent_t   cb_device_event;
  ARM_USBD_SignalEndpointEvent_t cb_endpoint_event;
  DriverStatus_t                 drv_status;
  int32_t                        ret;
  uint8_t                        ep_num;
  uint8_t                        ep_dir;
  usb_phy_config_struct_t        phyConfig = { BOARD_USB_PHY_D_CAL, BOARD_USB_PHY_TXCAL45DP, BOARD_USB_PHY_TXCAL45DM };

  if (ptr_ro_info->ptr_rw_info->drv_status.initialized == 0U) {
    return ARM_DRIVER_ERROR;
  }

  switch (state) {
    case ARM_POWER_FULL:

      // Store variables we need to preserve
      cb_device_event   = ptr_ro_info->ptr_rw_info->cb_device_event;
      cb_endpoint_event = ptr_ro_info->ptr_rw_info->cb_endpoint_event;
      drv_status        = ptr_ro_info->ptr_rw_info->drv_status;

      // Clear run-time info
      memset((void *)ptr_ro_info->ptr_rw_info, 0, sizeof(RW_Info_t));

      // Restore variables we wanted to preserve
      ptr_ro_info->ptr_rw_info->cb_device_event   = cb_device_event;
      ptr_ro_info->ptr_rw_info->cb_endpoint_event = cb_endpoint_event;
      ptr_ro_info->ptr_rw_info->drv_status        = drv_status;

      // Initialize pins, clocks, interrupts and peripheral
      ret = USBDn_ClockConfigure(ptr_ro_info->ctrl, 1U);
      if (ret != ARM_DRIVER_OK) {
        return ret;
      }
      if (USB_EhciPhyInit(ptr_ro_info->ctrl_id, BOARD_XTAL0_CLK_HZ, &phyConfig) != kStatus_USB_Success) {
        return ARM_DRIVER_ERROR;
      }
      if (USB_DeviceInit (ptr_ro_info->ctrl_id, USB_DeviceCallback, &ptr_ro_info->ptr_rw_info->deviceHandle) != kStatus_USB_Success) {
        return ARM_DRIVER_ERROR;
      }
      if (EnableIRQ(ptr_ro_info->irqn) != kStatus_Success) {
        return ARM_DRIVER_ERROR;
      }

      // Set driver status to powered
      ptr_ro_info->ptr_rw_info->drv_status.powered = 1U;
      break;

    case ARM_POWER_OFF:

      // Abort all endpoint transfers
      for (ep_num = 0U; ep_num < USBD_MAX_ENDPOINT_NUM; ep_num++) {
        for (ep_dir = 0U; ep_dir < 2U; ep_dir++) {
          (void)USBDn_EndpointTransferAbort(ptr_ro_info, (uint8_t)(ep_dir << 7) | ep_num);
        }
      }

      // De-initialize pins, clocks, interrupts and peripheral
      if (DisableIRQ(ptr_ro_info->irqn) != kStatus_Success) {
        return ARM_DRIVER_ERROR;
      }
      USB_EhciPhyDeinit(ptr_ro_info->ctrl_id);
      if (USB_DeviceDeinit(ptr_ro_info->ptr_rw_info->deviceHandle)!= kStatus_USB_Success) {
        return ARM_DRIVER_ERROR;
      }
      ret = USBDn_ClockConfigure(ptr_ro_info->ctrl, 0U);
      if (ret != ARM_DRIVER_OK) {
        return ret;
      }
      (void)IRQ_ClearPendingIRQ(ptr_ro_info->irqn);

      // Set driver status to not powered
      ptr_ro_info->ptr_rw_info->drv_status.powered = 0U;

      // Store variables we need to preserve
      cb_device_event   = ptr_ro_info->ptr_rw_info->cb_device_event;
      cb_endpoint_event = ptr_ro_info->ptr_rw_info->cb_endpoint_event;
      drv_status        = ptr_ro_info->ptr_rw_info->drv_status;

      // Clear run-time info
      memset((void *)ptr_ro_info->ptr_rw_info, 0, sizeof(RW_Info_t));

      // Restore variables we wanted to preserve
      ptr_ro_info->ptr_rw_info->cb_device_event   = cb_device_event;
      ptr_ro_info->ptr_rw_info->cb_endpoint_event = cb_endpoint_event;
      ptr_ro_info->ptr_rw_info->drv_status        = drv_status;
      break;

    case ARM_POWER_LOW:
      return ARM_DRIVER_ERROR_UNSUPPORTED;

    default:
      return ARM_DRIVER_ERROR_PARAMETER;
  }

  return ARM_DRIVER_OK;
}

/**
  \fn          int32_t USBDn_DeviceConnect (const RO_Info_t * const ptr_ro_info)
  \brief       Connect USB Device.
  \param[in]   ptr_ro_info     Pointer to USBD RO info structure (RO_Info_t)
  \return      \ref execution_status
*/
static int32_t USBDn_DeviceConnect (const RO_Info_t * const ptr_ro_info) {

  if (ptr_ro_info->ptr_rw_info->drv_status.powered == 0U) {
    return ARM_DRIVER_ERROR;
  }

  if (USB_DeviceRun(ptr_ro_info->ptr_rw_info->deviceHandle) != kStatus_USB_Success) {
    return ARM_DRIVER_ERROR;
  }

  return ARM_DRIVER_OK;
}

/**
  \fn          int32_t USBDn_DeviceDisconnect (const RO_Info_t * const ptr_ro_info)
  \brief       Disconnect USB Device.
  \param[in]   ptr_ro_info     Pointer to USBD RO info structure (RO_Info_t)
  \return      \ref execution_status
*/
static int32_t USBDn_DeviceDisconnect (const RO_Info_t * const ptr_ro_info) {

  if (ptr_ro_info->ptr_rw_info->drv_status.powered == 0U) {
    return ARM_DRIVER_ERROR;
  }

  if (USB_DeviceStop(ptr_ro_info->ptr_rw_info->deviceHandle) != kStatus_USB_Success) {
    return ARM_DRIVER_ERROR;
  }

  return ARM_DRIVER_OK;
}

/**
  \fn          ARM_USBD_STATE USBDn_DeviceGetState (const RO_Info_t * const ptr_ro_info)
  \brief       Get current USB Device State.
  \param[in]   ptr_ro_info     Pointer to USBD RO info structure (RO_Info_t)
  \return      Device State \ref ARM_USBD_STATE
*/
static ARM_USBD_STATE USBDn_DeviceGetState (const RO_Info_t * const ptr_ro_info) {
  return ptr_ro_info->ptr_rw_info->usbd_state;
}

/**
  \fn          int32_t USBDn_DeviceRemoteWakeup (const RO_Info_t * const ptr_ro_info)
  \brief       Trigger USB Remote Wakeup.
  \param[in]   ptr_ro_info     Pointer to USBD RO info structure (RO_Info_t)
  \return      \ref execution_status
*/
static int32_t USBDn_DeviceRemoteWakeup (const RO_Info_t * const ptr_ro_info) {
  uint8_t state;

  if (ptr_ro_info->ptr_rw_info->drv_status.powered == 0U) {
    return ARM_DRIVER_ERROR;
  }

  // Activate remote wakeup
  state = 1U;
  if (USB_DeviceSetStatus(ptr_ro_info->ptr_rw_info->deviceHandle, kUSB_DeviceStatusRemoteWakeup, &state) != kStatus_USB_Success) {
    return ARM_DRIVER_ERROR;
  }

  return ARM_DRIVER_OK;
}

/**
  \fn          int32_t USBDn_DeviceSetAddress (const RO_Info_t * const ptr_ro_info, uint8_t dev_addr)
  \brief       Set USB Device Address.
  \param[in]   ptr_ro_info     Pointer to USBD RO info structure (RO_Info_t)
  \param[in]   dev_addr  Device Address
  \return      \ref execution_status
*/
static int32_t USBDn_DeviceSetAddress (const RO_Info_t * const ptr_ro_info, uint8_t dev_addr) {
  usb_device_ehci_state_struct_t *ehciState = (usb_device_ehci_state_struct_t *)((usb_device_struct_t *)ptr_ro_info->ptr_rw_info->deviceHandle)->controllerHandle;
  uint8_t state;

  if (ptr_ro_info->ptr_rw_info->drv_status.powered == 0U) {
    return ARM_DRIVER_ERROR;
  }

  // Here set address is done by dirrectly writing to DEVICEADDR register because USB_DeviceEhciControl function
  // only allows writing address with kUSB_DeviceControlPreSetDeviceAddress parameter thus sets USBADRA bit also,
  // which is used for keeping address same for next IN transfer however, this function is called when IN was already
  // sent thus USBADRA bit has to be set at 0 when setting the address 
  ehciState->registerBase->DEVICEADDR = dev_addr << USBHS_DEVICEADDR_USBADR_SHIFT;

  // Set internal state to addressed
  state = (uint8_t)kUSB_DeviceStateAddress;
  if (USB_DeviceSetStatus(ptr_ro_info->ptr_rw_info->deviceHandle, kUSB_DeviceStatusDeviceState, &state) != kStatus_USB_Success) {
    return ARM_DRIVER_ERROR;
  }

  return ARM_DRIVER_OK;
}

/**
  \fn          int32_t USBDn_ReadSetupPacket (const RO_Info_t * const ptr_ro_info, uint8_t *setup)
  \brief       Read setup packet received over Control Endpoint.
  \param[in]   ptr_ro_info     Pointer to USBD RO info structure (RO_Info_t)
  \param[out]  setup  Pointer to buffer for setup packet
  \return      \ref execution_status
*/
static int32_t USBDn_ReadSetupPacket (const RO_Info_t * const ptr_ro_info, uint8_t *setup) {

  if (ptr_ro_info->ptr_rw_info->drv_status.powered == 0U) {
    return ARM_DRIVER_ERROR;
  }

  if (ptr_ro_info->ptr_rw_info->setup_received == 0U) {
    return ARM_DRIVER_ERROR;
  }

  do {
    ptr_ro_info->ptr_rw_info->setup_received = 0U;
    for (uint8_t i = 0U; i < 8U; i++) {
      setup[i] = ptr_ro_info->ptr_rw_info->setup_packet[i];
    }
  } while (ptr_ro_info->ptr_rw_info->setup_received != 0U);

  return ARM_DRIVER_OK;
}

/**
  \fn          int32_t USBDn_EndpointConfigure (const RO_Info_t * const ptr_ro_info,
                                                      uint8_t           ep_addr,
                                                      uint8_t           ep_type,
                                                      uint16_t          ep_max_packet_size)
  \brief       Configure USB Endpoint.
  \param[in]   ptr_ro_info     Pointer to USBD RO info structure (RO_Info_t)
  \param[in]   ep_addr  Endpoint Address
                - ep_addr.0..3: Address
                - ep_addr.7:    Direction
  \param[in]   ep_type  Endpoint Type (ARM_USB_ENDPOINT_xxx)
  \param[in]   ep_max_packet_size Endpoint Maximum Packet Size
  \return      \ref execution_status
*/
static int32_t USBDn_EndpointConfigure (const RO_Info_t * const ptr_ro_info,
                                              uint8_t           ep_addr,
                                              uint8_t           ep_type,
                                              uint16_t          ep_max_packet_size) {

  EP_Info_t                            *ptr_ep;
  uint8_t                               ep_num;
  uint8_t                               ep_dir;
  usb_device_endpoint_init_struct_t     epInit;
  usb_device_endpoint_callback_struct_t epCallback;

  if (ptr_ro_info->ptr_rw_info->drv_status.powered == 0U) {
    return ARM_DRIVER_ERROR;
  }

  ep_num = EP_NUM(ep_addr);

  if (ep_num >= USBD_MAX_ENDPOINT_NUM) {
    return ARM_DRIVER_ERROR_PARAMETER;
  }

  ep_dir = EP_DIR(ep_addr);
  ptr_ep = &ptr_ro_info->ptr_rw_info->ep_info[ep_num][ep_dir];

  // Clear Endpoint information
  memset((void *)ptr_ep, 0, sizeof(EP_Info_t));

  epInit.maxPacketSize     = ep_max_packet_size;
  epInit.endpointAddress   = ep_addr;
  epInit.transferType      = ep_type;
  epInit.zlt               = 0;

  epCallback.callbackFn    = USB_DeviceEndpointCb;
  epCallback.callbackParam = (void *)((uint32_t)ep_addr);

  if (USB_DeviceInitEndpoint(ptr_ro_info->ptr_rw_info->deviceHandle, &epInit, &epCallback) != kStatus_USB_Success) {
    return ARM_DRIVER_ERROR;
  }

  return ARM_DRIVER_OK;
}

/**
  \fn          int32_t USBDn_EndpointUnconfigure (const RO_Info_t * const ptr_ro_info, uint8_t ep_addr)
  \brief       Unconfigure USB Endpoint.
  \param[in]   ptr_ro_info     Pointer to USBD RO info structure (RO_Info_t)
  \param[in]   ep_addr  Endpoint Address
                - ep_addr.0..3: Address
                - ep_addr.7:    Direction
  \return      \ref execution_status
*/
static int32_t USBDn_EndpointUnconfigure (const RO_Info_t * const ptr_ro_info, uint8_t ep_addr) {
  EP_Info_t *ptr_ep;
  uint8_t    ep_num;
  uint8_t    ep_dir;

  if (ptr_ro_info->ptr_rw_info->drv_status.powered == 0U) {
    return ARM_DRIVER_ERROR;
  }

  ep_num = EP_NUM(ep_addr);

  if (ep_num >= USBD_MAX_ENDPOINT_NUM) {
    return ARM_DRIVER_ERROR_PARAMETER;
  }

  ep_dir = EP_DIR(ep_addr);
  ptr_ep = &ptr_ro_info->ptr_rw_info->ep_info[ep_num][ep_dir];

  if (USB_DeviceDeinitEndpoint(ptr_ro_info->ptr_rw_info->deviceHandle, ep_addr) != kStatus_USB_Success) {
    return ARM_DRIVER_ERROR;
  }

  // Clear Endpoint information
  memset((void *)ptr_ep, 0, sizeof(EP_Info_t));

  return ARM_DRIVER_OK;
}

/**
  \fn          int32_t USBDn_EndpointStall (const RO_Info_t * const ptr_ro_info, uint8_t ep_addr, bool stall)
  \brief       Set/Clear Stall for USB Endpoint.
  \param[in]   ptr_ro_info     Pointer to USBD RO info structure (RO_Info_t)
  \param[in]   ep_addr  Endpoint Address
                - ep_addr.0..3: Address
                - ep_addr.7:    Direction
  \param[in]   stall  Operation
                - \b false Clear
                - \b true Set
  \return      \ref execution_status
*/
static int32_t USBDn_EndpointStall (const RO_Info_t * const ptr_ro_info, uint8_t ep_addr, bool stall) {

  if (ptr_ro_info->ptr_rw_info->drv_status.powered == 0U) {
    return ARM_DRIVER_ERROR;
  }

  if (stall != 0U) {                    // If request to set STALL
    if (USB_DeviceStallEndpoint(ptr_ro_info->ptr_rw_info->deviceHandle, ep_addr) != kStatus_USB_Success) {
      return ARM_DRIVER_ERROR;
    }
  } else {                              // If request to clear STALL
    if (USB_DeviceUnstallEndpoint(ptr_ro_info->ptr_rw_info->deviceHandle, ep_addr) != kStatus_USB_Success) {
      return ARM_DRIVER_ERROR;
    }
    if (USB_DeviceCancel(ptr_ro_info->ptr_rw_info->deviceHandle, ep_addr) != kStatus_USB_Success) {
      return ARM_DRIVER_ERROR;
    }
  }

  return ARM_DRIVER_OK;
}

/**
  \fn          int32_t USBDn_EndpointTransfer (const RO_Info_t * const ptr_ro_info, uint8_t ep_addr, uint8_t *data, uint32_t num)
  \brief       Read data from or Write data to USB Endpoint.
  \param[in]   ptr_ro_info     Pointer to USBD RO info structure (RO_Info_t)
  \param[in]   ep_addr  Endpoint Address
                - ep_addr.0..3: Address
                - ep_addr.7:    Direction
  \param[out]  data Pointer to buffer for data to read or with data to write
  \param[in]   num  Number of data bytes to transfer
  \return      \ref execution_status
*/
static int32_t USBDn_EndpointTransfer (const RO_Info_t * const ptr_ro_info, uint8_t ep_addr, uint8_t *data, uint32_t num) {
  EP_Info_t *ptr_ep;
  uint8_t    ep_num;
  uint8_t    ep_dir;

  if (ptr_ro_info->ptr_rw_info->drv_status.powered == 0U) {
    return ARM_DRIVER_ERROR;
  }

  ep_num = EP_NUM(ep_addr);

  if (ep_num >= USBD_MAX_ENDPOINT_NUM) {
    return ARM_DRIVER_ERROR_PARAMETER;
  }

  ep_dir = EP_DIR(ep_addr);
  ptr_ep = &ptr_ro_info->ptr_rw_info->ep_info[ep_num][ep_dir];

  // Clear number of transferred bytes
  ptr_ep->num_transferred = 0U;

  if (ep_dir != 0U) {                   // If IN Endpoint
    (void)USB_DeviceSendRequest(ptr_ro_info->ptr_rw_info->deviceHandle, ep_addr, data, num);
  } else {                              // If OUT Endpoint
    (void)USB_DeviceRecvRequest(ptr_ro_info->ptr_rw_info->deviceHandle, ep_addr, data, num);
  }

  return ARM_DRIVER_OK;
}

/**
  \fn          uint32_t USBDn_EndpointTransferGetResult (const RO_Info_t * const ptr_ro_info, uint8_t ep_addr)
  \brief       Get result of USB Endpoint transfer.
  \param[in]   ptr_ro_info     Pointer to USBD RO info structure (RO_Info_t)
  \param[in]   ep_addr  Endpoint Address
                - ep_addr.0..3: Address
                - ep_addr.7:    Direction
  \return      number of successfully transferred data bytes
*/
static uint32_t USBDn_EndpointTransferGetResult (const RO_Info_t * const ptr_ro_info, uint8_t ep_addr) {
  uint8_t ep_num;

  if (ptr_ro_info->ptr_rw_info->drv_status.powered == 0U) {
    return 0U;
  }

  ep_num = EP_NUM(ep_addr);

  if (ep_num >= USBD_MAX_ENDPOINT_NUM) {
    return 0U;
  }

  return ptr_ro_info->ptr_rw_info->ep_info[ep_num][EP_DIR(ep_addr)].num_transferred;
}

/**
  \fn          int32_t USBDn_EndpointTransferAbort (const RO_Info_t * const ptr_ro_info, uint8_t ep_addr)
  \brief       Abort current USB Endpoint transfer.
  \param[in]   ptr_ro_info     Pointer to USBD RO info structure (RO_Info_t)
  \param[in]   ep_addr  Endpoint Address
                - ep_addr.0..3: Address
                - ep_addr.7:    Direction
  \return      \ref execution_status
*/
static int32_t USBDn_EndpointTransferAbort (const RO_Info_t * const ptr_ro_info, uint8_t ep_addr) {
  uint8_t ep_num;

  if (ptr_ro_info->ptr_rw_info->drv_status.powered == 0U) {
    return ARM_DRIVER_ERROR;
  }

  ep_num = EP_NUM(ep_addr);

  if (ep_num >= USBD_MAX_ENDPOINT_NUM) {
    return ARM_DRIVER_ERROR_PARAMETER;
  }

  if (USB_DeviceCancel(ptr_ro_info->ptr_rw_info->deviceHandle, ep_addr) != kStatus_USB_Success) {
    return ARM_DRIVER_ERROR;
  }


  return ARM_DRIVER_OK;
}

/**
  \fn          uint16_t USBDn_GetFrameNumber (const RO_Info_t * const ptr_ro_info)
  \brief       Get current USB Frame Number.
  \param[in]   ptr_ro_info     Pointer to USBD RO info structure (RO_Info_t)
  \return      Frame Number
*/
static uint16_t USBDn_GetFrameNumber (const RO_Info_t * const ptr_ro_info) {
  uint16_t frame = 0U;

  if (ptr_ro_info->ptr_rw_info->drv_status.powered == 0U) {
    return 0U;
  }

  (void)USB_DeviceGetStatus(ptr_ro_info->ptr_rw_info->deviceHandle, kUSB_DeviceStatusSynchFrame, (void *)(&frame));

  return frame;
}

// HAL callback functions ******************************************************

/**
  \fn          usb_status_t USB_DeviceCallback(usb_device_handle handle,
                                               uint32_t          event,
                                               void             *param)
  \brief       Device callback
  \return      usb_status_t
*/
static usb_status_t USB_DeviceCallback(usb_device_handle handle, uint32_t event, void *param) {
  RW_Info_t *ptr_rw_info;
  EP_Info_t *ptr_ep;
  uint8_t    speed;
  uint8_t    ep_addr;

  (void)param;

  if (handle == usbd1_rw_info.deviceHandle) {
    ptr_rw_info = &usbd1_rw_info;
  } else if (handle == usbd2_rw_info.deviceHandle) {
    ptr_rw_info = &usbd2_rw_info;
  } else {
    return kStatus_USB_Error;
  }

  switch (event) {
    case kUSB_DeviceEventBusReset:
      for (uint8_t i = 0U; i < USBD_MAX_ENDPOINT_NUM; i++) {
        ep_addr = i;
        ptr_ep = &ptr_rw_info->ep_info[EP_NUM(ep_addr)][EP_DIR(ep_addr)];
        USB_DeviceUnstallEndpoint(ptr_rw_info->deviceHandle, ep_addr);
        ptr_ep->num_transferred = 0U;
        (void)USB_DeviceCancel(ptr_rw_info->deviceHandle, ep_addr);

        ep_addr = ARM_USB_ENDPOINT_DIRECTION_MASK | i;
        ptr_ep = &ptr_rw_info->ep_info[EP_NUM(ep_addr)][EP_DIR(ep_addr)];
        USB_DeviceUnstallEndpoint(ptr_rw_info->deviceHandle, ep_addr);
        ptr_ep->num_transferred = 0U;
        (void)USB_DeviceCancel(ptr_rw_info->deviceHandle, ep_addr);
      }

      ptr_rw_info->usbd_state.active = 1U;
      ptr_rw_info->usbd_state.vbus   = 1U;
      USB_DeviceGetStatus(ptr_rw_info->deviceHandle, kUSB_DeviceStatusSpeed, &speed);
      if (ptr_rw_info->cb_device_event != NULL) { ptr_rw_info->cb_device_event(ARM_USBD_EVENT_RESET); }
      if (speed == USB_SPEED_FULL) {
        ptr_rw_info->usbd_state.speed = ARM_USB_SPEED_FULL;
      } else if (speed == USB_SPEED_HIGH) {
        ptr_rw_info->usbd_state.speed = ARM_USB_SPEED_HIGH;
        if (ptr_rw_info->cb_device_event != NULL) { ptr_rw_info->cb_device_event(ARM_USBD_EVENT_HIGH_SPEED); }
      }
      break;
    case kUSB_DeviceEventSuspend:
      ptr_rw_info->usbd_state.active = 0U;
      if (ptr_rw_info->cb_device_event != NULL) { ptr_rw_info->cb_device_event(ARM_USBD_EVENT_SUSPEND); }
      break;
    case kUSB_DeviceEventResume:
      ptr_rw_info->usbd_state.active = 1U;
      if (ptr_rw_info->cb_device_event != NULL) { ptr_rw_info->cb_device_event(ARM_USBD_EVENT_RESUME); }
      break;
    case kUSB_DeviceEventLPMResume:
      ptr_rw_info->usbd_state.active = 1U;
      if (ptr_rw_info->cb_device_event != NULL) { ptr_rw_info->cb_device_event(ARM_USBD_EVENT_RESUME); }
      break;
    case kUSB_DeviceEventDetach:
      ptr_rw_info->usbd_state.active = 0U;
      ptr_rw_info->usbd_state.vbus   = 0U;
      if (ptr_rw_info->cb_device_event != NULL) { ptr_rw_info->cb_device_event(ARM_USBD_EVENT_VBUS_OFF); }
      break;
    case kUSB_DeviceEventAttach:
      ptr_rw_info->usbd_state.vbus   = 1U;
      if (ptr_rw_info->cb_device_event != NULL) { ptr_rw_info->cb_device_event(ARM_USBD_EVENT_VBUS_ON); }
      break;
  }

  return kStatus_USB_Success;
}

/**
  \fn          usb_status_t USB_DeviceEndpointCb(usb_device_handle                              handle,
                                                 usb_device_endpoint_callback_message_struct_t *message,
                                                 void                                          *callbackParam)
  \brief       Endpoint callback
  \return      usb_status_t
*/
static usb_status_t USB_DeviceEndpointCb(usb_device_handle handle,
                                         usb_device_endpoint_callback_message_struct_t *message,
                                         void *callbackParam) {
  RW_Info_t *ptr_rw_info;
  EP_Info_t *ptr_ep;
  uint8_t    ep_addr = (uint8_t)((uint32_t)callbackParam & 0xFFU);

  if (handle == usbd1_rw_info.deviceHandle) {
    ptr_rw_info = &usbd1_rw_info;
  } else if (handle == usbd2_rw_info.deviceHandle) {
    ptr_rw_info = &usbd2_rw_info;
  } else {
    return kStatus_USB_Error;
  }

  ptr_ep = &ptr_rw_info->ep_info[EP_NUM(ep_addr)][EP_DIR(ep_addr)];

  if ((ep_addr == 0x00U) && (message->isSetup == 1U)) {
    // Store received setup packet data
    for (uint8_t i = 0U; i < 8U; i++) {
      ptr_rw_info->setup_packet[i] = message->buffer[i];
    }
    ptr_rw_info->setup_received = 1U;
    ptr_rw_info->cb_endpoint_event(0U, ARM_USBD_EVENT_SETUP);
  } else {
    ptr_ep->num_transferred = message->length;

    if (EP_DIR(ep_addr) != 0U) {        // If IN Endpoint
      ptr_rw_info->cb_endpoint_event(ep_addr, ARM_USBD_EVENT_IN);
    } else {                            // If OUT Endpoint
      ptr_rw_info->cb_endpoint_event(ep_addr, ARM_USBD_EVENT_OUT);
    }
  }

  return kStatus_USB_Success;
}

// Local driver functions definitions (for instances)
FUNCS_DEFINE(1)
FUNCS_DEFINE(2)

// Global driver structures ****************************************************

USBD_DRIVER(1)
USBD_DRIVER(2)

// Public functions ************************************************************

/**
  \fn          void USBD1_IRQ_Handler (void)
  \brief       USB1 Device Interrupt Handler (IRQ).
*/
void USBD1_IRQ_Handler (void) {
  USB_DeviceEhciIsrFunction(usbd1_rw_info.deviceHandle);
}

/**
  \fn          void USBD2_IRQ_Handler (void)
  \brief       USB2 Device Interrupt Handler (IRQ).
*/
void USBD2_IRQ_Handler (void) {
  USB_DeviceEhciIsrFunction(usbd2_rw_info.deviceHandle);
}

#endif  // DRIVER_CONFIG_VALID

/*! \endcond */
