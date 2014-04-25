/**
 *@file uartRx.c
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
#include "uartRx.h"
#include "bufferPool.h"
#include "isrDisp.h"
#include <tll_config.h>
#include <tll_sport.h>
#include <queue.h>
#include <power_mode.h>

/** Configure the UART DMA
 * Configures the DMA rx with the buffer and the buffer length to
 * receive
 * Parameters:
 * @param pchunk  pointer to receive chunk
 *
 * @return void
 */
void uartRx_dmaConfig(chunk_t *pChunk)
{
	/* 1. Disable DMA 10 */
	DISABLE_DMA(*pDMA10_CONFIG);

	/* 2. Configure start address */
	*pDMA10_START_ADDR = &pChunk->u08_buff[0];	// should this match audioRx?

	/* 3. set X count */
	*pDMA10_X_COUNT = pChunk->size;//*pDMA10_X_COUNT = 2;
	//*pDMA10_Y_COUNT = pChunk->size/2; // 16 bit data so we change the stride and count

	/* 4. set X modify */
	*pDMA10_X_MODIFY = 1; //*pDMA10_X_MODIFY = 0;
	//*pDMA10_Y_MODIFY = 2;

	/* 5. Re-enable DMA */
	ENABLE_DMA(*pDMA10_CONFIG);
}


/** Initialize uart rx
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
int uartRx_init(uartRx_t *pThis, bufferPool_t *pBuffP, isrDisp_t *pIsrDisp)
{
	if ( NULL == pThis || NULL == pBuffP || NULL == pIsrDisp) {
		printf("[UART RX]: Failed init \r\n");
		return FAIL;
	}

	pThis->pPending = NULL;
	pThis->pBuffP = pBuffP;

	// init queue with
	if(FAIL == queue_init(&pThis->queue, UARTRX_QUEUE_DEPTH))
		printf("[UART RX]: Queue init failed \r\n");

	/* Configure the DMA10 for RX (data receive/memory write) */
	*pDMA10_CONFIG = WDSIZE_8 | DI_EN | WNR; //| DMA2D; // not sure if this should be 2D

	// register own ISR to the ISR dispatcher
	isrDisp_registerCallback(pIsrDisp, ISR_DMA10_UART1_RX, uartRx_isr, pThis);

	printf("[UART RX]: RX init complete \r\n");

	return PASS;
}


/** start uart rx
 *    - start receiving first chunk from DMA
 * 		- acquire chunk from pool
 *      - config DMA
 *      - start DMA + SPORT
 * Parameters:
 * @param pThis  pointer to own object
 *
 * @return Zero on success.
 * Negative value on failure.
 */
int uartRx_start(uartRx_t *pThis)
{
	printf("[UART RX]: uartRx_start: implemented \r\n");

	/* prime the system by getting the first buffer filled */
	if ( FAIL == bufferPool_acquire(pThis->pBuffP, &pThis->pPending ) ) {
		printf("[UART RX]: Failed to acquire buffer \r\n");
		return FAIL;
	}

	uartRx_dmaConfig(pThis->pPending);

	/* 6. enable interrupt register */
	*pUART1_IER |= ERBFI;

	return PASS;
}


/** uartRx_isr

 * Parameters:
 * @param pThisArg  pointer to own object
 *
 * @return None
 */
void uartRx_isr(void *pThisArg)
{
	int queueFail = 0;
	int bufferPoolFail = 0;
	//printf("[UART RX ISR]\r\n");
	// local pThis to avoid constant casting
	uartRx_t *pThis = (uartRx_t*) pThisArg;

	if ( *pDMA10_IRQ_STATUS & 0x1 ) {

		//  chunk is now filled, so update the length
        pThis->pPending->len = pThis->pPending->size;

        /* Insert the chunk previously read by the DMA RX on the
         * RX QUEUE and a data is inserted to queue
         */
        if ( FAIL == queue_put(&pThis->queue, pThis->pPending) ) {
        	queueFail = 1;
        	// reuse the same buffer and overwrite last samples
        	uartRx_dmaConfig(pThis->pPending);
        	//printf("[UART RX INT]: RX Packet Dropped \r\n");
        } else {

        	/* Otherwise, attempt to acquire a chunk from the buffer
			 * pool.
			 */
			if ( PASS == bufferPool_acquire(pThis->pBuffP, &pThis->pPending) ) {
				/* If successful, configure the DMA to write
				 * to this chunk.
				 */
				uartRx_dmaConfig(pThis->pPending);
			} else {
				bufferPoolFail = 1;
				/* If not successful, then we are out of
				 * memory because the buffer pool is empty.
				 */
				//printf("Buffer pool is empty!\n");
			}
        }
		*pDMA10_IRQ_STATUS |= DMA_DONE;		// clear the interrupt
	}
}


/** uart rx get
 *   copies a filled chunk into pChunk
 *   blocking call, blocks if queue is empty
 *     - get from queue
 *     - copy in to pChunk
 *     - release chunk to buffer pool
 * Parameters:
 * @param pThis  pointer to own object
 * @param pChunk Pointer to chunk object
 *
 * @return Zero on success.
 * Negative value on failure.
 */
int uartRx_get(uartRx_t *pThis, chunk_t *pChunk)
{
	chunk_t *chunk_rx;

	/* Block till a chunk arrives on the rx queue */
	//while( queue_is_empty(&pThis->queue) )
	if( queue_is_empty(&pThis->queue) )
	{
		//printf("[UART RX] Queue is empty\r\n");
		return FAIL;
		//powerMode_change(PWR_ACTIVE);
		//asm("idle;");
	}
	//powerMode_change(PWR_FULL_ON);

	 if(FAIL == queue_get(&pThis->queue, (void**)&chunk_rx))
		return FAIL;
	else {
		 chunk_copy(chunk_rx, pChunk);
		 bufferPool_release(pThis->pBuffP, chunk_rx);
		 return PASS;
	}

	return FAIL;
}


/* uart rx dma stop
 * - empty for now
 *
 * @return
 */
void uartRx_dmaStop(void)
{
	// disable interrupt
	*pUART1_IER &= ~ERBFI;

	// disable the DMA
	DISABLE_DMA(*pDMA10_CONFIG);

	return;
}
