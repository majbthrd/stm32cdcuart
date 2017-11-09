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

#include "usbd_core.h"
#include "usbd_desc.h"
#include "usbd_conf.h"
#include "usbhelper.h"
#include "usbd_cdc.h"
#include "cdchelper.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define USBD_VID                      0x0483
#define USBD_PID                      0x5740
#define USBD_LANGID_STRING            0x409
#define USBD_MANUFACTURER_STRING      "Acme"
#define USBD_PRODUCT_FS_STRING        "CDC"

/* Private macro -------------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
static uint8_t *USBD_DeviceDescriptor(USBD_SpeedTypeDef speed, uint16_t *length);
static uint8_t *USBD_LangIDStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length);
static uint8_t *USBD_ManufacturerStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length);
static uint8_t *USBD_ProductStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length);
static uint8_t *USBD_SerialStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length);
static uint8_t *USBD_ConfigStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length);
static uint8_t *USBD_InterfaceStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length);
static void IntToUnicode (uint32_t value, uint8_t *pbuf, uint8_t len);

/* Private variables ---------------------------------------------------------*/
const USBD_DescriptorsTypeDef USBD_Desc =
{
  USBD_DeviceDescriptor,
  USBD_LangIDStrDescriptor, 
  USBD_ManufacturerStrDescriptor,
  USBD_ProductStrDescriptor,
  USBD_SerialStrDescriptor,
};

/* USB Standard Device Descriptor */
static const struct device_descriptor hUSBDDeviceDesc =
{
  sizeof(hUSBDDeviceDesc),    /* bLength */
  USB_DESC_TYPE_DEVICE,       /* bDescriptorType */
  USB_UINT16(0x0200),         /* bcdUSB */
  0x00,                       /* bDeviceClass */
  0x00,                       /* bDeviceSubClass */
  0x00,                       /* bDeviceProtocol */
  USB_MAX_EP0_SIZE,           /* bMaxPacketSize */
  USB_UINT16(USBD_VID),       /* idVendor */
  USB_UINT16(USBD_PID),       /* idProduct */
  USB_UINT16(0x0200),         /* bcdDevice */
  USBD_IDX_MFC_STR,           /* Index of manufacturer string */
  USBD_IDX_PRODUCT_STR,       /* Index of product string */
  USBD_IDX_SERIAL_STR,        /* Index of serial number string */
  USBD_MAX_NUM_CONFIGURATION  /* bNumConfigurations */
};

/* bespoke struct for this device; struct members are added and removed as needed */
struct configuration_1
{
  struct configuration_descriptor config;
  struct cdc_interface cdc[NUM_OF_CDC_UARTS];
};

/* fully initialize the bespoke struct as a const */
__ALIGN_BEGIN static const struct configuration_1 USBD_Composite_CfgFSDesc __ALIGN_END =
{
  {
    /*Configuration Descriptor*/
    sizeof(struct configuration_descriptor),         /* bLength */
    USB_DESC_TYPE_CONFIGURATION,                     /* bDescriptorType */
    USB_UINT16(sizeof(USBD_Composite_CfgFSDesc)),    /* wTotalLength */
    USBD_MAX_NUM_INTERFACES,                         /* bNumInterfaces */
    0x01,                                            /* bConfigurationValue */
    0x00,                                            /* iConfiguration */
    0x80,                                            /* bmAttributes */
    50,                                              /* MaxPower */
  },

  {
#if (NUM_OF_CDC_UARTS > 0)
    /* CDC1 */
    CDC_DESCRIPTOR(/* Command ITF */ 0x00, /* Data ITF */ 0x01, /* Command EP */ 0x82, /* DataOut EP */ 0x01, /* DataIn EP */ 0x81)
#endif
#if (NUM_OF_CDC_UARTS > 1)
    /* CDC2 */
    CDC_DESCRIPTOR(/* Command ITF */ 0x02, /* Data ITF */ 0x03, /* Command EP */ 0x84, /* DataOut EP */ 0x03, /* DataIn EP */ 0x83)
#endif
#if (NUM_OF_CDC_UARTS > 2)
    /* CDC3 */
    CDC_DESCRIPTOR(/* Command ITF */ 0x04, /* Data ITF */ 0x05, /* Command EP */ 0x86, /* DataOut EP */ 0x05, /* DataIn EP */ 0x85)
#endif
  },
};

