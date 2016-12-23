/*
    DMA-accelerated multi-UART USB CDC for STM32F072 microcontroller

    Copyright (C) 2015,2016 Peter Lawrence

    Permission is hereby granted, free of charge, to any person obtaining a 
    copy of this software and associated documentation files (the "Software"), 
    to deal in the Software without restriction, including without limitation 
    the rights to use, copy, modify, merge, publish, distribute, sublicense, 
    and/or sell copies of the Software, and to permit persons to whom the 
    Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in 
    all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
    THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
    DEALINGS IN THE SOFTWARE.
*/

#ifndef __USB_CDC_H_
#define __USB_CDC_H_

#include "usbd_def.h"
#include "usbd_ioreq.h"
#include "usbd_composite.h"
#include "config.h"

#define CDC_DATA_OUT_MAX_PACKET_SIZE        USB_FS_MAX_PACKET_SIZE /* don't exceed USB_FS_MAX_PACKET_SIZE; Linux data loss happens otherwise */
#define CDC_DATA_IN_MAX_PACKET_SIZE         256
#define CDC_CMD_PACKET_SIZE                 8 /* this may need to be enlarged for advanced CDC commands */

/*
INBOUND_BUFFER_SIZE should be 2x or more (bigger is better) of CDC_DATA_IN_MAX_PACKET_SIZE to ensure 
adequate time for the service routine to copy the data to the relevant USB IN endpoint PMA memory
*/
#define INBOUND_BUFFER_SIZE                 (4*CDC_DATA_IN_MAX_PACKET_SIZE)

/* listing CDC commands handled by switch statement in usbd_cdc.c */
#define CDC_SEND_ENCAPSULATED_COMMAND       0x00
#define CDC_GET_ENCAPSULATED_RESPONSE       0x01
#define CDC_SET_COMM_FEATURE                0x02
#define CDC_GET_COMM_FEATURE                0x03
#define CDC_CLEAR_COMM_FEATURE              0x04
#define CDC_SET_LINE_CODING                 0x20
#define CDC_GET_LINE_CODING                 0x21
#define CDC_SET_CONTROL_LINE_STATE          0x22
#define CDC_SEND_BREAK                      0x23

/* struct type used to store current line coding state */
typedef struct
{
  uint32_t bitrate;
  uint8_t  format;
  uint8_t  paritytype;
  uint8_t  datatype;
} USBD_CDC_LineCodingTypeDef;

/* struct type used for each instance of a CDC UART */
typedef struct
{
  /*
  ST's example code partially used 32-bit alignment of individual buffers with no explanation as to why;
  word alignment is relevant for DMA, so this practice was used (albeit in a more consistent manner) in this struct
  */
  uint32_t                   SetupBuffer[(CDC_CMD_PACKET_SIZE)/sizeof(uint32_t)];
  uint32_t                   OutboundBuffer[(CDC_DATA_OUT_MAX_PACKET_SIZE)/sizeof(uint32_t)];
  uint32_t                   InboundBuffer[(INBOUND_BUFFER_SIZE)/sizeof(uint32_t)];
  uint8_t                    CmdOpCode;
  uint8_t                    CmdLength;
  uint32_t                   InboundBufferReadIndex;
  volatile uint32_t          InboundTransferInProgress;
  volatile uint32_t          OutboundTransferNeedsRenewal;
  UART_HandleTypeDef         UartHandle;
  USBD_CDC_LineCodingTypeDef LineCoding;
  DMA_HandleTypeDef          hdma_tx;
  DMA_HandleTypeDef          hdma_rx;
} USBD_CDC_HandleTypeDef;

extern const USBD_CompClassTypeDef USBD_CDC;

#endif  // __USB_CDC_H_
