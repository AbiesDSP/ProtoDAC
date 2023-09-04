#include "comm.h"
#include "project.h"

#define USB_SERIAL_DATA_IFACE 4
#define USB_SERIAL_TX_EP 7
#define USB_SERIAL_RX_EP 8

static uint8  USBUART_started = 0u;

static uint8_t tx_ep_buf[64];
static uint8_t rx_ep_buf[64];

void CyBtldrCommStart(void) 
{
    /*Start USB Operation/device 0 and with 5V or 3V operation depend on Voltage Configuration in DWR */
    USBFS_Start(0u, USBFS_DWR_VDDD_OPERATION);

    /* USB component started, the correct enumeration will be checked in first Read operation */
    USBUART_started = 1u;
    USBFS_CDC_Init();
}

void CyBtldrCommStop(void) 
{
    USBFS_Stop();
}

void CyBtldrCommReset(void) 
{
    /* Enumeration is done, enable OUT endpoint for receive data from Host */
    USBFS_CDC_Init();
    USBFS_LoadInEP(USB_SERIAL_TX_EP, tx_ep_buf, 64);
    USBFS_ReadOutEP(USB_SERIAL_RX_EP, rx_ep_buf, 64);
    USBFS_EnableOutEP(USB_SERIAL_RX_EP);
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
		if ((size -bufIndex) >  USBFS_BTLDR_SIZEOF_READ_BUFFER)
		{
			transfer_Size = USBFS_BTLDR_SIZEOF_READ_BUFFER;
		}
		else 
		{
			transfer_Size = (size -bufIndex);
		}
		/* Enable IN transfer */
        memcpy(tx_ep_buf, &pData[bufIndex], transfer_Size);
    	USBFS_LoadInEP(USB_SERIAL_TX_EP, NULL, transfer_Size);
		/* Wait for the master to read it. */
   		while ((USBFS_GetEPState(USB_SERIAL_TX_EP) == USBFS_IN_BUFFER_FULL) &&
           (0u != timeoutMs))
    	{
        	CyDelay(1);
        	timeoutMs--;
    	}
		if (USBFS_GetEPState(USB_SERIAL_TX_EP) == USBFS_IN_BUFFER_FULL)
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
    
    if (size > USBFS_BTLDR_SIZEOF_WRITE_BUFFER)
    {
        size = USBFS_BTLDR_SIZEOF_WRITE_BUFFER;
    }

    /* Wait on enumeration in first time */
    if (0u != USBUART_started)
    {
        /* Wait for Device to enumerate */
        while ((0u ==USBFS_GetConfiguration()) && (0u != timeoutMs))
        {
            CyDelay(1);
            timeoutMs--;
        }
        
        /* Enable first OUT, if enumeration complete */
        if (0u != USBFS_GetConfiguration())
        {
            (void) USBFS_IsConfigurationChanged();  /* Clear configuration changes state status */
            CyBtldrCommReset();
            USBUART_started = 0u;
        }
    }
    else /* Check for configuration changes, has been done by Host */
    {
        if (0u != USBFS_IsConfigurationChanged()) /* Host could send double SET_INTERFACE request or RESET */
        {
            if (0u != USBFS_GetConfiguration())   /* Init OUT endpoints when device reconfigured */
            {
                CyBtldrCommReset();
                USBFS_LoadInEP(USB_SERIAL_TX_EP, tx_ep_buf, 64);
                USBFS_ReadOutEP(USB_SERIAL_RX_EP, rx_ep_buf, 64);
                USBFS_EnableOutEP(USB_SERIAL_RX_EP);
            }
        }
    }
    
    timeoutMs = ((uint16) 10u * timeOut); /* Re-arm timeout */
    
    /* Wait on next packet */
    while((USBFS_GetEPState(USB_SERIAL_RX_EP) != USBFS_OUT_BUFFER_FULL) && \
          (0u != timeoutMs))
    {
        CyDelay(1);
        timeoutMs--;
    }

    /* OUT EP has completed */
    if (USBFS_GetEPState(USB_SERIAL_RX_EP) == USBFS_OUT_BUFFER_FULL)
    {
        *count = USBFS_GetEPCount(USB_SERIAL_RX_EP);
        memcpy(pData, rx_ep_buf, *count);
        USBFS_EnableOutEP(USB_SERIAL_RX_EP);
        retCode = CYRET_SUCCESS;
    }
    else
    {
        *count = 0u;
        retCode = CYRET_TIMEOUT;
    }
    
    return(retCode);
}
