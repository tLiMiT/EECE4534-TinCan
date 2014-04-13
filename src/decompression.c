/**
 *@file decompression.c
 *
 *@brief
 *  - decompress received data
 *
 * Target:   TLL6527v1-0
 * Compiler:
 *
 * @author Tim Liming
 * 		   Mark Hatch
 *
 *******************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include "chunk.h"
#include "decompression.h"

/** Configures a blank state structure
 *
 * @return Zero on success.
 * 			Negative value on failure.
 */
int decompression_init(decompression_t *state)
{
    printf("[INIT] Decompression BIT: %d RATE: %d\n", SHIFT, SAMPLE_DIV);
    state = NULL;
    return PASS;
}

/** Receive a chunk and decompress it
 *
 * @return Zero on success.
 * 			Negative value on failure.
 */
int decompressData(decompression_t *pThis, chunk_t *pchunk ) {
	// implement decompression here

}
