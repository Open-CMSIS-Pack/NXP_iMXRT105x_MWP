/*------------------------------------------------------------------------------
 * Example main module
 * Copyright (c) 2019-2022 Arm Limited (or its affiliates). All rights reserved.
 *------------------------------------------------------------------------------
 * Name:    main.c
 * Purpose: Main module
 *-----------------------------------------------------------------------------*/

#include "cmsis_os2.h"                  // ::CMSIS:RTOS2

#include "peripherals.h"                // Keil::Board Support:SDK Project Template:Project_Template
#include "pin_mux.h"                    // Keil::Board Support:SDK Project Template:Project_Template
#include "clock_config.h"               // Keil::Board Support:SDK Project Template:Project_Template
#include "board.h"                      // Keil::Board Support:SDK Project Template:Project_Template


/* Main stack size must be multiple of 8 Bytes */
#define APP_MAIN_STK_SZ (1024U)
uint64_t app_main_stk[APP_MAIN_STK_SZ / 8];
const osThreadAttr_t app_main_attr = {
  .stack_mem  = &app_main_stk[0],
  .stack_size = sizeof(app_main_stk)
};


/*------------------------------------------------------------------------------
 * Application main thread
 *----------------------------------------------------------------------------*/
__NO_RETURN void app_main (void *argument) {

  (void)argument;

  for (;;) {
     // Add application code here
  }
}

/*------------------------------------------------------------------------------
 * main function
 *-----------------------------------------------------------------------------*/
int main(void) {

  // System initialization
  BOARD_ConfigMPU();
  BOARD_InitBootPeripherals();
  BOARD_InitBootPins();
  BOARD_InitBootClocks();
  BOARD_InitDebugConsole();

  // Update System Core Clock info
  SystemCoreClockUpdate();

  //

  osKernelInitialize ();                        // Initialize CMSIS-RTOS2
  osThreadNew (app_main, NULL, &app_main_attr); // Create application main thread
  osKernelStart ();                             // Start thread execution

  for (;;) {}
}
