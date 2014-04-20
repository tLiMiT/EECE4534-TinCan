#ifndef _UART_TX_H_
#define _UART_TX_H_

#include "queue.h"
#include "bufferPool.h"
#include "isrDisp.h"

/***************************************************
* 		DEFINES
***************************************************/
/**
 * @def UARTTX_QUEUE_DEPTH
 * @brief tx queue depth
 */
#define UARTTX_QUEUE_DEPTH	7


/***************************************************
* 		DATA TYPES
***************************************************/
/** uart TX object
 */
typedef struct {
  queue_t		queue;		/* queue for received buffers */
  chunk_t		*pPending; 	/* pointer to pending chunk just in receiving */
  bufferPool_t	*pBuffP; 	/* pointer to buffer pool */
  int 			running;
} uartTx_t;


/***************************************************
* 		Access Methods
***************************************************/
/** Configure the UART DMA
 * Configures the DMA tx with the buffer and the buffer length to
 * receive
 * Parameters:
 * @param pchunk  pointer to receive chunk
 *
 * @return void
 */
void uartTx_dmaConfig(chunk_t *pChunk);

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
int uartTx_init(uartTx_t *pThis, bufferPool_t *pBuffP, isrDisp_t *pIsrDisp);

/** start uart tx
 *   - empty for now
 * Parameters:
 * @param pThis  pointer to own object
 *
 * @return Zero on success.
 * Negative value on failure.
 */
int uartTx_start(uartTx_t *pThis);

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
void uartTx_isr(void *pThisArg);

/** uart tx put
 *   copies filled pChunk into the TX queue for transmission
 *    if queue is full, then chunk is dropped
 * Parameters:
 * @param pThis  pointer to own object
 *
 * @return Zero on success.
 * Negative value on failure.
 */
int uartTx_put(uartTx_t *pThis, chunk_t *pChunk);

/* uart tx dma stop
 * - empty for now
 *
 * @return
 */
void uartTx_dmaStop(void);

void uartTx_dmaStart(void);

#endif
