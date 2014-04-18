/**
 *@file uartTx.c
 *
 *@brief
 *  - receive data over UART
 *
 * Target:   TLL6527v1-0
 * Compiler: VDSP++     Output format: VDSP++ "*.dxe"
 *
 * @author  Tim Liming
 *
 *******************************************************************************/
#include "tll_common.h"
#include "uartTx.h"
#include "bufferPool.h"
#include "isrDisp.h"
#include <tll_config.h>
#include <tll_sport.h>
#include <queue.h>
#include <power_mode.h>

/** Configure the UART DMA
 * Configures the DMA tx with the buffer and the buffer length to
 * receive
 * Parameters:
 * @param pchunk  pointer to receive chunk
 *
 * @return void
 */
void uartTx_dmaConfig(chunk_t *pChunk)
{
	/* 1. Disable DMA 11 */
	DISABLE_DMA(*pDMA11_CONFIG);

	/* 2. Configure start address */
	*pDMA11_START_ADDR = &pChunk->u08_buff; // should this match audio?

	/* 3. set X count */
	*pDMA11_X_COUNT = pChunk->bytesUsed;	// should this match audio?

	/* 4. set X modify */
	*pDMA11_X_MODIFY = 1;

	/* 5. enable interrupt register */
	*pUART1_IER |= ETBEI;

	/* 6. Re-enable DMA */
	ENABLE_DMA(*pDMA11_CONFIG);
}


/** Initialize uart tx
 *    - get pointer to buffer pool
 *    - register interrupt handler
 *    - initialize RX queue

 * Parameters:
 * @param pThis  pointer to own object
 * @param pBuffP  pointer to buffer pool to take and return chunks from
 * @param pIsrDisp   pointer to interrupt dispatcher to get ISR registered
 *
 * @return Zero on success.
 * Negative value on failure.
 */
int uartTx_init(uartTx_t *pThis, bufferPool_t *pBuffP, isrDisp_t *pIsrDisp)
{
	// parameter checking
	if ( NULL == pThis || NULL == pBuffP || NULL == pIsrDisp ) {
		printf("[UART TX]: Failed init \r\n");
		return FAIL;
	}

	pThis->pPending     = NULL;
	pThis->pBuffP       = pBuffP;

	// init queue
	queue_init(&pThis->queue, UARTTX_QUEUE_DEPTH);

	/* Configure the DMA11 for TX (data transfer/memory read) */
	*pDMA11_CONFIG = WDSIZE_16 | DI_EN | DMA2D; // not sure if this should be 2D

	// register own ISR to the ISR dispatcher
	isrDisp_registerCallback(pIsrDisp, ISR_DMA11_UART1_TX, uartTx_isr, pThis);

	printf("[UART TX]: TX init complete \r\n");

	return PASS;
}


/** start uart tx
 *   - empty for now
 * Parameters:
 * @param pThis  pointer to own object
 *
 * @return Zero on success.
 * Negative value on failure.
 */
int uartTx_start(uartTx_t *pThis)
{
	printf("[UART TX]: uartTx_start: implemented \r\n");

	// empty nothing to be done, DMA kicked off during run time
	return PASS;
}


/** uart tx isr  (to be called from dispatcher)
 *   - get chunk from tx queue
 *    - if valid, release old pending chunk to buffer pool
 *    - configure DMA
 *    - if not valid, configure DMA to replay same chunk again
 * Parameters:
 * @param pThis  pointer to own object
 *
 * @return None
 */
void uartTx_isr(void *pThisArg)
{
	// create local casted pThis to avoid casting on every single access
	uartTx_t  *pThis = (uartTx_t*) pThisArg;

	chunk_t *pchunk = NULL;

	// validate that TX DMA IRQ was triggered
	if ( *pDMA11_IRQ_STATUS & 0x1  ) {
		/* 1. Attempt to get the new chunk, and check if it's available: */
		if (PASS == queue_get(&pThis->queue, (void **)&pchunk) ) {
			/* 2. If so, release old chunk on success back to buffer pool */
			bufferPool_release(pThis->pBuffP, pThis->pPending);

			/* 3. Register the new chunk as pending */
			pThis->pPending = pchunk;

		} else {
			printf("[UART TX]: TX Queue Empty! \r\n");
		}

		*pDMA11_IRQ_STATUS |= 0x0001; // Clear the interrupt

		// config DMA either with new chunk (if there was one), or with old chunk on empty Q
		uartTx_dmaConfig(pThis->pPending);
	}
}



/** uart tx put
 *   copies filled pChunk into the TX queue for transmission
 *    if queue is full, then chunk is dropped
 * Parameters:
 * @param pThis  pointer to own object
 *
 * @return Zero on success.
 * Negative value on failure.
 */
int uartTx_put(uartTx_t *pThis, chunk_t *pChunk)
{
	chunk_t *pchunk_temp = NULL;

	if ( NULL == pThis || NULL == pChunk ) {
		printf("[UART TX]: Failed to put \r\n");
		return FAIL;
	}

	// do uart put stuff here
}

