#ifndef _UART_RX_H_
#define _UART_RX_H_

#include "queue.h"
#include "bufferPool.h"
#include "isrDisp.h"

/***************************************************
            DEFINES
***************************************************/   
/** queue depth */
#define UARTRX_QUEUE_DEPTH 7


/***************************************************
            DATA TYPES
***************************************************/

/** uart status enum
 */
typedef enum {
    UARTRX_WAITING = 0,
    UARTRX_COMPLETING
} uart_state_t;

/** uart RX object
 */
typedef struct {
  queue_t        queue;  /* queue for received buffers */
  chunk_t        *pPending; /* pointer to pending chunk just in receiving */
  bufferPool_t   *pBuffP; /* pointer to buffer pool */
  uart_state_t   state;
} uartRx_t;


/***************************************************
            Access Methods 
***************************************************/

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
int uartRx_init(uartRx_t *pThis, bufferPool_t *pBuffP,
                 isrDisp_t *pIsrDisp);

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
int uartRx_start(uartRx_t *pThis);

void uartRx_dmaConfig(char* cPtr, unsigned short len);
void uartRx_dmaStop(void);


/** uartRx_isr

 * Parameters:
 * @param pThisArg  pointer to own object
 *
 * @return None 
 */
void uartRx_isr(void *pThisArg);

/** uartRx_get
 *   copyies a filled chunk into pChunk
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

int uartRx_getNb(uartRx_t *pThis, chunk_t *pChunk);


#endif
