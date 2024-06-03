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
 * $Revision:   V1.0
 *
 * Project:     USB Host EHCI Controller Driver Hardware-specific module
 *              for NXP i.MX RT 10xx Series
 *
 * -----------------------------------------------------------------------------
 */

/* History:
 *  Version 1.0
 *    Initial release
 */

/*! \page evkb_imxrt1050_usbh CMSIS-Driver for USB Host Interface

<b>CMSIS-Driver for USB Host Interface Setup</b>

The CMSIS-Driver for USB Host Interface enables necessary clocks and uses dedicated
USB pins so no additional configuration in the \ref config_pinclock "MCUXpresso Config Tools" is necessary.

The CMSIS-Driver for USB Host Interface uses CMSIS-Driver for **EHCI** which is provided in the **Keil::CMSIS-Driver** pack.

The procedure of configuring EHCI driver for usage:
-# Enable the component **Keil::CMSIS-Driver:USB Host:EHCI** variant **TT**
-# Edit the **USBH_EHCI_Config.h** file and set define values as follows:
   - **USBH0_EHCI_DRV_NUM**   = **1**
   - **USBH0_EHCI_BASE_ADDR** = **0x402E0100**
   - **USBH1_EHCI_ENABLED**   = **1**
   - **USBH1_EHCI_DRV_NUM**   = **2**
   - **USBH1_EHCI_BASE_ADDR** = **0x402E0300**
   - **USBH_EHCI_MAX_PIPES**  = **4**

Note:
  If USB Communication Area has to be located in specific memory then enable **Relocate EHCI Communication Area**
  by setting **USBHn_EHCI_COM_AREA_RELOC** define value to **1** and use name specified as value 
  of **USBHn_EHCI_COM_AREA_SECTION_NAME** define in your linker script.
  The **Communication Area** on this microcontroller **must be located in the internal SRAM memory**.

  The **USBH_EHCI_HW_iMXRT10xx.c** file provides hardware-specific interface to EHCI driver.
*/

/*! \cond */

#include "USBH_EHCI_HW_iMXRT10xx.h"

#include "RTE_Components.h"

#ifdef   RTE_Drivers_USBH_EHCI
#include "USBH_EHCI_HW.h"
#endif

#include "USB_iMXRT10xx.h"

#include "fsl_device_registers.h"
#include "fsl_clock.h"

// Local variables
static USBH_EHCI_Interrupt_t EHCI_IRQ_Handler[2];

// Public functions ************************************************************

/**
  \fn          int32_t USBH_EHCI_HW_Initialize (uint8_t ctrl, USBH_EHCI_Interrupt_t interrupt_handler)
  \brief       Initialize USB Host EHCI Interface.
  \param[in]   ctrl               Index of USB Host controller (1 .. 2)
  \param[in]   interrupt_handler  Pointer to Interrupt Handler Routine
  \return      0 on success, -1 on error.
*/
int32_t USBH_EHCI_HW_Initialize (uint8_t ctrl, USBH_EHCI_Interrupt_t interrupt_handler) {

  if ((ctrl == 0U) || (ctrl > 2U)) {
    return -1;
  }

  EHCI_IRQ_Handler[ctrl - 1U] = interrupt_handler;

  return USB_RoleSet(ctrl, ARM_USB_ROLE_HOST);
}

/**
  \fn          int32_t USBH_EHCI_HW_Uninitialize (uint8_t ctrl)
  \brief       De-initialize USB Host EHCI Interface.
  \param[in]   ctrl               Index of USB Host controller (1 .. 2)
  \return      0 on success, -1 on error.
*/
int32_t USBH_EHCI_HW_Uninitialize (uint8_t ctrl) {

  return USB_RoleSet(ctrl, ARM_USB_ROLE_NONE);
}

