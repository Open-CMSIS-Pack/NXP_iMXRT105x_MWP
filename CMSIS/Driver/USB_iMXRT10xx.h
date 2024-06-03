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
 * $Revision:   V2.0
 *
 * Project:     USB common (Device and Host) header for NXP i.MX RT 10xx Series
 *
 * -----------------------------------------------------------------------------
 */

#ifndef USB_IMXRT10XX_H_
#define USB_IMXRT10XX_H_

#include <stdint.h>

#include "Driver_USB.h"

#ifdef  __cplusplus
extern  "C"
{
#endif

/**
  \fn          int32_t USB_RoleSet (uint8_t ctrl, uint8_t role)
  \brief       Set USB Controller role.
  \param[in]   ctrl            Index of USB controller
  \param[in]   role            Role (ARM_USB_ROLE_NONE, ARM_USB_ROLE_DEVICE or ARM_USB_ROLE_HOST)
  \return      0 on success, -1 on error.
*/
extern int32_t USB_RoleSet (uint8_t ctrl, uint8_t role);

/**
  \fn          int32_t USB_RoleGet (uint8_t ctrl)
  \brief       Get USB Controller role.
  \param[in]   ctrl            Index of USB controller
  \return      Role (ARM_USB_ROLE_NONE, ARM_USB_ROLE_DEVICE or ARM_USB_ROLE_HOST).
*/
extern uint8_t USB_RoleGet (uint8_t ctrl);

/**
  \fn          void USB_OTG1_IRQHandler (void)
  \brief       USB1 Interrupt Handler (IRQ).
*/
extern void USB_OTG1_IRQHandler (void);

/**
  \fn          void USB_OTG2_IRQHandler (void)
  \brief       USB2 Interrupt Handler (IRQ).
*/
extern void USB_OTG2_IRQHandler (void);

#ifdef  __cplusplus
}
#endif

#endif  // USB_IMXRT10XX_H_
