#pragma once

void usb_serial_init(void);
int usb_serial_tx_buf_size(void);
int usb_serial_rx_buf_size(void);

int usb_serial_write(const void *src, int amount);
int usb_serial_read(void *dst, int amount);

// USB Serial Tasks
void USBSerialTx(void *pvParameters);
void USBSerialRx(void *pvParameters);
