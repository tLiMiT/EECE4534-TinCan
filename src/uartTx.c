/*****************************************************************************
 *@file: uartTx.c
 *
 *@brief: 
 *  - UART TX
 *
 * Target:   TLL6527v1-0      
 * Compiler: VDSP++     Output format: VDSP++ "*.dxe"
 *
 * @author:  James Kirk, Joao Mendes
 *
 *****************************************************************************/

#include "tll_common.h"
#include "uartTx.h"
#include "bufferPool.h"
#include "isrDisp.h"
#include <tll_config.h>
#include <tll_sport.h>
#include <queue.h>

/** Initialization of uartTx
 *
 * @param pThis  pointer to uartTx
 *
 * @return Zero on success, negative otherwise 
 */
int uartTx_init(uartTx_t *pThis, bufferPool_t *pBuffP, isrDisp_t *pIsrDisp)
{	
    if ((pThis == NULL) || (pBuffP == NULL) || (pIsrDisp == NULL)) 
    {
        //printf("[UTX]: Failed init\n");
        return FAIL;
    }
    
    pThis->pPending     = NULL;
    pThis->pBuffP       = pBuffP;
    pThis->running      = 0;    // DMA off
    
    queue_init(&pThis->queue, UARTTX_QUEUE_DEPTH);   

//    *pDMA11_CONFIG |= (0x0001 << 7);  //Enable the DMA interrupt
    *pDMA11_CONFIG = (DI_EN | SYNC);  //Enable the DMA interrupt
    
    //Register the interrupt handler
    isrDisp_registerCallback(pIsrDisp, ISR_DMA11_UART1_TX, uartTx_isr, pThis);
    
    //printf("[UTX]: TX init complete\n");
    
    return PASS;
}

/** uartTx_put
 *
 * @param pThis   pointer to uartTx
 * @param pChunk  pointer to data chunk
 *
 * @return Zero on success, negative otherwise 
 */
int uartTx_send(uartTx_t *pThis, chunk_t *pChunk)
{	

    if (NULL == pThis || NULL == pChunk) 
    {
        //printf("[TX]: Failed to put\n");
        return FAIL;
    }


	/* If DMA not running ? */
    if (0 == pThis->running)
    {
        pThis->running = 1;

		pThis->pPending = pChunk;

        // config DMA either with new chunk (if there was one)
        uartTx_dmaConfig(pThis->pPending);
	}
	else
	{
		/* DMA already running add chunk to queue */
		if (PASS != queue_put(&pThis->queue, pChunk))
		{
			//printf("[TX] Send, DMA RUNNING - FAIL PLACE IN QUEUE\n");
			// return chunk to pool if queue is full, effectivly dropping the chunk
			return FAIL;
		}
	}
    
    return PASS;
}

/** uartTx_dmaConfig
 * 
 * @param pchunk  pointer to receive chunk
 *
 * @return void
 */
void uartTx_dmaConfig(chunk_t *pChunk)
{
    DISABLE_DMA(*pDMA11_CONFIG);
    *pDMA11_START_ADDR   = &pChunk->u08_buff;
    *pDMA11_X_COUNT      = pChunk->bytesUsed;  // 8 bit data so we change the stride and count
    *pDMA11_X_MODIFY     = 1;
    ENABLE_DMA(*pDMA11_CONFIG);
    *pUART1_IER |= ETBEI;
    // enable transmit ISR for UART -- doubles as flow control
    // see p25-18 HRM
}

/*
 * Stop DMA transmission after being finished.
 */
void uartTx_dmaStop(void) {
	// stop DMA it might otherwise bombard us with interrupts
    DISABLE_DMA(*pDMA11_CONFIG);

    // turn off the TX ISR as we do not use the DMA anymore
    *pUART1_IER &= ~ETBEI;

    return;
}

/** uartTx_isr
 * 
 * @param pThis  pointer to uartTx
 *
 * @return void 
 */
void uartTx_isr(void *pThisArg)
{
	// create local casted pThis to avoid casting on every single access
    uartTx_t  *pThis = (uartTx_t*) pThisArg;

    chunk_t                  *pchunk              = NULL;
    // validate that TX DMA IRQ was triggered 
    if (*pDMA11_IRQ_STATUS & 0x1)
    {
        /* release old chunk on success */
       bufferPool_release(pThis->pBuffP, pThis->pPending);

       /* get new chunk from queue */
        if (PASS == queue_get(&pThis->queue, (void**)&pchunk))
        {
            /* register new chunk as pending */
            pThis->pPending = pchunk;

            // config DMA either with new chunk (if there was one)
            uartTx_dmaConfig(pThis->pPending);
        }
        else
        {
        	// queue is empty nothing more to transmit.
        	uartTx_dmaStop();

        	// set the DMA running to false. Next call is a fresh start
        	// no need to clean the DMA it is on one shot mode anyway
        	pThis->running = 0; // indicate next call will be a fresh start
        }

    	// Clear TX interrupt
        *pDMA11_IRQ_STATUS  |= 0x0001;
    }
}
