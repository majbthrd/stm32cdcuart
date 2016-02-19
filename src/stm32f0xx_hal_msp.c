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

#include "usbd_def.h"

/* Private typedef -----------------------------------------------------------*/
typedef void (*do_function)(void);
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
static void enable_gpio_a(void) { __GPIOA_CLK_ENABLE(); }
static void enable_gpio_b(void) { __GPIOB_CLK_ENABLE(); }
static void enable_gpio_c(void) { __GPIOC_CLK_ENABLE(); }
static void enable_gpio_d(void) { __GPIOD_CLK_ENABLE(); }
static void enable_usart1(void) { __USART1_CLK_ENABLE(); }
static void enable_usart2(void) { __USART2_CLK_ENABLE(); }
static void enable_usart3(void) { __USART3_CLK_ENABLE(); }
static void enable_usart4(void) { __USART4_CLK_ENABLE(); }
static void release_usart1(void) { __USART1_FORCE_RESET(); __USART1_RELEASE_RESET(); }
static void release_usart2(void) { __USART2_FORCE_RESET(); __USART2_RELEASE_RESET(); }
static void release_usart3(void) { __USART3_FORCE_RESET(); __USART3_RELEASE_RESET(); }
static void release_usart4(void) { __USART4_FORCE_RESET(); __USART4_RELEASE_RESET(); }
/* Private variables ---------------------------------------------------------*/

static const struct
{
  USART_TypeDef        *Instance;
  do_function         enable_usart;
  do_function         release_usart;
  do_function         enable_rx;
  GPIO_TypeDef        *gpio_rx;
  uint32_t            pin_rx;
  uint32_t            af_rx;
  do_function         enable_tx;
  GPIO_TypeDef        *gpio_tx;
  uint32_t            pin_tx;
  uint32_t            af_tx;
  DMA_Channel_TypeDef *tx_channel;
  DMA_Channel_TypeDef *rx_channel;
  IRQn_Type           IRQn;
} UARTconfig[] = /* pin assignments for UARTs */
{
  {
    USART1, enable_usart1, release_usart1, 
    enable_gpio_a, GPIOA, GPIO_PIN_10, GPIO_AF1_USART1, /* RX pin */
    enable_gpio_a, GPIOA, GPIO_PIN_9, GPIO_AF1_USART1,  /* TX pin */
    DMA1_Channel2, DMA1_Channel3, DMA1_Channel2_3_IRQn
  },
  {
    USART3, enable_usart3, release_usart3, 
    enable_gpio_c, GPIOC, GPIO_PIN_5, GPIO_AF1_USART3,  /* RX pin */ 
    enable_gpio_c, GPIOC, GPIO_PIN_4, GPIO_AF1_USART3,  /* TX pin */
    DMA1_Channel7, DMA1_Channel6, DMA1_Channel4_5_6_7_IRQn
  },
};

void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
  GPIO_InitTypeDef  GPIO_InitStruct;
  unsigned index;

  __DMA1_CLK_ENABLE();

  for (index = 0; index < (sizeof(UARTconfig) / sizeof(*UARTconfig)); index++)
  {
    if (UARTconfig[index].Instance != huart->Instance)
      continue;

    /* Enable GPIO clock */
    UARTconfig[index].enable_tx();
    UARTconfig[index].enable_rx();

    /* Enable USARTx clock */
    UARTconfig[index].enable_usart();

    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_PULLUP;
    GPIO_InitStruct.Speed     = GPIO_SPEED_HIGH;

    /* UART TX GPIO pin configuration  */
    GPIO_InitStruct.Pin       = UARTconfig[index].pin_tx;
    GPIO_InitStruct.Alternate = UARTconfig[index].af_tx;
    HAL_GPIO_Init(UARTconfig[index].gpio_tx, &GPIO_InitStruct);

    /* UART RX GPIO pin configuration  */
    GPIO_InitStruct.Pin       = UARTconfig[index].pin_rx;
    GPIO_InitStruct.Alternate = UARTconfig[index].af_rx;
    HAL_GPIO_Init(UARTconfig[index].gpio_rx, &GPIO_InitStruct);

    huart->hdmatx->Instance                 = UARTconfig[index].tx_channel;
    huart->hdmatx->Init.Direction           = DMA_MEMORY_TO_PERIPH;
    huart->hdmatx->Init.PeriphInc           = DMA_PINC_DISABLE;
    huart->hdmatx->Init.MemInc              = DMA_MINC_ENABLE;
    huart->hdmatx->Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    huart->hdmatx->Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
    huart->hdmatx->Init.Mode                = DMA_NORMAL;
    huart->hdmatx->Init.Priority            = DMA_PRIORITY_LOW;

    HAL_DMA_Init(huart->hdmatx);

    huart->hdmarx->Instance                 = UARTconfig[index].rx_channel;
    huart->hdmarx->Init.Direction           = DMA_PERIPH_TO_MEMORY;
    huart->hdmarx->Init.PeriphInc           = DMA_PINC_DISABLE;
    huart->hdmarx->Init.MemInc              = DMA_MINC_ENABLE;
    huart->hdmarx->Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    huart->hdmarx->Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
    huart->hdmarx->Init.Mode                = DMA_CIRCULAR;
    huart->hdmarx->Init.Priority            = DMA_PRIORITY_LOW;

    HAL_DMA_Init(huart->hdmarx);

    /* NVIC configuration for DMA transfer complete interrupt */
    HAL_NVIC_SetPriority(UARTconfig[index].IRQn, 5 /* hard-coded: customize if needed */, 0);
    HAL_NVIC_EnableIRQ(UARTconfig[index].IRQn);
  }
}

void HAL_UART_MspDeInit(UART_HandleTypeDef *huart)
{
  unsigned index;

  for (index = 0; index < (sizeof(UARTconfig) / sizeof(*UARTconfig)); index++)
  {
    if (UARTconfig[index].Instance != huart->Instance)
      continue;

    UARTconfig[index].release_usart();

    HAL_GPIO_DeInit(UARTconfig[index].gpio_tx, UARTconfig[index].pin_tx);
    HAL_GPIO_DeInit(UARTconfig[index].gpio_rx, UARTconfig[index].pin_rx);
  }
}
