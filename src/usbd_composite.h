#ifndef __USB_COMPOSITE_H_
#define __USB_COMPOSITE_H_

#include  "usbd_def.h"
#include  "usbd_ioreq.h"

typedef struct
{
  uint8_t  (*Init)             (struct _USBD_HandleTypeDef *pdev , uint8_t cfgidx);
  uint8_t  (*DeInit)           (struct _USBD_HandleTypeDef *pdev , uint8_t cfgidx);
 /* Control Endpoints */
  uint8_t  (*Setup)            (struct _USBD_HandleTypeDef *pdev , USBD_SetupReqTypedef  *req);  
  uint8_t  (*EP0_TxSent)       (struct _USBD_HandleTypeDef *pdev );    
  uint8_t  (*EP0_RxReady)      (struct _USBD_HandleTypeDef *pdev );  
  /* Class Specific Endpoints */
  uint8_t  (*DataIn)           (struct _USBD_HandleTypeDef *pdev , uint8_t epnum);   
  uint8_t  (*DataOut)          (struct _USBD_HandleTypeDef *pdev , uint8_t epnum); 
  uint8_t  (*SOF)              (struct _USBD_HandleTypeDef *pdev); 
  void (*PMAConfig)            (PCD_HandleTypeDef *hpcd, uint32_t *pma_address);
} USBD_CompClassTypeDef;

/* array of callback functions invoked by USBD_RegisterClass() in main.c */
extern const USBD_ClassTypeDef USBD_Composite;

void USBD_Composite_PMAConfig(PCD_HandleTypeDef *hpcd, uint32_t *pma_address);

#endif  // __USB_CDC_H_
