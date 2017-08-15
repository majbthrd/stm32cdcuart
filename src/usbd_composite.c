/*
    shim code to enable USB Composite devices within ST Micro's USB stack

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

#include "usbd_composite.h"
#include "usbd_desc.h" /* for USBD_CfgFSDesc_len and USBD_CfgFSDesc_pnt */
#include "usbd_cdc.h"

/* USB handle declared in main.c */
extern USBD_HandleTypeDef USBD_Device;

/* local function prototyping */

static uint8_t USBD_Composite_Init (USBD_HandleTypeDef *pdev, uint8_t cfgidx);
static uint8_t USBD_Composite_DeInit (USBD_HandleTypeDef *pdev, uint8_t cfgidx);
static uint8_t USBD_Composite_Setup (USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req);
static uint8_t USBD_Composite_DataIn (USBD_HandleTypeDef *pdev, uint8_t epnum);
static uint8_t USBD_Composite_DataOut (USBD_HandleTypeDef *pdev, uint8_t epnum);
static uint8_t USBD_Composite_EP0_TxSent (USBD_HandleTypeDef *pdev);
static uint8_t USBD_Composite_EP0_RxReady (USBD_HandleTypeDef *pdev);
static const uint8_t *USBD_Composite_GetFSCfgDesc (uint16_t *length);
static uint8_t USBD_Composite_SOF (struct _USBD_HandleTypeDef *pdev);

/* interface class callbacks structure that is used by main.c */
const USBD_ClassTypeDef USBD_Composite = 
{
  .Init                  = USBD_Composite_Init,
  .DeInit                = USBD_Composite_DeInit,
  .Setup                 = USBD_Composite_Setup,
  .EP0_TxSent            = USBD_Composite_EP0_TxSent,
  .EP0_RxReady           = USBD_Composite_EP0_RxReady,
  .DataIn                = USBD_Composite_DataIn,
  .DataOut               = USBD_Composite_DataOut,
  .SOF                   = USBD_Composite_SOF,
  .IsoINIncomplete       = NULL,
  .IsoOUTIncomplete      = NULL,     
  .GetFSConfigDescriptor = USBD_Composite_GetFSCfgDesc,    
};

struct composite_list_struct
{
  const USBD_CompClassTypeDef *pnt;
};

/*
A deficiency of the ST Micro code is that it makes no provision whatsoever to allow for USB Composite devices.
To make it possible to implement Composite devices, but yet leverage ST Micro's USB drivers, this driver acts like 
one of ST Micro's legacy code's single class drivers, but then invokes all composite members added to this array
*/

static const struct composite_list_struct composite_list[] =
{
  { &USBD_CDC }, /* in this particular code, there is only CDC */
};

static uint8_t USBD_Composite_Init (USBD_HandleTypeDef *pdev, uint8_t cfgidx)
{
  unsigned index;

  for (index = 0; index < (sizeof(composite_list) / sizeof(*composite_list)); index++)
  {
    if (composite_list[index].pnt->Init)
      composite_list[index].pnt->Init(pdev, cfgidx);
  }

  return USBD_OK;
}

static uint8_t USBD_Composite_DeInit (USBD_HandleTypeDef *pdev, uint8_t cfgidx)
{
  unsigned index;

  for (index = 0; index < (sizeof(composite_list) / sizeof(*composite_list)); index++)
  {
    if (composite_list[index].pnt->DeInit)
      composite_list[index].pnt->DeInit(pdev, cfgidx);
  }
  
  return USBD_OK;
}

static uint8_t USBD_Composite_Setup (USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req)
{
  unsigned index;

  for (index = 0; index < (sizeof(composite_list) / sizeof(*composite_list)); index++)
  {
    if (composite_list[index].pnt->Setup)
      composite_list[index].pnt->Setup(pdev, req);
  }

  return USBD_OK;
}

static uint8_t USBD_Composite_DataIn (USBD_HandleTypeDef *pdev, uint8_t epnum)
{
  unsigned index;

  for (index = 0; index < (sizeof(composite_list) / sizeof(*composite_list)); index++)
  {
    if (composite_list[index].pnt->DataIn)
      composite_list[index].pnt->DataIn(pdev, epnum);
  }

  return USBD_OK;
}

static uint8_t USBD_Composite_DataOut (USBD_HandleTypeDef *pdev, uint8_t epnum)
{      
  unsigned index;

  for (index = 0; index < (sizeof(composite_list) / sizeof(*composite_list)); index++)
  {
    if (composite_list[index].pnt->DataOut)
      composite_list[index].pnt->DataOut(pdev, epnum);
  }

  return USBD_OK;
}

static uint8_t USBD_Composite_SOF (USBD_HandleTypeDef *pdev)
{
  unsigned index;

  for (index = 0; index < (sizeof(composite_list) / sizeof(*composite_list)); index++)
  {
    if (composite_list[index].pnt->SOF)
      composite_list[index].pnt->SOF(pdev);
  }

  return USBD_OK;
}

static uint8_t USBD_Composite_EP0_TxSent (USBD_HandleTypeDef *pdev)
{ 
  unsigned index;

  for (index = 0; index < (sizeof(composite_list) / sizeof(*composite_list)); index++)
  {
    if (composite_list[index].pnt->EP0_TxSent)
      composite_list[index].pnt->EP0_TxSent(pdev);
  }

  return USBD_OK;
}

static uint8_t USBD_Composite_EP0_RxReady (USBD_HandleTypeDef *pdev)
{ 
  unsigned index;

  for (index = 0; index < (sizeof(composite_list) / sizeof(*composite_list)); index++)
  {
    if (composite_list[index].pnt->EP0_RxReady)
      composite_list[index].pnt->EP0_RxReady(pdev);
  }

  return USBD_OK;
}

static const uint8_t *USBD_Composite_GetFSCfgDesc (uint16_t *length)
{
  *length = USBD_CfgFSDesc_len;
  return USBD_CfgFSDesc_pnt;
}

void USBD_Composite_PMAConfig(PCD_HandleTypeDef *hpcd, uint32_t *pma_address)
{
  unsigned index;

  for (index = 0; index < (sizeof(composite_list) / sizeof(*composite_list)); index++)
  {
    if (composite_list[index].pnt->PMAConfig)
      composite_list[index].pnt->PMAConfig(hpcd, pma_address);
  }
}
