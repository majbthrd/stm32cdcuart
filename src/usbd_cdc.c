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

#include "usbd_cdc.h"
#include "usbd_desc.h"
#include "usbd_composite.h"
#include "config.h"

/* USB handle declared in main.c */
extern USBD_HandleTypeDef USBD_Device;

/* local function prototyping */

static uint8_t USBD_CDC_Init (USBD_HandleTypeDef *pdev, uint8_t cfgidx);
static uint8_t USBD_CDC_DeInit (USBD_HandleTypeDef *pdev, uint8_t cfgidx);
static uint8_t USBD_CDC_Setup (USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req);
static uint8_t USBD_CDC_DataIn (USBD_HandleTypeDef *pdev, uint8_t epnum);
static uint8_t USBD_CDC_DataOut (USBD_HandleTypeDef *pdev, uint8_t epnum);
static uint8_t USBD_CDC_EP0_RxReady (USBD_HandleTypeDef *pdev);
static uint8_t USBD_CDC_SOF (struct _USBD_HandleTypeDef *pdev);
static void USBD_CDC_PMAConfig(PCD_HandleTypeDef *hpcd, uint32_t *pma_address);

static USBD_StatusTypeDef USBD_CDC_ReceivePacket (USBD_HandleTypeDef *pdev, unsigned index);
static USBD_StatusTypeDef USBD_CDC_TransmitPacket (USBD_HandleTypeDef *pdev, unsigned index, uint16_t offset, uint16_t length);

static int8_t CDC_Itf_Control (USBD_CDC_HandleTypeDef *hcdc, uint8_t cmd, uint8_t* pbuf, uint16_t length);
static void Error_Handler (void);
static void ComPort_Config (USBD_CDC_HandleTypeDef *hcdc, unsigned reconfig);

/* CDC interface class callbacks structure that is used by main.c */
const USBD_CompClassTypeDef USBD_CDC = 
{
  .Init                  = USBD_CDC_Init,
  .DeInit                = USBD_CDC_DeInit,
  .Setup                 = USBD_CDC_Setup,
  .EP0_TxSent            = NULL,
  .EP0_RxReady           = USBD_CDC_EP0_RxReady,
  .DataIn                = USBD_CDC_DataIn,
  .DataOut               = USBD_CDC_DataOut,
  .SOF                   = USBD_CDC_SOF,
  .PMAConfig             = USBD_CDC_PMAConfig,
};

/*
parameters for this CDC implementation
*/

/* default state of all UARTs when first initialized */
static const USBD_CDC_LineCodingTypeDef defaultLineCoding =
{
  .bitrate    = 115200, /* baud rate */
  .format     = 0x00,   /* stop bits-1 */
  .paritytype = 0x00,   /* parity - none */
  .datatype   = 0x08    /* nb. of bits 8 */
};

/* endpoint numbers and "instance" (base register address) for each UART */
static const struct
{
  USART_TypeDef *Instance;
  uint8_t data_in_ep, data_out_ep, command_ep, command_itf;
} parameters[NUM_OF_CDC_UARTS] = 
{
#if (NUM_OF_CDC_UARTS > 0)
  {
    .Instance = USART1,
    .data_in_ep  = 0x81,
    .data_out_ep = 0x01,
    .command_ep  = 0x82,
    .command_itf = 0x00,
  },
#endif
#if (NUM_OF_CDC_UARTS > 1)
  {
    .Instance = USART3,
    .data_in_ep  = 0x83,
    .data_out_ep = 0x03,
    .command_ep  = 0x84,
    .command_itf = 0x02,
  },
#endif
};

/* context for each and every UART managed by this CDC implementation */
static USBD_CDC_HandleTypeDef context[NUM_OF_CDC_UARTS];

