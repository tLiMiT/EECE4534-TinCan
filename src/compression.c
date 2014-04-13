/**
 *@file compression.c
 *
 *@brief
 *  - compress data for transmission
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
#include "compression.h"

/** Configures a blank state structure
 *
 * @return Zero on success.
 * 			Negative value on failure.
 */
int compression_init(compression_t *state)
{
    printf("[INIT] Compression BIT: %d RATE: %d\n", SHIFT, SAMPLE_DIV);
    state = NULL;
    return PASS;
}

/** Takes a chunk and compresses it
 *
 * @return Zero on success.
 * 			Negative value on failure.
 */
int compressData(compression_t *pThis, chunk_t *pchunk ) {
	// implement compression here

}
