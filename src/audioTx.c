/**
 *@file audioTx.c
 *
 *@brief
 *  - receive audio samples from DMA
 *
 * Target:   TLL6527v1-0      
 * Compiler: VDSP++     Output format: VDSP++ "*.dxe"
 *
 * @author  Gunar Schirner
 *          Rohan Kangralkar
 * @date 03/15/2009
 *
 * LastChange:
 * $Id: audioTx.c 813 2013-03-12 03:06:44Z ovaskevi $
 *
 *******************************************************************************/
#include "tll_common.h"
#include "audioTx.h"
#include "bufferPool.h"
#include "isrDisp.h"
#include <tll_config.h>
#include <tll_sport.h>
#include <queue.h>
#include <power_mode.h>

/** 
 * Configures the DMA tx with the buffer and the buffer length to 
 * transfer
 * Parameters:
 * @param pchunk  pointer to tx chunk
 * @return void
 */
void audioTx_dmaConfig(chunk_t *pchunk)
{
	/* 1. Disable DMA 4*/
	DISABLE_DMA(*pDMA4_CONFIG);

	/* 2. Configure start address */
	*pDMA4_START_ADDR = &pchunk->u16_buff[0];

	/* 3. set X count */
	*pDMA4_X_COUNT = pchunk->len/2;//*pDMA4_X_COUNT = 2;//pchunk->bytesUsed/2;
	//*pDMA4_Y_COUNT = pchunk->len/2; // 16 bit data so we change the stride and count
   
	/* 4. set X modify */
	*pDMA4_X_MODIFY = 2; //*pDMA4_X_MODIFY = 0;
	//*pDMA4_Y_MODIFY = 2;
   
	/* 5. Re-enable DMA */
	ENABLE_DMA(*pDMA4_CONFIG);

}


/** Initialize audio tx
 *    - get pointer to buffer pool
 *    - register interrupt handler
 *    - initialize TX queue

 * Parameters:
 * @param pThis  pointer to own object
 * @param pBuffP  pointer to buffer pool to take and return chunks from
 * @param pIsrDisp   pointer to interrupt dispatcher to get ISR registered
 *
 * @return Zero on success.
 * Negative value on failure.
 */
int audioTx_init(audioTx_t *pThis, bufferPool_t *pBuffP,
                 isrDisp_t *pIsrDisp)
{
    // parameter checking
    if ( NULL == pThis || NULL == pBuffP || NULL == pIsrDisp ) {
        printf("[Audio TX]: Failed init \r\n");
        return FAIL;
    }
    
    // store pointer to buffer pool for later access     
    pThis->pBuffP       = pBuffP;

    pThis->pPending     = NULL; // nothing pending
    pThis->running      = 0;    // DMA turned off by default
    
    // init queue 
    queue_init(&pThis->queue, AUDIOTX_QUEUE_DEPTH);   
 
    /* Configure the DMA4 for TX (data transfer/memory read) */
    /* Read, 2-D, interrupt enabled, 16 bit transfer, Auto buffer */
    *pDMA4_CONFIG = WDSIZE_16 | DI_EN; //| DMA2D; /* 16 bit and DMA enable */
    
    // register own ISR to the ISR dispatcher
    isrDisp_registerCallback(pIsrDisp, ISR_DMA4_SPORT0_TX, audioTx_isr, pThis);
    
    printf("[Audio TX]: TX init complete \r\n");
    
    return PASS;
}



/** start audio tx
 *   - empty for now
 * Parameters:
 * @param pThis  pointer to own object
 *
 * @return Zero on success.
 * Negative value on failure.
 */
int audioTx_start(audioTx_t *pThis)
{
     
    printf("[Audio TX]: audioTx_start: implemented \r\n");

    // empty nothing to be done, DMA kicked off during run time 
    return PASS;   
}



/** audio tx isr  (to be called from dispatcher) 
 *   - get chunk from tx queue
 *    - if valid, release old pending chunk to buffer pool 
 *    - configure DMA 
 *    - if not valid, configure DMA to replay same chunk again
 * Parameters:
 * @param pThis  pointer to own object
 *
 * @return None 
 */
void audioTx_isr(void *pThisArg)
{
    // create local casted pThis to avoid casting on every single access
    audioTx_t  *pThis = (audioTx_t*) pThisArg;

    chunk_t *pchunk = NULL;

    // validate that TX DMA IRQ was triggered 
    if ( *pDMA4_IRQ_STATUS & 0x1  ) {
        //printf("[TXISR]\n");
        /* We need to remove the data from the queue and create space for more data
         * (The data was read previously by the DMA)

        1. First, attempt to get the new chunk, and check if it's available: */
    	if (PASS == queue_get(&pThis->queue, (void **)&pchunk) ) {
    		/* 2. If so, release old chunk on success back to buffer pool */
    		bufferPool_release(pThis->pBuffP, pThis->pPending);

    		/* 3. Register the new chunk as pending */
    		pThis->pPending = pchunk;

    	} else {
    		//printf("[Audio TX]: TX Queue Empty! \r\n");
    	}

        *pDMA4_IRQ_STATUS  |= DMA_DONE;     // Clear the interrupt

        // config DMA either with new chunk (if there was one), or with old chunk on empty Q
        audioTx_dmaConfig(pThis->pPending);
    }
}




/** audio tx put
 *   copies filled pChunk into the TX queue for transmission
 *    if queue is full, then chunk is dropped 
 * Parameters:
 * @param pThis  pointer to own object
 *
 * @return Zero on success.
 * Negative value on failure.
 */
int audioTx_put(audioTx_t *pThis, chunk_t *pChunk)
{
	chunk_t *pchunk_temp = NULL;

    if ( NULL == pThis || NULL == pChunk ) {
        //printf("[Audio TX]: Failed to put \r\n");
        return FAIL;
    }
    
    // block if queue is full
    //while(queue_is_full(&pThis->queue)) {
    if(queue_is_full(&pThis->queue)) {
        //printf("[Audio TX]: Queue Full \r\n");
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
        	pThis->running  = 1;
            pThis->pPending = pchunk_temp;
            audioTx_dmaConfig(pThis->pPending);
            ENABLE_SPORT0_TX();

        } else {
        	/* DMA already running add chunk to queue */
            if ( PASS != queue_put(&pThis->queue, pchunk_temp) ) {
            	// return chunk to pool if queue is full, effectively dropping the chunk
                bufferPool_release(pThis->pBuffP, pchunk_temp);
                return FAIL;
            }
        }

    } else {
    	// drop if we don't get free space
    	//printf("[Audio TX]: failed to get buffer \r\n");
    	return FAIL;
    }
    
    return PASS;
}

