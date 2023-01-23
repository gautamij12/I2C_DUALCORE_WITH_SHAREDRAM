/********************************************************************************
* File Name:   main.c
*
* Description: This is the source code for CM0+ in the the Dual CPU IPC Semaphore 
*              Application for ModusToolbox.
*
* Related Document: See README.md
*
*
*********************************************************************************
* Copyright 2020-2021, Cypress Semiconductor Corporation (an Infineon company) or
* an affiliate of Cypress Semiconductor Corporation.  All rights reserved.
*
* This software, including source code, documentation and related
* materials ("Software") is owned by Cypress Semiconductor Corporation
* or one of its affiliates ("Cypress") and is protected by and subject to
* worldwide patent protection (United States and foreign),
* United States copyright laws and international treaty provisions.
* Therefore, you may use this Software only as provided in the license
* agreement accompanying the software package from which you
* obtained this Software ("EULA").
* If no EULA applies, Cypress hereby grants you a personal, non-exclusive,
* non-transferable license to copy, modify, and compile the Software
* source code solely for use in connection with Cypress's
* integrated circuit products.  Any reproduction, modification, translation,
* compilation, or representation of this Software except as specified
* above is prohibited without the express written permission of Cypress.
*
* Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. Cypress
* reserves the right to make changes to the Software without notice. Cypress
* does not assume any liability arising out of the application or use of the
* Software or any product or circuit described in the Software. Cypress does
* not authorize its products for use in any products where a malfunction or
* failure of the Cypress product may reasonably be expected to result in
* significant property damage, injury or death ("High Risk Product"). By
* including Cypress's product in a High Risk Product, the manufacturer
* of such system or application assumes all risk of such use and in doing
* so agrees to indemnify Cypress against all liability.
*********************************************************************************/

#include "cy_pdl.h"
#include "ipc_def.h"
#include <stdlib.h> 
#include "cycfg.h"
#include "cybsp.h"

#define I2C_USE

#ifdef I2C_USE
///* Valid command packet size of three bytes */
#define PACKET_SIZE             (0xFFu)
//
///* Master write and read buffer of size three bytes */
#define SL_RD_BUFFER_SIZE       (PACKET_SIZE)
#define SL_WR_BUFFER_SIZE       (PACKET_SIZE)


uint8_t i2c_read_buffer [SL_RD_BUFFER_SIZE];
uint8_t i2c_write_buffer[SL_WR_BUFFER_SIZE];
cy_stc_scb_i2c_context_t i2cContext;

void HandleEventsSlave(uint32 event);

void I2C_Isr(void)
{
	Cy_SCB_I2C_SlaveInterrupt(SCB3, &i2cContext);
}

void HandleEventsSlave(uint32_t event)
{
	int i;
    //Cy_SCB_UART_PutString(CYBSP_UART_HW, "lodsifjoaikjewfoiejfoaij I2C interrupt\r\n");
	if(CY_SCB_I2C_SLAVE_WRITE_EVENT & event)
	{
		//SCB_I2C_CTRL(sI2C_HW)|=(SCB_I2C_CTRL_S_NOT_READY_ADDR_NACK_Msk | SCB_I2C_CTRL_S_NOT_READY_DATA_NACK_Msk);
	}
    /* Check write complete event. */
    if (0UL != (CY_SCB_I2C_SLAVE_WR_CMPLT_EVENT & event))
    {
        Cy_SCB_I2C_SlaveConfigWriteBuf(SCB3, i2c_read_buffer, SL_RD_BUFFER_SIZE, &i2cContext);
    }

    /* Check write complete event. */
    if (0UL != (CY_SCB_I2C_SLAVE_RD_CMPLT_EVENT & event))
    {
    	Cy_SCB_I2C_SlaveConfigReadBuf(SCB3, i2c_write_buffer, SL_WR_BUFFER_SIZE, &i2cContext);
    }
}
#endif

int main(void)
{
	uint32_t dataRate;

	 cy_rslt_t result;

	/* Initialize the device and board peripherals */
	result = cybsp_init() ;
	if (result != CY_RSLT_SUCCESS)
	{
		CY_ASSERT(0);
	}
#ifdef I2C_USE
	 Cy_GPIO_SetHSIOM(CYBSP_I2C_SCL_PORT, CYBSP_I2C_SCL_PIN, P6_0_SCB3_I2C_SCL);
	 Cy_GPIO_SetHSIOM(CYBSP_I2C_SDA_PORT, CYBSP_I2C_SDA_PIN, P6_1_SCB3_I2C_SDA);
	  /* Configure pins for I2C operation */
	  Cy_GPIO_SetDrivemode(CYBSP_I2C_SCL_PORT, CYBSP_I2C_SCL_PIN, CY_GPIO_DM_OD_DRIVESLOW);
	  Cy_GPIO_SetDrivemode(CYBSP_I2C_SDA_PORT, CYBSP_I2C_SDA_PIN, CY_GPIO_DM_OD_DRIVESLOW);


    Cy_SCB_I2C_Init(SCB3, &CYBSP_I2C_config, &i2cContext);
    /* Configure write and read buffers for communication with master */
    Cy_SCB_I2C_SlaveConfigReadBuf (SCB3, i2c_read_buffer,  SL_RD_BUFFER_SIZE, &i2cContext);
    Cy_SCB_I2C_SlaveConfigWriteBuf(SCB3, i2c_write_buffer, SL_WR_BUFFER_SIZE, &i2cContext);
	Cy_SCB_I2C_RegisterEventCallback(SCB3, (cy_cb_scb_i2c_handle_events_t) HandleEventsSlave, &i2cContext);

    const cy_stc_sysint_t i2cIntrConfig =
    {
        .intrSrc      = NvicMux8_IRQn,
		.cm0pSrc	  = scb_3_interrupt_IRQn,
        .intrPriority = 2UL,
    };
    /* Hook interrupt service routine and enable interrupt */
    (void) Cy_SysInt_Init(&i2cIntrConfig, &I2C_Isr);

    NVIC_EnableIRQ(NvicMux8_IRQn);

    /* Enable I2C to operate */
    Cy_SCB_I2C_Enable(SCB3);
#endif
    /* Enable global interrupts */
    __enable_irq();

    /* Lock the sempahore to wait for CM4 to be init */
    Cy_IPC_Sema_Set(SEMA_NUM, false);

    /* Enable CM4. CY_CORTEX_M4_APPL_ADDR must be updated if CM4 memory layout is changed. */
     Cy_SysEnableCM4(CY_CORTEX_M4_APPL_ADDR);

    /* Wait till CM4 unlocks the semaphore */
    do
    {
        __WFE();
    }
    while (Cy_IPC_Sema_Status(SEMA_NUM) == CY_IPC_SEMA_STATUS_LOCKED);

    /* Update clock settings */
    SystemCoreClockUpdate();

    for (;;)
    {
        /* Check if the button is pressed */
        if (Cy_GPIO_Read(CYBSP_SW2_PORT, CYBSP_SW2_PIN) == 0)
        {        
        #if ENABLE_SEMA
            if (Cy_IPC_Sema_Set(SEMA_NUM, false) == CY_IPC_SEMA_SUCCESS)
        #endif        
            {
                /* Print a message to the console */
                Cy_SCB_UART_PutString(CYBSP_UART_HW, "Message sent from CM0+\r\n");

            #if ENABLE_SEMA    
                while (CY_IPC_SEMA_SUCCESS != Cy_IPC_Sema_Clear(SEMA_NUM, false));
            #endif
            }
        }    
    }
}

/* [] END OF FILE */