static uint8_t USBD_CDC_Init (USBD_HandleTypeDef *pdev, uint8_t cfgidx)
{
  USBD_CDC_HandleTypeDef *hcdc = context;
  unsigned index;

  for (index = 0; index < NUM_OF_CDC_UARTS; index++,hcdc++)
  {
    /* Open EP IN */
    USBD_LL_OpenEP(pdev, parameters[index].data_in_ep, USBD_EP_TYPE_BULK, USB_FS_MAX_PACKET_SIZE);
    
    /* Open EP OUT */
    USBD_LL_OpenEP(pdev, parameters[index].data_out_ep, USBD_EP_TYPE_BULK, USB_FS_MAX_PACKET_SIZE);

    /* Open Command IN EP */
    USBD_LL_OpenEP(pdev, parameters[index].command_ep, USBD_EP_TYPE_INTR, CDC_CMD_PACKET_SIZE);
  
    /* Configure the UART peripheral */
    hcdc->InboundBufferReadIndex = 0;
    hcdc->InboundTransferInProgress = 0;
    hcdc->OutboundTransferNeedsRenewal = 0;
    hcdc->UartHandle.Instance = parameters[index].Instance;
    hcdc->LineCoding = defaultLineCoding;
    __HAL_LINKDMA(&hcdc->UartHandle, hdmatx, hcdc->hdma_tx);
    __HAL_LINKDMA(&hcdc->UartHandle, hdmarx, hcdc->hdma_rx);
    ComPort_Config(hcdc, 0);
     
    /* Prepare Out endpoint to receive next packet */
    USBD_CDC_ReceivePacket(pdev, index);
  }

  return USBD_OK;
}

static uint8_t USBD_CDC_DeInit (USBD_HandleTypeDef *pdev, uint8_t cfgidx)
{
  USBD_CDC_HandleTypeDef *hcdc = context;
  unsigned index;

  for (index = 0; index < NUM_OF_CDC_UARTS; index++,hcdc++)
  {
    /* Close EP IN */
    USBD_LL_CloseEP(pdev, parameters[index].data_in_ep);
  
    /* Close EP OUT */
    USBD_LL_CloseEP(pdev, parameters[index].data_out_ep);
  
    /* Close Command IN EP */
    USBD_LL_CloseEP(pdev, parameters[index].command_ep);

    /* DeInitialize the UART peripheral */
    if (hcdc->UartHandle.Instance)
      if(HAL_UART_DeInit(&hcdc->UartHandle) != HAL_OK)
      {
        /* Initialization Error */
        Error_Handler();
      }
  }
  
  return USBD_OK;
}

static uint8_t USBD_CDC_Setup (USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req)
{
  USBD_CDC_HandleTypeDef *hcdc = context;
  unsigned index;

  for (index = 0; index < NUM_OF_CDC_UARTS; index++,hcdc++)
  {
    if (parameters[index].command_itf != req->wIndex)
      continue;

    switch (req->bmRequest & USB_REQ_TYPE_MASK)
    {
    case USB_REQ_TYPE_CLASS :
      if (req->wLength)
      {
        if (req->bmRequest & 0x80)
        {
          CDC_Itf_Control(hcdc, req->bRequest, (uint8_t *)hcdc->SetupBuffer, req->wLength);
          USBD_CtlSendData (pdev, (uint8_t *)hcdc->SetupBuffer, req->wLength);
        }
        else
        {
          hcdc->CmdOpCode = req->bRequest;
          hcdc->CmdLength = req->wLength;
        
          USBD_CtlPrepareRx (pdev, (uint8_t *)hcdc->SetupBuffer, req->wLength);
        }
      }
      else
      {
          CDC_Itf_Control(hcdc, req->bRequest, NULL, 0);
      }
      break;
 
    default: 
      break;
    }

    break;
  }

  return USBD_OK;
}

