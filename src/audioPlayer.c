/**
 *@file audioPlayer.c
 *
 *@brief
 *  - core module for audio player
 *
 * Target:   TLL6527v1-0      
 * Compiler: VDSP++     Output format: VDSP++ "*.dxe"
 *
 * @author:  Gunar Schirner
 *           Rohan Kangralkar
 * @date	03/08/2010
 *
 * LastChange:
 * $Id: audioPlayer.c 846 2014-02-27 15:35:54Z fengshen $
 *
 *******************************************************************************/

#include <tll_common.h>
#include "audioPlayer.h"
#include <bf52xI2cMaster.h>
#include "ssm2602.h"
#include <isrDisp.h>
#include <extio.h>
#include <tll6527_core_timer.h>

//Chunk for receive path
chunk_t receiveChunk;
//Chunk for transmit path
chunk_t transmitChunk;

/**
 * @def I2C_CLK
 * @brief Configure I2C clock to run at 400KHz
  */
#define I2C_CLOCK   (400*_1KHZ)
/**
 * @def VOLUME_CHANGE_STEP
 * @brief Magnitude of change in the volume when increasing or decreasing
 */
#define VOLUME_CHANGE_STEP (4)
/**
 * @def VOLUME_MAX
 * @brief MAX volume possible is +6db refer to ssm2603 manual
 */
#define VOLUME_MAX (0x7F)
/**
 * @def VOLUME_MIN
 * @brief MIN volume possible is -73db refer to ssm2603 manual
 */
#define VOLUME_MIN (0x2F)


/** initialize audio player 
 *@param pThis  pointer to own object 
 *
 *@return 0 success, non-zero otherwise
 **/
int audioPlayer_init(audioPlayer_t *pThis)
{
    int                         status                  = 0;
    
    printf("[AP]: Init start \r\n");
    
    pThis->volume 		= VOLUME_MAX; /*default volume */
    pThis->frequency 	= SSM2602_SR_8000/2; /* default frequency, need not copy to Left and right channel*/

    /* Initialize the core timer */
    coreTimer_init();
    
    /* configure TWI interface for I2C operation */
    bf52xI2cMaster_init(0, I2C_CLOCK);
    
    /* Initialize the interrupt Dispatcher */
    status = isrDisp_init(&pThis->isrDisp);
    if ( PASS != status ) {
        return FAIL;
    }
    
    /* Initialize the SSM2602 over the I2C interface */
    /* Initialize the SSM2602 to playback audio data */
    /* Initialize sport0 to receive audio data */
    //status = ssm2602_init(&pThis->isrDisp, pThis->volume, pThis->frequency, SSM2602_TX);
    status = ssm2602_init(&pThis->isrDisp, 0x27, SSM2602_SR_16000, SSM2602_RX|SSM2602_TX);
    if (PASS != status) {
        printf("SSM2602 init failed\r\n");
        return status;
    }
    
    /* Initialize the buffer pool */
    status = bufferPool_init(&pThis->bp);
    if ( PASS != status ) {
        return FAIL;
    }
    
    /* Initialize transmit/receive chunks */
	// init local chunk
	chunk_init(&receiveChunk);
	chunk_init(&transmitChunk);

    /* Initialize the extio */
    status = extio_init(&pThis->isrDisp);
    if ( PASS != status ) {
        return FAIL;
    }

    /* Initialize the audio RX module*/
    status = audioRx_init(&pThis->rx, &pThis->bp, &pThis->isrDisp);
    if ( PASS != status) {
        return FAIL;
    }

    /* Initialize the UART TX module */
    status = uartTx_init(&pThis->uartTx, &pThis->bp, &pThis->isrDisp);
    if ( PASS != status ) {
            return FAIL;
    }

    /* Initialize the UART RX module */
	status = uartRx_init(&pThis->uartRx, &pThis->bp, &pThis->isrDisp);
	if ( PASS != status ) {
			return FAIL;
	}

    /* Initialize the audio TX module */
    status = audioTx_init(&pThis->tx, &pThis->bp, &pThis->isrDisp);
    if ( PASS != status ) {
        return FAIL;
    }

    printf("[AP]: Init complete\r\n");

    return PASS;
}




/** startup phase after initialization 
 *@param pThis  pointer to own object 
 *
 *@return 0 success, non-zero otherwise
 **/
int audioPlayer_start(audioPlayer_t *pThis)
{
    int                         status                  = 0;
    
    printf("[AP]: startup \r\n");
    
    /* Start the audio RX module */
    status = audioRx_start(&pThis->rx);
    if ( PASS != status) {
        return FAIL;
    }

	/*Start the UART TX Module */
	status = uartTx_start(&pThis->uartTx);
	if(PASS != status){
		return FAIL;
	}

	/*Start the UART RX Module */
	status = uartRx_start(&pThis->uartRx);
	if (PASS != status){
		return FAIL;
	}

    /* Start the audio TX module */
    status = audioTx_start(&pThis->tx);
	if ( PASS != status) {
        return FAIL;
    }

    return PASS;
}



/** main loop of audio player does not terminate
 *@param pThis  pointer to own object 
 *
 *@return 0 success, non-zero otherwise
 **/
void audioPlayer_run (audioPlayer_t *pThis) {

	printf("[AP]: running \r\n");

	// setup UART
	UARTStart();
    
    while(1) {

    	//testAudioLoopback(pThis);
    	testNBAudioPath(pThis);
		//UARTStart();
    	/*
    	if(PASS == audioRx_get(&pThis->rx, &transmitChunk))
    	{
    		if(PASS == uartTx_put(&pThis->uartTx, &transmitChunk))
    		{
    			if(PASS == uartRx_get(&pThis->uartRx, &receiveChunk))
    			{
    				audioTx_put(&pThis->tx, &receiveChunk);
    			}
    		}
    	}
		*/
    }
    UARTStop();
}


/** Starts the wireless communicator
 *
 * @return PASS on success, FAIL otherwise
 */
int UARTStart( void )
{
    *pPORTF_FER 	|= 0xc000;		/* set function enable register for 14 and 15 */
    *pPORTF_MUX 	|= 0x0c00;		/* set PF15to14_MUX to 2nd alt peripheral */
    *pPORTFIO_DIR 	|= 0x4000;		/* set PF14 to output */

    asm("ssync;");

    return PASS;
}


/** Stops the wireless communicator
 *
 * @return PASS on success, FAIL otherwise
 */
int UARTStop( void )
{
	/*
    bf52x_uart_settings settings = {
        .parenable = 0,
        .parity = 0,
        .rxtx_baud = BF52X_BAUD_RATE_115200
    };
	*/

    *pPORTF_FER 	&= 0x0000;		/* clear function enable register */

    asm("ssync;");

    return PASS;
}

void testNBAudioPath(audioPlayer_t *pThis)
{
	audioRx_get(&pThis->rx, &receiveChunk);
	//UARTStart();
	uartTx_put(&pThis->uartTx, &receiveChunk);
	uartRx_get(&pThis->uartRx, &transmitChunk);
	//UARTStop();
	audioTx_put(&pThis->tx, &transmitChunk);
}

void testAudioLoopBack(audioPlayer_t *pThis)
{
	if(PASS == audioRx_get(&pThis->rx, &receiveChunk))
	audioTx_put(&pThis->tx, &receiveChunk);
}


