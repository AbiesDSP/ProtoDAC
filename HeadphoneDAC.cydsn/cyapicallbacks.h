/* ========================================
 *
 * Copyright YOUR COMPANY, THE YEAR
 * All Rights Reserved
 * UNPUBLISHED, LICENSED SOFTWARE.
 *
 * CONFIDENTIAL AND PROPRIETARY INFORMATION
 * WHICH IS THE PROPERTY OF your company.
 *
 * ========================================
*/
#ifndef CYAPICALLBACKS_H
#define CYAPICALLBACKS_H
    
#include "audio_out.h"
#include "usb.h"

#define USBFS_EP_1_ISR_ENTRY_CALLBACK
#define USBFS_EP_1_ISR_EntryCallback() usb_audio_out_ep_isr()

#define USBFS_EP_3_ISR_ENTRY_CALLBACK
#define USBFS_EP_3_ISR_EntryCallback() usb_audio_out_fb_isr()
    
#endif /* CYAPICALLBACKS_H */   
/* [] */
