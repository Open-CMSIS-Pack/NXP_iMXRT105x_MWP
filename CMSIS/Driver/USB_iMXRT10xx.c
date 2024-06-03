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
 * $Date:       30. May 2024
 * $Revision:   V1.0
 *
 * Project:     USB common (Device and Host) module for NXP i.MX RT 10xx Series
 *
 * -----------------------------------------------------------------------------
 */

/* History:
 *  Version 1.0
 *    Initial release
 */

#include "USB_iMXRT10xx.h"

#include "cmsis_compiler.h"

#include "USBD_iMXRT10xx.h"
#include "USBH_EHCI_HW_iMXRT10xx.h"

// Local weak functions (these get replaced by handlers from USBH/USBD modules respectively)

__WEAK void USBH1_IRQ_Handler (void) {}
__WEAK void USBD1_IRQ_Handler (void) {}

__WEAK void USBH2_IRQ_Handler (void) {}
__WEAK void USBD2_IRQ_Handler (void) {}

// Local variables

static volatile uint8_t usb_role[2] = { ARM_USB_ROLE_NONE, ARM_USB_ROLE_NONE };

// Public functions ************************************************************

/**
  \fn          int32_t USB_RoleSet (uint8_t ctrl, uint8_t role)
  \brief       Set USB Controller role.
  \param[in]   ctrl            Index of USB controller (1 .. 2)
  \param[in]   role            Role (ARM_USB_ROLE_NONE, ARM_USB_ROLE_DEVICE or ARM_USB_ROLE_HOST)
  \return      0 on success, -1 on error.
*/
int32_t USB_RoleSet (uint8_t ctrl, uint8_t role) {
  uint8_t ctrl_ofs = ctrl - 1U;

  if (ctrl_ofs > 1U) {
    return -1;
  }

  usb_role[ctrl_ofs] = role;

  return 0;
}

/**
  \fn          int32_t USB_RoleGet (uint8_t ctrl)
  \brief       Get USB Controller role.
  \param[in]   ctrl            Index of USB controller (1 .. 2)
  \return      Role (ARM_USB_ROLE_NONE, ARM_USB_ROLE_DEVICE or ARM_USB_ROLE_HOST).
*/
uint8_t USB_RoleGet (uint8_t ctrl) {
  uint8_t ctrl_ofs = ctrl - 1U;

  if (ctrl_ofs > 1U) {
    return -1;
  }

  return usb_role[ctrl_ofs];
}

/**
  \fn          void USB_OTG1_IRQHandler (void)
  \brief       USB1 Interrupt Handler (IRQ).
*/
void USB_OTG1_IRQHandler (void) {
  switch (usb_role[0]) {
    case ARM_USB_ROLE_HOST:
      USBH1_IRQ_Handler();
      break;

    case ARM_USB_ROLE_DEVICE:
      USBD1_IRQ_Handler();
      break;

    default:
      break;
  }
}

/**
  \fn          void USB_OTG2_IRQHandler (void)
  \brief       USB2 Interrupt Handler (IRQ).
*/
void USB_OTG2_IRQHandler (void) {
  switch (usb_role[1]) {
    case ARM_USB_ROLE_HOST:
      USBH2_IRQ_Handler();
      break;

    case ARM_USB_ROLE_DEVICE:
      USBD2_IRQ_Handler();
      break;

    default:
      break;
  }
}
