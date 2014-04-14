/******************************************************************************
 *@file: uartTx.h
 *
 *@brief: 
 *  - receive data using uart
 *
 * Target:   TLL6527v1-0      
 * Compiler: VDSP++     Output format: VDSP++ "*.dxe"
 *
 * @author:  James Kirk, Joao Mendes
 *
 *****************************************************************************/
#ifndef _UART_TX_H_
#define _UART_TX_H_

#include "queue.h"
#include "bufferPool.h"
#include "isrDisp.h"

/***************************************************
            DEFINES
***************************************************/   
/** queue depth */
#define UARTTX_QUEUE_DEPTH 7



/***************************************************
            DATA TYPES
***************************************************/

/** uart TX object
 */
typedef struct {
  queue_t       queue;     // queue for buffers waiting in line to go to the Xbee  
  chunk_t       *pPending; // pointer to chunk that's to be processed next
  bufferPool_t  *pBuffP;   // pointer to buffer pool that will go to the XBee
  int           running;   // DMA is Running
} uartTx_t;


/***************************************************
            Access Methods 
***************************************************/

/** Initialization of uartTx
 *
 * @param pThis  pointer to uartTx
 *
 * @return Zero on success, negative otherwise 
 */
int uartTx_init(uartTx_t *pThis, bufferPool_t *pBuffP, isrDisp_t *pIsrDisp);

/** uartTx_put
 *
 * @param pThis   pointer to uartTx
 * @param pChunk  pointer to data chunk
 to be eventually placed on buffer
 * @return Zero on success, negative otherwise 
 */
int uartTx_send(uartTx_t *pThis, chunk_t *pChunk);

/** uartTx_isr
 * 
 * @param pThis  pointer to uartTx
 *
 * @return void 
 */
void uartTx_isr(void *pThisArg);


/** uartTx_dmaConfig
 * 
 * @param pchunk  pointer to receive chunk
 *
 * @return void
 */
void uartTx_dmaConfig(chunk_t *pchunk);

 



#endif
