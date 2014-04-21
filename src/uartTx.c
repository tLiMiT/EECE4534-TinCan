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
	*pDMA11_START_ADDR = &pChunk->u16_buff[0]; // should this match audioTx?

	/* 3. set X count */
	*pDMA11_X_COUNT = pChunk->len/2;//*pDMA11_X_COUNT = 2; // should this match audioTx?
	//*pDMA11_Y_COUNT = pChunk->len/2;

	/* 4. set X modify */
	*pDMA11_X_MODIFY = 2;//*pDMA11_X_MODIFY = 0;
	//*pDMA11_Y_MODIFY = 2;

	/* 5. Re-enable DMA */
	ENABLE_DMA(*pDMA11_CONFIG);

	/* 6. enable interrupt register */
	*pUART1_IER |= ETBEI;
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
	pThis->running      = 0;

	// init queue
	queue_init(&pThis->queue, UARTTX_QUEUE_DEPTH);

	/* Configure the DMA11 for TX (data transfer/memory read) */
	*pDMA11_CONFIG = SYNC | WDSIZE_16 | DI_EN; // not sure if this should be 2D

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
	//printf("[UTX ISR]\r\n");
	// create local casted pThis to avoid casting on every single access
	uartTx_t  *pThis = (uartTx_t*) pThisArg;

	chunk_t *pchunk = NULL;

	// validate that TX DMA IRQ was triggered
	if ( *pDMA11_IRQ_STATUS & 0x1  ) {
		/* 1. Attempt to get the new chunk, and check if it's available: */
		if (PASS == queue_get(&pThis->queue, (void **)&pchunk) ) {
			//printf("[UTX ISR] AC\r\n");
			/* 2. If so, release old chunk on success back to buffer pool */
			bufferPool_release(pThis->pBuffP, pThis->pPending);

			/* 3. Register the new chunk as pending */
			pThis->pPending = pchunk;

			// config DMA either with new chunk (if there was one), or with old chunk on empty Q
			uartTx_dmaConfig(pThis->pPending);
		} else {
			//printf("[UART TX]: TX Queue Empty! \r\n");

			// queue is empty, stop the DMA
			uartTx_dmaStop();

			// indicate that the DMA has stopped
			pThis->running = 0;
		}
		*pDMA11_IRQ_STATUS |= DMA_DONE;		// Clear the interrupt
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
	        //printf("[UART TX]: Failed to put \r\n");
	        return FAIL;
	    }

	    // block if queue is full
	    //while(queue_is_full(&pThis->queue)) {
	    if(queue_is_full(&pThis->queue)) {
	        //printf("[UART TX]: Queue Full \r\n");
	        return FAIL;
	        //powerMode_change(PWR_ACTIVE);
	        //asm("idle;");
	    }
	    //powerMode_change(PWR_FULL_ON);

	    // get free chunk from pool
		if ( PASS == bufferPool_acquire(pThis->pBuffP, &pchunk_temp) ) {
			// copy chunk into free buffer for queue
			chunk_copy(pChunk, pchunk_temp);

			/* If DMA not running ? */
			if ( 0 == pThis->running ) {
				/* directly put chunk to DMA transfer & enable */
				//printf("[UART TX] put chunk directly onto DMA transfer\r\n");
				pThis->running  = 1;
				pThis->pPending = pchunk_temp;
				uartTx_dmaConfig(pThis->pPending);

			} else {
				//printf("[UART TX] Chunk added to queue\r\n");
				/* DMA already running add chunk to queue */
				if ( FAIL == queue_put(&pThis->queue, pchunk_temp) ) {
					//printf("[UART TX] Failed to add to queue\r\n");
					// return chunk to pool if queue is full, effectively dropping the chunk
					bufferPool_release(pThis->pBuffP, pchunk_temp);
					return FAIL;
				}
			}

		} else {
			// drop if we don't get free space
			//printf("[UART TX]: failed to get buffer \r\n");
			return FAIL;
		}

	    return PASS;

}


/* uart tx dma stop
 * - empty for now
 *
 * @return
 */
void uartTx_dmaStop(void)
{

	// disable interrupt
	*pUART1_IER &= ~ETBEI;

	// disable the DMA
	DISABLE_DMA(*pDMA11_CONFIG);

	return;
}

void uartTx_dmaStart(void)
{

	/* 6. Re-enable DMA */
	ENABLE_DMA(*pDMA11_CONFIG);

	/* 5. enable interrupt register */
	*pUART1_IER |= ETBEI;

	return;
}

