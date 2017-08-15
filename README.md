DMA-accelerated multi-UART USB CDC for STM32F072 microcontroller
================================================================

This USB CDC ACM implementation was written out of frustration with the [example USB CDC class to UART bridge implementation](http://www.st.com/web/en/catalog/tools/PF260612) provided by ST Micro.

This code re-writes the USB CDC implementation to hopefully overcome these limitations, but tries to not otherwise 'reinvent the wheel' where ST's existing code (V1.2.1) is serviceable as-is.

Improvements over the ST-provided example code are:
*	Data loss and lockup issues in ST's example code are hopefully all addressed
*	Both receive and transmit UART data transfers are DMA-accelerated.
*	Fixed-value variables are declared as const wherever possible to reduce RAM usage.
*	USB descriptors are written as more-maintainable structs.

Testing was done with Linux, Windows 7, and Windows 10 hosts.

## Build Requirements

One approach is to use [Rowley Crossworks for ARM](http://www.rowley.co.uk/arm/) to compile this code.  It is not free software, but has been my favorite go-to ARM development tool for a decade and counting.

*OR*

I've modified the build environment provided by [Alex Taradov's mcu-starter-projects](https://github.com/ataradov/mcu-starter-projects) to work with the STM32F072/STM32F042.  With this approach, the code can be built using only open-source software.  In Ubuntu-derived distributions, this is likely achieved with as little as:

```
sudo apt-get install gcc-arm-none-eabi libnewlib-arm-none-eabi build-essential
```

## Sanity Checklist If Customizing

The STM32F072B Discovery Kit precludes the use of UART2, as the available pins for this are mapped to incompatible devices.

The only available pins in the device used in STM32F072B Discovery Kit for UART4 share de-bouncing circuitry that artificially restricts the maximum data rate.

config.h has a NUM\_OF\_CDC\_UARTS value that is used throughout the code to control the number of CDC UARTs.

The Command and Data Interface numbers in the USB descriptor in usbd\_desc.c must be continguous and start from zero.

An understanding of USB descriptors is important when modifying usb_desc.c.  This data conveys the configuration of the device (including endpoint, etc.) to the host PC.

The UARTconfig array in stm32f0xx\_hal\_msp.c must be customized to suit the pin-mapping used in your application.  The values provided were used on the [STM32F072BDISCOVERY PCB](http://www.st.com/stm32f072discovery-pr).

The parameters array values in usbd\_cdc.c must be consistent with both the UARTconfig array in stm32f0xx\_hal\_msp.c and the Command and Data Interface numbers in the USB descriptor in usbd\_desc.c.

The DMA IRQ handlers in usbd\_cdc.c must be consistent with the UARTconfig array in stm32f0xx\_hal\_msp.c.

USB transfers are handled via a distinct section of memory called "PMA".  Read the ST documentation on this.  At most, there is 1kBytes that must be shared across all endpoints.  Consider the usage of this PMA memory when scaling up the number of UARTs and buffer sizes.

