#include "comm.h"
#include "project.h"

/***************************************
*    Bootloader Variables
***************************************/
#define USBUART_BTLDR_OUT_EP_1 0x03
#define USBUART_BTLDR_IN_EP_1  0x02

static uint8  USBUART_started = 0u;

void CyBtldrCommStart(void) 
{
    /*Start USB Operation/device 0 and with 5V or 3V operation depend on Voltage Configuration in DWR */
    USBUART_Start(0u, USBUART_DWR_VDDD_OPERATION);

    /* USB component started, the correct enumeration will be checked in first Read operation */
    USBUART_started = 1u;
}

void CyBtldrCommStop(void) 
{
    USBUART_Stop();
}

void CyBtldrCommReset(void) 
{
    /* Enumeration is done, enable OUT endpoint for receive data from Host */
    USBUART_CDC_Init();
}

cystatus CyBtldrCommWrite(uint8* pData, uint16 size, uint16 *count, uint8 timeOut) CYSMALL                                                            
{
    cystatus retCode;
    uint16 timeoutMs;
	uint16 bufIndex = 0;
	uint8 transfer_Size;
	*count = 0;
    timeoutMs = ((uint16) 10u * timeOut);  /* Convert from 10mS check to number 1mS checks */
    while(bufIndex < size)
	{
		if ((size -bufIndex) >  USBUART_BTLDR_SIZEOF_READ_BUFFER)
		{
			transfer_Size = USBUART_BTLDR_SIZEOF_READ_BUFFER;
		}
		else 
		{
			transfer_Size = (size -bufIndex);
		}
		/* Enable IN transfer */
    	USBUART_LoadInEP(USBUART_BTLDR_IN_EP_1, &pData[bufIndex], transfer_Size);
		/* Wait for the master to read it. */
   		while ((USBUART_GetEPState(USBUART_BTLDR_IN_EP_1) == USBUART_IN_BUFFER_FULL) &&
           (0u != timeoutMs))
    	{
        	CyDelay(1);
        	timeoutMs--;
    	}
		if (USBUART_GetEPState(USBUART_BTLDR_IN_EP_1) == USBUART_IN_BUFFER_FULL)
    	{
        	retCode = CYRET_TIMEOUT;
			break; 
    	}
	    else
	    {
	        *count += transfer_Size;
	        retCode = CYRET_SUCCESS;
			bufIndex  += transfer_Size;
	    }
	}
		
    return(retCode);
}

cystatus CyBtldrCommRead(uint8* pData, uint16 size, uint16 *count, uint8 timeOut) CYSMALL
                                                            
{
    cystatus retCode;
    uint16 timeoutMs;

    timeoutMs = ((uint16) 10u * timeOut);  /* Convert from 10mS check to number 1mS checks */
    
    if (size > USBUART_BTLDR_SIZEOF_WRITE_BUFFER)
    {
        size = USBUART_BTLDR_SIZEOF_WRITE_BUFFER;
    }

    /* Wait on enumeration in first time */
    if (0u != USBUART_started)
    {
        /* Wait for Device to enumerate */
        while ((0u ==USBUART_GetConfiguration()) && (0u != timeoutMs))
        {
            CyDelay(1);
            timeoutMs--;
        }
        
        /* Enable first OUT, if enumeration complete */
        if (0u != USBUART_GetConfiguration())
        {
            (void) USBUART_IsConfigurationChanged();  /* Clear configuration changes state status */
            CyBtldrCommReset();
            USBUART_started = 0u;
        }
    }
    else /* Check for configuration changes, has been done by Host */
    {
        if (0u != USBUART_IsConfigurationChanged()) /* Host could send double SET_INTERFACE request or RESET */
        {
            if (0u != USBUART_GetConfiguration())   /* Init OUT endpoints when device reconfigured */
            {
                CyBtldrCommReset();
            }
        }
    }
    
    timeoutMs = ((uint16) 10u * timeOut); /* Re-arm timeout */
    
    /* Wait on next packet */
    while((USBUART_GetEPState(USBUART_BTLDR_OUT_EP_1) != USBUART_OUT_BUFFER_FULL) && \
          (0u != timeoutMs))
    {
        CyDelay(1);
        timeoutMs--;
    }

    /* OUT EP has completed */
    if (USBUART_GetEPState(USBUART_BTLDR_OUT_EP_1) == USBUART_OUT_BUFFER_FULL)
    {
        *count = USBUART_ReadOutEP(USBUART_BTLDR_OUT_EP_1, pData, size);
        retCode = CYRET_SUCCESS;
    }
    else
    {
        *count = 0u;
        retCode = CYRET_TIMEOUT;
    }
    
    return(retCode);
}