static uint8_t USBD_CDC_DataIn (USBD_HandleTypeDef *pdev, uint8_t epnum)
{
  USBD_CDC_HandleTypeDef *hcdc = context;
  unsigned index;

  for (index = 0; index < NUM_OF_CDC_UARTS; index++,hcdc++)
  {
    if (parameters[index].data_in_ep == (epnum | 0x80))
    {
      hcdc->InboundTransferInProgress = 0;
      break;
    }
  }

  return USBD_OK;
}

static uint8_t USBD_CDC_DataOut (USBD_HandleTypeDef *pdev, uint8_t epnum)
{      
  USBD_CDC_HandleTypeDef *hcdc = context;
  uint32_t RxLength;
  unsigned index;

  for (index = 0; index < NUM_OF_CDC_UARTS; index++,hcdc++)
  {
    if (parameters[index].data_out_ep == epnum)
    {
      /* Get the received data length */
      RxLength = USBD_LL_GetRxDataSize (pdev, epnum);

      /* hand the data to the HAL */
      HAL_UART_Transmit_DMA(&hcdc->UartHandle, (uint8_t *)hcdc->OutboundBuffer, RxLength);

      break;
    }
  }

  return USBD_OK;
}

static uint8_t USBD_CDC_SOF (struct _USBD_HandleTypeDef *pdev)
{
  uint32_t buffsize, write_index;
  USBD_CDC_HandleTypeDef *hcdc = context;
  unsigned index;

  for (index = 0; index < NUM_OF_CDC_UARTS; index++,hcdc++)
  {
    write_index = INBOUND_BUFFER_SIZE - hcdc->hdma_rx.Instance->CNDTR;

    /* the circular DMA should reset CNDTR when it reaches zero, but just in case it is briefly zero, we fix the value */
    if (INBOUND_BUFFER_SIZE == write_index)
      write_index = 0;

    if(hcdc->InboundBufferReadIndex != write_index)
    {
      if(hcdc->InboundBufferReadIndex > write_index)
      {
        /* write index has looped around, so send partial data from the write index to the end of the buffer */
        buffsize = INBOUND_BUFFER_SIZE - hcdc->InboundBufferReadIndex;
      }
      else 
      {
        /* send all data between read index and write index */
        buffsize = write_index - hcdc->InboundBufferReadIndex;
      }

      if(USBD_CDC_TransmitPacket(pdev, index, hcdc->InboundBufferReadIndex, buffsize) == USBD_OK)
      {
        hcdc->InboundBufferReadIndex += buffsize;
        /* if we've reached the end of the buffer, loop around to the beginning */
        if (hcdc->InboundBufferReadIndex == INBOUND_BUFFER_SIZE)
        {
          hcdc->InboundBufferReadIndex = 0;
        }
      }
    }

    if (hcdc->OutboundTransferNeedsRenewal) /* if there is a lingering request needed due to a HAL_BUSY, retry it */
      USBD_CDC_ReceivePacket(pdev, index);
  }

  return USBD_OK;
}

static uint8_t USBD_CDC_EP0_RxReady (USBD_HandleTypeDef *pdev)
{ 
  USBD_CDC_HandleTypeDef *hcdc = context;
  unsigned index;

  for (index = 0; index < NUM_OF_CDC_UARTS; index++,hcdc++)
  {
    if (parameters[index].command_itf != pdev->request.wIndex)
      continue;

    if (hcdc->CmdOpCode != 0xFF)
    {
      CDC_Itf_Control(hcdc, hcdc->CmdOpCode, (uint8_t *)hcdc->SetupBuffer, hcdc->CmdLength);
      hcdc->CmdOpCode = 0xFF; 
    }

    break;
  }

  return USBD_OK;
}

static uint8_t USBD_CDC_TransmitPacket(USBD_HandleTypeDef *pdev, unsigned index, uint16_t offset, uint16_t length)
{      
  USBD_StatusTypeDef outcome;

  if (context[index].InboundTransferInProgress)
    return USBD_BUSY;

  /* Transmit next packet */
  outcome = USBD_LL_Transmit(pdev, parameters[index].data_in_ep, (uint8_t *)(context[index].InboundBuffer) + offset, length);
  
  if (USBD_OK == outcome)
  {
    /* Tx Transfer in progress */
    context[index].InboundTransferInProgress = 1;
  }

  return outcome;
}

