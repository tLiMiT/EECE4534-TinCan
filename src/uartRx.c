/**
 *@file uartRx.c
 *
 *@brief
 *  - receive data over UART
 *
 * Target:   TLL6527v1-0
 * Compiler:
 *
 * @author Tim Liming
 * 		   Mark Hatch
 *
 *******************************************************************************/
#include "tll_common.h"
#include "uartRx.h"
#include "bufferPool.h"
#include "isrDisp.h"
#include <tll_config.h>
#include <tll_sport.h>
#include <queue.h>
#include <power_mode.h>


/**
 * Configures the DMA rx with the buffer and the buffer length to receive
 * Parameters:
 * @param pchunk	pointer to receive chunk
 * @param len  		length
 *
 * @return void
 */
void uartRx_dmaConfig(char *cPtr, unsigned short len)
{
	// Disable DMA
	DISABLE_DMA(*pDMA10_CONFIG);

	// Configure start address
	*pDMA10_START_ADDR = cPtr;

	// Set X count
	*pDMA10_X_COUNT = len;

	// Set X modify
	*pDMA10_X_MODIFY = 1;

	// Enable Receive Buffer Full Interrupt
	*pUART1_IER |= 0x1;

	// Re-enable DMA
	ENABLE_DMA(*pDMA10_CONFIG);

	//printf("[UART]: RX DMA Config Complete\n");
}

/**
 *
 */
int uartRx_init(uartRx_t *pThis, bufferPool_t *pBuffP, isrDisp_t *pIsrDisp)
{

}

