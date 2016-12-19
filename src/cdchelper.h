/*
    USB descriptor macros for CDC

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

#ifndef __CDC_HELPER_H
#define __CDC_HELPER_H

#include <stdint.h>
#include "usbhelper.h"

struct cdc_interface
{
  struct interface_association_descriptor cdc_association;
  struct interface_descriptor             ctl_interface;
  struct cdc_functional_descriptor_header cdc_acm_header;
  struct cdc_cm_functional_descriptor     cdc_cm;
  struct cdc_acm_functional_descriptor    cdc_acm;
  struct cdc_union_functional_descriptor  cdc_union;
  struct endpoint_descriptor              ctl_ep;
  struct interface_descriptor             dat_interface;
  struct endpoint_descriptor              ep_in;
  struct endpoint_descriptor              ep_out;
};

/* macro to help generate CDC ACM USB descriptors */

#define CDC_DESCRIPTOR(COMMAND_ITF, DATA_ITF, COMMAND_EP, DATAOUT_EP, DATAIN_EP) \
    { \
      { \
        /*Interface Association Descriptor */ \
        sizeof(struct interface_association_descriptor), /* bLength: Interface Association Descriptor size */ \
        USB_DESC_TYPE_INTERFACE_ASSOCIATION,             /* bDescriptorType: Interface Association */ \
        COMMAND_ITF,                                     /* bFirstInterface: First Interface of Association */ \
        0x02,                                            /* bInterfaceCount: quantity of interfaces in association */ \
        0x02,                                            /* bFunctionClass: Communication Interface Class */ \
        0x02,                                            /* bFunctionSubClass: Abstract Control Model */ \
        0x01,                                            /* bFunctionProtocol: Common AT commands */ \
        0x00,                                            /* iInterface */ \
      }, \
 \
      { \
        /*Interface Descriptor */ \
        sizeof(struct interface_descriptor),             /* bLength: Interface Descriptor size */ \
        USB_DESC_TYPE_INTERFACE,                         /* bDescriptorType: Interface */ \
        COMMAND_ITF,                                     /* bInterfaceNumber: Number of Interface */ \
        0x00,                                            /* bAlternateSetting: Alternate setting */ \
        0x01,                                            /* bNumEndpoints: One endpoints used */ \
        0x02,                                            /* bInterfaceClass: Communication Interface Class */ \
        0x02,                                            /* bInterfaceSubClass: Abstract Control Model */ \
        0x01,                                            /* bInterfaceProtocol: Common AT commands */ \
        0x00,                                            /* iInterface */ \
      }, \
 \
      { \
        /*Header Functional Descriptor*/ \
        sizeof(struct cdc_functional_descriptor_header), /* bLength: Endpoint Descriptor size */ \
        0x24,                                            /* bDescriptorType: CS_INTERFACE */ \
        0x00,                                            /* bDescriptorSubtype: Header Func Desc */ \
        USB_UINT16(0x0110),                              /* bcdCDC: spec release number */ \
      }, \
 \
      { \
        /*Call Management Functional Descriptor*/ \
        sizeof(struct cdc_cm_functional_descriptor),     /* bFunctionLength */ \
        0x24,                                            /* bDescriptorType: CS_INTERFACE */ \
        0x01,                                            /* bDescriptorSubtype: Call Management Func Desc */ \
        0x00,                                            /* bmCapabilities: D0+D1 */ \
        DATA_ITF,                                        /* bDataInterface */ \
      }, \
 \
      { \
        /*ACM Functional Descriptor*/ \
        sizeof(struct cdc_acm_functional_descriptor),    /* bFunctionLength */ \
        0x24,                                            /* bDescriptorType: CS_INTERFACE */ \
        0x02,                                            /* bDescriptorSubtype: Abstract Control Management desc */ \
        0x02,                                            /* bmCapabilities */ \
      }, \
 \
      { \
        /*Union Functional Descriptor*/ \
        sizeof(struct cdc_union_functional_descriptor),  /* bFunctionLength */ \
        0x24,                                            /* bDescriptorType: CS_INTERFACE */ \
        0x06,                                            /* bDescriptorSubtype: Union func desc */ \
        COMMAND_ITF,                                     /* bMasterInterface: Communication class interface */ \
        DATA_ITF,                                        /* bSlaveInterface0: Data Class Interface */ \
      }, \
 \
      { \
        /* Command Endpoint Descriptor*/ \
        sizeof(struct endpoint_descriptor),              /* bLength: Endpoint Descriptor size */ \
        USB_DESC_TYPE_ENDPOINT,                          /* bDescriptorType: Endpoint */ \
        COMMAND_EP,                                      /* bEndpointAddress */ \
        0x03,                                            /* bmAttributes: Interrupt */ \
        USB_UINT16(CDC_CMD_PACKET_SIZE),                 /* wMaxPacketSize: */ \
        0x10,                                            /* bInterval: */  \
      }, \
 \
      { \
        /*Data class interface descriptor*/ \
        sizeof(struct interface_descriptor),             /* bLength: Endpoint Descriptor size */ \
        USB_DESC_TYPE_INTERFACE,                         /* bDescriptorType: */ \
        DATA_ITF,                                        /* bInterfaceNumber: Number of Interface */ \
        0x00,                                            /* bAlternateSetting: Alternate setting */ \
        0x02,                                            /* bNumEndpoints: Two endpoints used */ \
        0x0A,                                            /* bInterfaceClass: CDC */ \
        0x00,                                            /* bInterfaceSubClass: */ \
        0x00,                                            /* bInterfaceProtocol: */ \
        0x00,                                            /* iInterface: */ \
      }, \
 \
      { \
        /* Data Endpoint OUT Descriptor */ \
        sizeof(struct endpoint_descriptor),              /* bLength: Endpoint Descriptor size */ \
        USB_DESC_TYPE_ENDPOINT,                          /* bDescriptorType: Endpoint */ \
        DATAOUT_EP,                                      /* bEndpointAddress */ \
        0x02,                                            /* bmAttributes: Bulk */ \
        USB_UINT16(USB_FS_MAX_PACKET_SIZE),              /* wMaxPacketSize: */ \
        0x00,                                            /* bInterval: ignore for Bulk transfer */ \
      }, \
 \
      { \
        /* Data Endpoint IN Descriptor*/ \
        sizeof(struct endpoint_descriptor),              /* bLength: Endpoint Descriptor size */ \
        USB_DESC_TYPE_ENDPOINT,                          /* bDescriptorType: Endpoint */ \
        DATAIN_EP,                                       /* bEndpointAddress */ \
        0x02,                                            /* bmAttributes: Bulk */ \
        USB_UINT16(USB_FS_MAX_PACKET_SIZE),              /* wMaxPacketSize: */ \
        0x00                                             /* bInterval: ignore for Bulk transfer */ \
      } \
    },

#endif /* __CDC_HELPER_H */