static uint8_t USBD_CDC_ReceivePacket(USBD_HandleTypeDef *pdev, unsigned index)
{
  USBD_StatusTypeDef outcome;

  outcome = USBD_LL_PrepareReceive(pdev, parameters[index].data_out_ep, (uint8_t *)context[index].OutboundBuffer, CDC_DATA_OUT_MAX_PACKET_SIZE);

  context[index].OutboundTransferNeedsRenewal = (USBD_OK != outcome); /* set if the HAL was busy so that we know to retry it */

  return outcome;
}

static int8_t CDC_Itf_Control (USBD_CDC_HandleTypeDef *hcdc, uint8_t cmd, uint8_t* pbuf, uint16_t length)
{ 
  switch (cmd)
  {
  case CDC_SEND_ENCAPSULATED_COMMAND:
    /* Add your code here */
    break;

  case CDC_GET_ENCAPSULATED_RESPONSE:
    /* Add your code here */
    break;

  case CDC_SET_COMM_FEATURE:
    /* Add your code here */
    break;

  case CDC_GET_COMM_FEATURE:
    /* Add your code here */
    break;

  case CDC_CLEAR_COMM_FEATURE:
    /* Add your code here */
    break;

  case CDC_SET_LINE_CODING:
    hcdc->LineCoding.bitrate    = (uint32_t)(pbuf[0] | (pbuf[1] << 8) | (pbuf[2] << 16) | (pbuf[3] << 24));
    hcdc->LineCoding.format     = pbuf[4];
    hcdc->LineCoding.paritytype = pbuf[5];
    hcdc->LineCoding.datatype   = pbuf[6];
    
    /* Set the new configuration */
    ComPort_Config(hcdc, 1);
    break;

  case CDC_GET_LINE_CODING:
    pbuf[0] = (uint8_t)(hcdc->LineCoding.bitrate);
    pbuf[1] = (uint8_t)(hcdc->LineCoding.bitrate >> 8);
    pbuf[2] = (uint8_t)(hcdc->LineCoding.bitrate >> 16);
    pbuf[3] = (uint8_t)(hcdc->LineCoding.bitrate >> 24);
    pbuf[4] = hcdc->LineCoding.format;
    pbuf[5] = hcdc->LineCoding.paritytype;
    pbuf[6] = hcdc->LineCoding.datatype;     
    
    /* Add your code here */
    break;

  case CDC_SET_CONTROL_LINE_STATE:
    /* Add your code here */
    break;

  case CDC_SEND_BREAK:
    /* Add your code here */
    break;    
    
  default:
    break;
  }
  
  return USBD_OK;
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
  USBD_CDC_HandleTypeDef *hcdc = context;
  unsigned index;

  for (index = 0; index < NUM_OF_CDC_UARTS; index++,hcdc++)
  {
    if (&hcdc->UartHandle != huart)
      continue;

    /* Initiate next USB packet transfer once UART completes transfer (transmitting data over Tx line) */
    USBD_CDC_ReceivePacket(&USBD_Device, index);

    break;
  }
}

