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
 * Project:     USB Device Driver header for NXP i.MX RT 10xx Series
 *
 * -----------------------------------------------------------------------------
 */

#ifndef USBD_IMXRT10XX_H_
#define USBD_IMXRT10XX_H_

#include "Driver_USBD.h"

#ifdef  __cplusplus
extern  "C"
{
#endif

// Global driver structures ****************************************************

extern ARM_DRIVER_USBD Driver_USBD1;
extern ARM_DRIVER_USBD Driver_USBD2;

// Public functions ************************************************************

/**
  \fn          void USBD1_IRQ_Handler (void)
  \brief       USB1 Device Interrupt Handler (IRQ).
*/
extern void USBD1_IRQ_Handler (void);

/**
  \fn          void USBD2_IRQ_Handler (void)
  \brief       USB2 Device Interrupt Handler (IRQ).
*/
extern void USBD2_IRQ_Handler (void);

#ifdef  __cplusplus
}
#endif

#endif  // USBD_IMXRT10XX_H_