/* pointer and length of configuration descriptor for main USB driver */
const uint8_t *const USBD_CfgFSDesc_pnt = (const uint8_t *)&USBD_Composite_CfgFSDesc;
const uint16_t USBD_CfgFSDesc_len = sizeof(USBD_Composite_CfgFSDesc);

/* USB Standard Device Descriptor */
static const uint8_t USBD_LangIDDesc[USB_LEN_LANGID_STR_DESC]= 
{
  USB_LEN_LANGID_STR_DESC,         
  USB_DESC_TYPE_STRING,       
  LOBYTE(USBD_LANGID_STRING),
  HIBYTE(USBD_LANGID_STRING), 
};

static uint8_t USBD_StrDesc[USBD_MAX_STR_DESC_SIZ];

/**  * @brief  Returns the device descriptor. 
  * @param  speed: Current device speed
  * @param  length: Pointer to data length variable
  * @retval Pointer to descriptor buffer
  */
static uint8_t *USBD_DeviceDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
  *length = sizeof(hUSBDDeviceDesc);
  return (uint8_t*)&hUSBDDeviceDesc;
}

/**
  * @brief  Returns the LangID string descriptor.        
  * @param  speed: Current device speed
  * @param  length: Pointer to data length variable
  * @retval Pointer to descriptor buffer
  */
static uint8_t *USBD_LangIDStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
  *length = sizeof(USBD_LangIDDesc);  
  return (uint8_t*)USBD_LangIDDesc;
}

/**
  * @brief  Returns the product string descriptor. 
  * @param  speed: Current device speed
  * @param  length: Pointer to data length variable
  * @retval Pointer to descriptor buffer
  */
static uint8_t *USBD_ProductStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
  USBD_GetString((uint8_t *)USBD_PRODUCT_FS_STRING, USBD_StrDesc, length);    
  return USBD_StrDesc;
}

/**
  * @brief  Returns the manufacturer string descriptor. 
  * @param  speed: Current device speed
  * @param  length: Pointer to data length variable
  * @retval Pointer to descriptor buffer
  */
static uint8_t *USBD_ManufacturerStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
  USBD_GetString((uint8_t *)USBD_MANUFACTURER_STRING, USBD_StrDesc, length);
  return USBD_StrDesc;
}

/**
  * @brief  Returns the serial number string descriptor.        
  * @param  speed: Current device speed
  * @param  length: Pointer to data length variable
  * @retval Pointer to descriptor buffer
  */
static uint8_t *USBD_SerialStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
  uint32_t deviceserial0, deviceserial1, deviceserial2;
  
  /*
  for some peculiar reason, ST doesn't define the unique ID registers in the HAL include files
  these registers are documented in Chapter 33 of the RM0091 Reference Manual
  */
  deviceserial0 = *(uint32_t*)(0x1FFFF7AC); /*DEVICE_ID1*/
  deviceserial1 = *(uint32_t*)(0x1FFFF7B0); /*DEVICE_ID2*/
  deviceserial2 = *(uint32_t*)(0x1FFFF7B4); /*DEVICE_ID3*/
  
  deviceserial0 += deviceserial2;
  
  USBD_StrDesc[0] = *length = 0x1A;
  USBD_StrDesc[1] = USB_DESC_TYPE_STRING;
  IntToUnicode (deviceserial0, &USBD_StrDesc[2] ,8);
  IntToUnicode (deviceserial1, &USBD_StrDesc[18] ,4);
  return USBD_StrDesc;  
}

/**
  * @brief  Convert Hex 32Bits value into char 
  * @param  value: value to convert
  * @param  pbuf: pointer to the buffer 
  * @param  len: buffer length
  * @retval None
  */
static void IntToUnicode (uint32_t value, uint8_t *pbuf, uint8_t len)
{
  uint8_t idx = 0;
  
  for( idx = 0 ; idx < len ; idx ++)
  {
    if( ((value >> 28)) < 0xA )
    {
      pbuf[ 2* idx] = (value >> 28) + '0';
    }
    else
    {
      pbuf[2* idx] = (value >> 28) + 'A' - 10; 
    }
    
    value = value << 4;
    
    pbuf[ 2* idx + 1] = 0;
  }
}