static void ComPort_Config(USBD_CDC_HandleTypeDef *hcdc, unsigned reconfig)
{
  if (reconfig)
    if (HAL_UART_DeInit(&hcdc->UartHandle) != HAL_OK)
    {
      /* Initialization Error */
      Error_Handler();
    }
  
  /* set the Stop bit */
  switch (hcdc->LineCoding.format)
  {
  case 0:
    hcdc->UartHandle.Init.StopBits = UART_STOPBITS_1;
    break;
  case 2:
    hcdc->UartHandle.Init.StopBits = UART_STOPBITS_2;
    break;
  default:
    hcdc->UartHandle.Init.StopBits = UART_STOPBITS_1;
    break;
  }
  
  /* set the parity bit*/
  switch (hcdc->LineCoding.paritytype)
  {
  case 0:
    hcdc->UartHandle.Init.Parity = UART_PARITY_NONE;
    break;
  case 1:
    hcdc->UartHandle.Init.Parity = UART_PARITY_ODD;
    break;
  case 2:
    hcdc->UartHandle.Init.Parity = UART_PARITY_EVEN;
    break;
  default:
    hcdc->UartHandle.Init.Parity = UART_PARITY_NONE;
    break;
  }
  
  /*set the data type : only 8bits and 9bits is supported */
  switch (hcdc->LineCoding.datatype)
  {
  case 0x07:
    /* With this configuration a parity (Even or Odd) must be set */
    hcdc->UartHandle.Init.WordLength = UART_WORDLENGTH_8B;
    break;
  case 0x08:
    if(hcdc->UartHandle.Init.Parity == UART_PARITY_NONE)
    {
      hcdc->UartHandle.Init.WordLength = UART_WORDLENGTH_8B;
    }
    else 
    {
      hcdc->UartHandle.Init.WordLength = UART_WORDLENGTH_9B;
    }
    
    break;
  default:
    hcdc->UartHandle.Init.WordLength = UART_WORDLENGTH_8B;
    break;
  }
  
  hcdc->UartHandle.Init.BaudRate = hcdc->LineCoding.bitrate;
  hcdc->UartHandle.Init.HwFlowCtl  = UART_HWCONTROL_NONE;
  hcdc->UartHandle.Init.Mode       = UART_MODE_TX_RX;
  
  if(HAL_UART_Init(&hcdc->UartHandle) != HAL_OK)
  {
    /* Initialization Error */
    Error_Handler();
  }

  /* Start reception */
  HAL_UART_Receive_DMA(&hcdc->UartHandle, (uint8_t *)(hcdc->InboundBuffer), INBOUND_BUFFER_SIZE);
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *UartHandle)
{
  /* Transfer error occurred in reception and/or transmission process */
  Error_Handler();
}

static void Error_Handler(void)
{
  __BKPT();
}

static void USBD_CDC_PMAConfig(PCD_HandleTypeDef *hpcd, uint32_t *pma_address)
{
  unsigned index;

  /* allocate PMA memory for all endpoints associated with CDC */
  for (index = 0; index < NUM_OF_CDC_UARTS; index++)
  {
    HAL_PCDEx_PMAConfig(hpcd, parameters[index].data_in_ep,  PCD_SNG_BUF, *pma_address);
    *pma_address += CDC_DATA_IN_MAX_PACKET_SIZE;
    HAL_PCDEx_PMAConfig(hpcd, parameters[index].data_out_ep, PCD_SNG_BUF, *pma_address);
    *pma_address += CDC_DATA_OUT_MAX_PACKET_SIZE;
    HAL_PCDEx_PMAConfig(hpcd, parameters[index].command_ep,  PCD_SNG_BUF, *pma_address);
    *pma_address += CDC_CMD_PACKET_SIZE;
  }
}

void DMA1_Channel2_3_IRQHandler(void)
{
  /* FIXME: the array index is manually coded */
#if (NUM_OF_CDC_UARTS > 0)
  HAL_DMA_IRQHandler(context[0].UartHandle.hdmatx);
  HAL_DMA_IRQHandler(context[0].UartHandle.hdmarx);
#endif
}

void DMA1_Channel4_5_6_7_IRQHandler(void)
{
  /* FIXME: the array index is manually coded */
#if (NUM_OF_CDC_UARTS > 1)
  HAL_DMA_IRQHandler(context[1].UartHandle.hdmatx);
  HAL_DMA_IRQHandler(context[1].UartHandle.hdmarx);
#endif
}
