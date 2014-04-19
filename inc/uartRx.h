#ifndef _UART_RX_H_
#define _UART_RX_H_

#include "queue.h"
#include "bufferPool.h"
#include "isrDisp.h"

/***************************************************
* 		DEFINES
***************************************************/
/**
 * @def UARTRX_QUEUE_DEPTH
 * @brief rx queue depth
 */
#define UARTRX_QUEUE_DEPTH	7


/***************************************************
* 		DATA TYPES
***************************************************/
/** uart RX object
 */
typedef struct {
  queue_t        queue;  	/* queue for received buffers */
  chunk_t        *pPending; /* pointer to pending chunk just in receiving */
  bufferPool_t   *pBuffP; 	/* pointer to buffer pool */
} uartRx_t;


/***************************************************
* 		Access Methods
***************************************************/
/** Configure the UART DMA
 * Configures the DMA rx with the buffer and the buffer length to
 * receive
 * Parameters:
 * @param pchunk  pointer to receive chunk
 *
 * @return void
 */
void uartRx_dmaConfig(chunk_t* cPtr);

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
int uartRx_init(uartRx_t *pThis, bufferPool_t *pBuffP, isrDisp_t *pIsrDisp);

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
int uartRx_start(uartRx_t *pThis);

/** uartRx_isr

 * Parameters:
 * @param pThisArg  pointer to own object
 *
 * @return None
 */
void uartRx_isr(void *pThisArg);

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
int uartRx_get(uartRx_t *pThis, chunk_t *pChunk);

#endif