/**
  \fn          int32_t USBH_EHCI_HW_PowerControl (uint8_t ctrl, uint32_t state)
  \brief       Control USB Host EHCI Interface Power.
  \param[in]   ctrl               Index of USB Host controller (1 .. 2)
  \param[in]   state              Power state (0 = power off, 1 = power on)
  \return      0 on success, -1 on error.
*/
int32_t USBH_EHCI_HW_PowerControl (uint8_t ctrl, uint32_t state) {

  if ((ctrl == 0U) || (ctrl > 2U)) {
    return -1;
  }
  if (state > 1U) {
    return -1;
  }

  if (state == 0U) {                                    // If power off requested
    if (ctrl == 1U) {                                   // USB1
      NVIC_DisableIRQ     (USB_OTG1_IRQn);              // Disable interrupt
      NVIC_ClearPendingIRQ(USB_OTG1_IRQn);              // Clear pending interrupt
      CLOCK_DisableUsbhs0PhyPllClock();
    } else {                                            // USB2
      NVIC_DisableIRQ     (USB_OTG2_IRQn);              // Disable interrupt
      NVIC_ClearPendingIRQ(USB_OTG2_IRQn);              // Clear pending interrupt
      CLOCK_DisableUsbhs1PhyPllClock();
    }
  } else {                                              // If power on requested
    if (ctrl == 1U) {                                   // USB1
      if (CLOCK_EnableUsbhs0PhyPllClock(kCLOCK_Usbphy480M, 480000000U) == false) {
        return -1;
      }
      NVIC_EnableIRQ(USB_OTG1_IRQn);                    // Enable interrupt
    } else {                                            // USB2
      if (CLOCK_EnableUsbhs1PhyPllClock(kCLOCK_Usbphy480M, 480000000U) == false) {
        return -1;
      }
      NVIC_EnableIRQ(USB_OTG2_IRQn);                    // Enable interrupt
    }
  }

  return 0;
}

/**
  \fn          void USBH1_IRQ_Handler (void)
  \brief       USB1 Host Interrupt Handler (IRQ).
*/
void USBH1_IRQ_Handler (void) {
  uint32_t usbsts, portsc1;

  // Special handling
  usbsts = USB1->USBSTS;
  if ((usbsts & USB_USBSTS_PCI_MASK) != 0U) {           // If Port Change Detect is active
    portsc1 = USB1->PORTSC1;
    if ((portsc1 & USB_PORTSC1_SUSP_MASK)!= 0U) {       // If Suspend is active disable HS disconnect detector
      USBPHY1->CTRL &= ~USBPHY_CTRL_ENHOSTDISCONDETECT_MASK;
    } else if ( ((portsc1 & USB_PORTSC1_CCS_MASK) != 0U)                             &&
               (((portsc1 & USB_PORTSC1_PSPD_MASK) >> USB_PORTSC1_PSPD_SHIFT) == 2U) &&
               ( (portsc1 & USB_PORTSC1_FPR_MASK)                             == 0U)) {
      // If Current Connect Status is active and HS Device Connected and no forced suspend, 
      // enable HS disconnect detector
      USBPHY1->CTRL |=  USBPHY_CTRL_ENHOSTDISCONDETECT_MASK;
    } else {                    // else disable HS disconnect detector
      USBPHY1->CTRL &= ~USBPHY_CTRL_ENHOSTDISCONDETECT_MASK;
    }
  }

  EHCI_IRQ_Handler[0]();                                // Call EHCI IRQ Handler
}

/**
  \fn          void USBH2_IRQ_Handler (void)
  \brief       USB2 Host Interrupt Handler (IRQ).
*/
void USBH2_IRQ_Handler (void) {
  uint32_t usbsts, portsc1;

  // Special handling
  usbsts = USB2->USBSTS;
  if ((usbsts & USB_USBSTS_PCI_MASK) != 0U) {           // If Port Change Detect is active
    portsc1 = USB2->PORTSC1;
    if ((portsc1 & USB_PORTSC1_SUSP_MASK)!= 0U) {       // If Suspend is active disable HS disconnect detector
      USBPHY2->CTRL &= ~USBPHY_CTRL_ENHOSTDISCONDETECT_MASK;
    } else if ( ((portsc1 & USB_PORTSC1_CCS_MASK) != 0U)                             &&
               (((portsc1 & USB_PORTSC1_PSPD_MASK) >> USB_PORTSC1_PSPD_SHIFT) == 2U) &&
               ( (portsc1 & USB_PORTSC1_FPR_MASK)                             == 0U)) {
      // If Current Connect Status is active and HS Device Connected and no forced suspend, 
      // enable HS disconnect detector
      USBPHY2->CTRL |=  USBPHY_CTRL_ENHOSTDISCONDETECT_MASK;
    } else {                    // else disable HS disconnect detector
      USBPHY2->CTRL &= ~USBPHY_CTRL_ENHOSTDISCONDETECT_MASK;
    }
  }

  EHCI_IRQ_Handler[1]();                                // Call EHCI IRQ Handler
}

/*! \endcond */
