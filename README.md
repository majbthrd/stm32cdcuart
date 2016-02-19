DMA-accelerated multi-UART USB CDC for STM32F072 microcontroller
================================================================

ST Micro provides an [example USB CDC class to UART bridge implementation](http://www.st.com/web/en/catalog/tools/PF260612), but I found their code to be inadequate.  There were some performance/speed issues and it was only written to support one UART.

This code re-writes the USB CDC implementation to hopefully overcome these limitations, but tries to not otherwise 'reinvent the wheel' where ST's existing code (V1.2.1) is serviceable as-is.

## Requirements

[Rowley Crossworks for ARM](http://www.rowley.co.uk/arm/) is needed to compile this code.  The source code is gcc-friendly, but you must adapt the code yourself if you wish to adopt a different tool chain.

## Sanity Checklist If Customizing

USBD\_MAX\_NUM\_INTERFACES in usbd\_conf.h must conform to the NUM\_OF\_CDC\_UARTS value in usbd\_cdc.h (nominally 2x NUM\_OF\_CDC\_UARTS since each CDC has a Command and Data interface).

The Command and Data Interface numbers in the USB descriptor in usbd\_desc.c must be continguous and start from zero.

The UARTconfig array in stm32f0xx\_hal\_msp.c must be customized to suit the pin-mapping used in your application.  The values provided were used on the [STM32F072BDISCOVERY PCB](http://www.st.com/stm32f072discovery-pr).

The parameters array values in usbd\_cdc.c must be consistent with both the UARTconfig array in stm32f0xx\_hal\_msp.c and the Command and Data Interface numbers in the USB descriptor in usbd\_desc.c.

The DMA IRQ handlers in usbd\_cdc.c must be consistent with the UARTconfig array in stm32f0xx\_hal\_msp.c.

