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
#include <audioSample.h>

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
	*pDMA3_CONFIG &= ~(DMAEN);

	/* 2. Configure start address */
	*pDMA3_START_ADDR = &pchunk->u16_buff[0];

	/* 3. set X count */
	*pDMA3_X_COUNT = pchunk->bytesUsed/2;

	/* 4. set X modify */
	*pDMA3_X_MODIFY = 2;

	/* 5 Re-enable DMA */
	*pDMA3_CONFIG |= DMAEN;

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
        printf("[ARX]: Failed init\r\n");
        return FAIL;
    }
    
    pThis->pPending     = NULL;
    pThis->pBuffP       = pBuffP;
    
    // init queue with 
    if(FAIL == queue_init(&pThis->queue, AUDIORX_QUEUE_DEPTH))
    	printf("[ARX]: Queue init failed\n");

    *pDMA3_CONFIG = WDSIZE_16 | DI_EN | WNR | FLOW_AUTO; /* 16 bit amd DMA enable */

    // register own ISR to the ISR dispatcher
    isrDisp_registerCallback(pIsrDisp, ISR_DMA3_SPORT0_RX, audioRx_isr, pThis);

    printf("[ARX]: RX init complete\r\r\n");
    
    return PASS;
}




/** start audio rx
 *    - start receiving first chunk from DMA
 *      - acqurie chunk from pool 
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

//#ifdef ENABLE_FILE_STUB
	//pThis->audioRx_pFile = fopen(FILE_NAME, "rb");
   // if(NULL == pThis->audioRx_pFile) {
   //     printf("Can't open files: %s\r\n",FILE_NAME);
   // }
   // fseek(pThis->audioRx_pFile, 44, SEEK_SET); // remove wav header
//#else
//    pThis->audioSample.count = 44; /** we skip the wave header we assume that the data is
//                             16 bit mono*/
//#endif
		printf("[AUDIO RX]: audioRx_start: implemented\r\n");
	/* prime the system by getting the first buffer filled */
	   if ( FAIL == bufferPool_acquire(pThis->pBuffP, &pThis->pPending ) ) {
	         printf("[ARX]: Failed to acquire buffer\n");
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
	//printf("RX ISR\n");
	// local pThis to avoid constant casting
	audioRx_t *pThis  = (audioRx_t*) pThisArg;

	if ( *pDMA3_IRQ_STATUS & 0x1 ) {
		//  chunk is now filled, so update the length
        pThis->pPending->bytesUsed = pThis->pPending->bytesMax;

        /* 1. Attempt to insert the pending chunk previously read by the
         * DMA RX into the RX QUEUE and a data is inserted to queue
         */
        if (queue_put(&pThis->queue, pThis->pPending) == FAIL) {
            /* 2. If chunk could not be inserted into the queue,
             * configure the DMA and overwrite last samples.
             * This means the RX packet was dropped */
        	audioRx_dmaConfig(pThis->pPending);
        } else {

        	/* 3. Otherwise, attempt to acquire a chunk from the buffer
			 * pool.
			 */
			if (bufferPool_acquire(pThis->pBuffP, &pThis->pPending) == PASS) {
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

        *pDMA3_IRQ_STATUS  |= 0x0001;   // clear the interrupt
    }
}



/** audio rx get 
 * @brief Read the chunk from the file
 * Parameters:
 * @param pThis  pointer to own object
 * @param pChunk Chunk Pointer to be filled
 *
 * @return Zero on success.
 * Negative value on failure.
 */
int audioRx_get(audioRx_t *pThis, chunk_t **ppChunk)
{
///Test code for audio sample.
/*	int size = 0;

    if (bufferPool_acquire(pThis->pBuffP, pChunk) != PASS) {
        printf("Could not acquire chunk for audio sample\n");
        return FAIL;
    }

#ifdef ENABLE_FILE_STUB
    size = audioRx_fileRead(pThis->audioRx_pFile, *pChunk);
#else
    size = audioSample_get(&pThis->audioSample, *pChunk);
#endif
*/
		chunk_t                  *chunk_rx;

		/* Block till a chunk arrives on the rx queue */
		while( queue_is_empty(&pThis->queue) ) {
			asm("nop;");
		}

		if(PASS == queue_get(&pThis->queue, (void**)&chunk_rx))
			printf("[ARX]: Got chunk from queue\n");

		chunk_copy(chunk_rx, *ppChunk);
		printf("ppChunk: %i\n", (*ppChunk)->u16_buff[0]);

		if ( FAIL == bufferPool_release(pThis->pBuffP, chunk_rx) ) {
			printf("[ARX]: Failed to release chunk to pool");
			return FAIL;
		}

		return PASS;
    }

int audioRx_getNbNc(audioRx_t *pThis, chunk_t **ppChunk)
{

    // check if something in queue
    if(queue_is_empty(&pThis->queue)){
    	return FAIL;
    } else {
        queue_get(&pThis->queue, (void**)ppChunk);
        return PASS;
    }
}

