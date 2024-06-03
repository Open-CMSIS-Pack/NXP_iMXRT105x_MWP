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
 * Project:     USB Host EHCI Controller Driver Hardware-specific header
 *              for NXP i.MX RT 10xx Series
 *
 * -----------------------------------------------------------------------------
 */

#ifndef USBH_EHCI_HW_IMXRT10XX_H_
#define USBH_EHCI_HW_IMXRT10XX_H_

#ifdef  __cplusplus
extern  "C"
{
#endif


/**
  \fn          void USBH1_IRQ_Handler (void)
  \brief       USB1 Host Interrupt Handler (IRQ).
*/
extern void USBH1_IRQ_Handler (void);

/**
  \fn          void USBH2_IRQ_Handler (void)
  \brief       USB2 Host Interrupt Handler (IRQ).
*/
extern void USBH2_IRQ_Handler (void);

#ifdef  __cplusplus
}
#endif

#endif  // USBH_EHCI_HW_IMXRT10XX_H_
