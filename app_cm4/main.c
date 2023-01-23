#include "cy_pdl.h"
#include "cyhal.h"
#include "cybsp.h"
#include "ipc_def.h"
#include <stdio.h>

#if ENABLE_SEMA
    #define LED_STATE   CYBSP_LED_STATE_OFF
#else
    #define LED_STATE   CYBSP_LED_STATE_ON
#endif
//#define I2C_USE

#ifdef I2C_USE
#define PACKET_SIZE             (0xFFu)
//
///* Master write and read buffer of size three bytes */
#define SL_RD_BUFFER_SIZE       (PACKET_SIZE)
#define SL_WR_BUFFER_SIZE       (PACKET_SIZE)
//
///* Start and end of packet markers */
#define PACKET_SOP              (0x01u)
#define PACKET_EOP              (0x17u)
//
///* Command valid status */
//#define STS_CMD_DONE            (0x00u)
#define STS_CMD_FAIL            (0xFFu)
//
///* Packet positions */
//#define PACKET_SOP_POS          (0x00u)
//#define PACKET_STS_POS          (0x01u)
//#define PACKET_LED_POS          (0x01u)
//#define PACKET_EOP_POS          (0x02u)
//
uint8_t i2c_read_buffer [SL_RD_BUFFER_SIZE] =
                                       {PACKET_SOP, STS_CMD_FAIL, PACKET_EOP};
uint8_t i2c_write_buffer[SL_WR_BUFFER_SIZE];
//cyhal_i2c_t i2c_slave;
//cyhal_i2c_cfg_t i2c_slave_cfg = {true, I2C_SLAVE_ADDRESS, I2C_SLAVE_FREQ};
//void i2c_slave_init(void);
//void handle_i2c_slave_events(void *callback_arg, cyhal_i2c_event_t event);
//void handle_error(void);
cy_stc_scb_i2c_context_t i2cContext;

void HandleEventsSlave(uint32 event);
void I2C_Isr(void)
{
    Cy_SCB_I2C_Interrupt(SCB3, &i2cContext);
}

void HandleEventsSlave(uint32_t event)
{
	int i;
    Cy_SCB_UART_PutString(CYBSP_UART_HW, "I2C interrupt\r\n");
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
cy_stc_scb_uart_context_t uart_context;
void HandleEventsUart(uint32 event);

void UART_Isr(void)
{
	Cy_SCB_UART_Interrupt(CYBSP_UART_HW, &uart_context);
}

void HandleEventsUart(uint32 event)
{
	CyDelay(100);
}

int main(void)
{
    cy_rslt_t result;
    char txBuffer[100];
    /* Initialize the device and board peripherals */
    /*result = cybsp_init() ;
    if (result != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }*/


    /* Initialize the SCB block to use the debug UART port */
    Cy_SCB_UART_Init(CYBSP_UART_HW, &CYBSP_UART_config, &uart_context);
    const cy_stc_sysint_t uartIntrConfig =
    {
        .intrSrc      = CYBSP_UART_IRQ,
        .intrPriority = 1UL,
    };

	(void) Cy_SysInt_Init(&uartIntrConfig, &UART_Isr);
	NVIC_EnableIRQ(CYBSP_UART_IRQ);
    Cy_SCB_UART_RegisterCallback(CYBSP_UART_HW, (cy_cb_scb_uart_handle_events_t)HandleEventsUart, &uart_context);

    /* Enabling the SCB block for UART operation */
    Cy_SCB_UART_Enable(CYBSP_UART_HW);

    /* Initialize the User Button */
    Cy_GPIO_Pin_Init(CYBSP_SW2_PORT, CYBSP_SW2_PIN, &CYBSP_SW2_config);

    /* Initialize the User LED */
    cyhal_gpio_init(CYBSP_USER_LED, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, LED_STATE);




#ifdef I2C_USE
    Cy_SCB_I2C_Init(SCB3, &CYBSP_I2C_config, &i2cContext);
	/* Configure write and read buffers for communication with master */
	Cy_SCB_I2C_SlaveConfigReadBuf (SCB3, i2c_read_buffer,  SL_RD_BUFFER_SIZE, &i2cContext);
	Cy_SCB_I2C_SlaveConfigWriteBuf(SCB3, i2c_write_buffer, SL_WR_BUFFER_SIZE, &i2cContext);
	Cy_SCB_I2C_RegisterEventCallback(SCB3, (cy_cb_scb_i2c_handle_events_t) HandleEventsSlave, &i2cContext);

	  /* Configure master to operate with desired data rate */
//    dataRate = Cy_SCB_I2C_SetDataRate(SCB3, 100000U, Cy_SysClk_PeriphGetFrequency(CY_SYSCLK_DIV_8_BIT, 0U));
//    if ((dataRate > 100000U) || (dataRate == 0U))
//    {
//        /* Can not reach desired data rate */
//        CY_ASSERT(0U);
//    }

	/* Populate configuration structure (code specific for CM4) */
	const cy_stc_sysint_t i2cIntrConfig =
	{
		.intrSrc      = scb_3_interrupt_IRQn,
		.intrPriority = 7UL,
	};
	/* Hook interrupt service routine and enable interrupt */
	(void) Cy_SysInt_Init(&i2cIntrConfig, &I2C_Isr);
	NVIC_EnableIRQ(scb_3_interrupt_IRQn);

	/* Enable I2C to operate */
	Cy_SCB_I2C_Enable(SCB3);
#endif
    /* Enable global interrupts */
    __enable_irq();

    /* \x1b[2J\x1b[;H - ANSI ESC sequence for clear screen */

    sprintf(txBuffer, "\x1b[2J\x1b[;H");
    Cy_SCB_UART_Transmit(CYBSP_UART_HW, txBuffer, 12,&uart_context);
    //Cy_SCB_UART_PutString(CYBSP_UART_HW, "\x1b[2J\x1b[;H");

    //Cy_SCB_UART_PutString(CYBSP_UART_HW, "******************  IPC Semaphore Example  ****************** \r\n\n");

    //Cy_SCB_UART_PutString(CYBSP_UART_HW, "<Press the kit's user button to print messages>\r\n\n");
    sprintf(txBuffer, "\n\n\r******************  I2C_DUALCORE_WITH_SHAREDRAM  ****************** \r\n\n");
    Cy_SCB_UART_Transmit(CYBSP_UART_HW, txBuffer, 65,&uart_context);
    sprintf(txBuffer, "\n\n\r<Press the kit's user button to print messages>\n\n\r");
    Cy_SCB_UART_Transmit(CYBSP_UART_HW, txBuffer, 50,&uart_context);
    /* Unlock the semaphore and wake-up the CM0+ */
    Cy_IPC_Sema_Clear(SEMA_NUM, false);
    __SEV();

    for (;;)
    {
        if (cyhal_gpio_read(CYBSP_USER_BTN) == CYBSP_BTN_PRESSED)
        {
        #if ENABLE_SEMA
            /* Attempt to lock the semaphore */
            if (Cy_IPC_Sema_Set(SEMA_NUM, false) == CY_IPC_SEMA_SUCCESS)
        #endif
            {
                /* Print a message to the console */
                sprintf(txBuffer, "Message sent from CM4\r\n");
                Cy_SCB_UART_Transmit(CYBSP_UART_HW, txBuffer, 23,&uart_context);
            #if ENABLE_SEMA
                while (CY_IPC_SEMA_SUCCESS != Cy_IPC_Sema_Clear(SEMA_NUM, false));
            #endif                
            }
        }
    }
}


/* [] END OF FILE */
