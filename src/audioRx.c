/**
 *@file audioRx.c
 *
 *@brief
 *  - receive audio samples from DMA
 *
 * Target:   TLL6527v1-0      
 * Compiler: VDSP++     Output format: VDSP++ "*.dxe"
 *
 * @author  Gunar Schirner
 *           Rohan Kangralkar
 * @date 03/15/2009
 *
 * LastChange:
 * $Id: audioRx.c 846 2014-02-27 15:35:54Z fengshen $
 *
 *******************************************************************************/
#include "tll_common.h"
#include "audioRx.h"
#include "bufferPool.h"
#include "isrDisp.h"
#include <tll_config.h>
#include <tll_sport.h>
#include <queue.h>
#include <power_mode.h>

/** 
 * Configures the DMA rx with the buffer and the buffer length to 
 * receive
 * Parameters:
 * @param pchunk  pointer to receive chunk
 *
 * @return void
 */
void audioRx_dmaConfig(chunk_t *pchunk)
{
	/* 1. Disable DMA 3*/
	DISABLE_DMA(*pDMA3_CONFIG);

	/* 2. Configure start address */
	*pDMA3_START_ADDR = &pchunk->u16_buff[0];

	/* 3. set X count */
	*pDMA3_X_COUNT = 2;
	*pDMA3_Y_COUNT = pchunk->size/2; // 16 bit data so we change the stride and count

	/* 4. set X modify */
	*pDMA3_X_MODIFY = 0;
	*pDMA3_Y_MODIFY = 2;

	/* 5 Re-enable DMA */
	ENABLE_DMA(*pDMA3_CONFIG);

}



/** Initialize audio rx
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
int audioRx_init(audioRx_t *pThis, bufferPool_t *pBuffP,
                 isrDisp_t *pIsrDisp)
{
    if ( NULL == pThis || NULL == pBuffP || NULL == pIsrDisp) {
        printf("[Audio RX]: Failed init\r\n");
        return FAIL;
    }
    
    pThis->pPending     = NULL;
    pThis->pBuffP       = pBuffP;
    
    // init queue with 
    if(FAIL == queue_init(&pThis->queue, AUDIORX_QUEUE_DEPTH))
    	printf("[Audio RX]: Queue init failed\n");

    *pDMA3_CONFIG = WDSIZE_16 | DI_EN | WNR | DMA2D; /* 16 bit and DMA enable */

    // register own ISR to the ISR dispatcher
    isrDisp_registerCallback(pIsrDisp, ISR_DMA3_SPORT0_RX, audioRx_isr, pThis);

    printf("[Audio RX]: RX init complete \r\n");
    
    return PASS;
}




/** start audio rx
 *    - start receiving first chunk from DMA
 *      - acquire chunk from pool
 *      - config DMA 
 *      - start DMA + SPORT
 * Parameters:
 * @param pThis  pointer to own object
 *
 * @return Zero on success.
 * Negative value on failure.
 */
int audioRx_start(audioRx_t *pThis)
{
	printf("[Audio RX]: audioRx_start: implemented \r\n");

	/* prime the system by getting the first buffer filled */
	if ( FAIL == bufferPool_acquire(pThis->pBuffP, &pThis->pPending ) ) {
		printf("[Audio RX]: Failed to acquire buffer\n");
		return FAIL;
	}

	audioRx_dmaConfig(pThis->pPending);

	// enable the audio transfer
	ENABLE_SPORT0_RX();

	return PASS;
}



/** audio rx isr  (to be called from dispatcher) 

 * Parameters:
 * @param pThis  pointer to own object
 *
 * @return None 
 */
void audioRx_isr(void *pThisArg)
{
	//printf("[ARX]; RX ISR \r\n");

	// local pThis to avoid constant casting
	audioRx_t *pThis = (audioRx_t*) pThisArg;

	if ( *pDMA3_IRQ_STATUS & 0x1 ) {

		//  chunk is now filled, so update the length
        pThis->pPending->len = pThis->pPending->size;

        /* Insert the chunk previously read by the DMA RX on the
         * RX QUEUE and a data is inserted to queue
         */
        if ( FAIL == queue_put(&pThis->queue, pThis->pPending) ) {

        	// reuse the same buffer and overwrite last samples
        	audioRx_dmaConfig(pThis->pPending);
        	//printf("[ARX INT]: RX Packet Dropped \r\n");
        } else {

        	/* Otherwise, attempt to acquire a chunk from the buffer
			 * pool.
			 */
			if ( PASS == bufferPool_acquire(pThis->pBuffP, &pThis->pPending) ) {
				/* If successful, configure the DMA to write
				 * to this chunk.
				 */
				audioRx_dmaConfig(pThis->pPending);
			} else {

				/* If not successful, then we are out of
				 * memory because the buffer pool is empty.
				 */
				//printf("Buffer pool is empty!\n");
			}
        }
        *pDMA3_IRQ_STATUS |= 0x0001;   // clear the interrupt
    }
}


/** audio rx get
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
int audioRx_get(audioRx_t *pThis, chunk_t *pChunk)
{
	chunk_t *chunk_rx;

    /* Block till a chunk arrives on the rx queue */
   // while( queue_is_empty(&pThis->queue) ) {
	if( queue_is_empty(&pThis->queue) ) {
    	printf("[ARX] Queue is empty\r\n");
    	return FAIL;
        //powerMode_change(PWR_ACTIVE);
        //asm("idle;");
    }
   //powerMode_change(PWR_FULL_ON);

    queue_get(&pThis->queue, (void**)&chunk_rx);

    chunk_copy(chunk_rx, pChunk);

    if ( FAIL == bufferPool_release(pThis->pBuffP, chunk_rx) ) {
        return FAIL;
    }
    return PASS;
}


/** audio rx get (no block, no copy)
 *
 * @brief Read the chunk from the file
 * Parameters:
 * @param pThis  pointer to own object
 * @param pChunk Chunk Pointer to be filled
 *
 * @return Zero on success.
 * Negative value on failure.
 */
int audioRx_getNbNc(audioRx_t *pThis, chunk_t **ppChunk)
{
	int retval = FAIL;
    // check if something in queue
    if(queue_is_empty(&pThis->queue))
    {
    	return retval;
    }
    else {
        if(PASS == queue_get(&pThis->queue, (void**)ppChunk))
        	retval = PASS;
        else
        {
        	printf("[Audio RX]: Failed to get chunk\r\n");
        }
    }
    return retval;
}

