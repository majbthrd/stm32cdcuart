DMA-accelerated multi-UART USB CDC for STM32F072 microcontroller
================================================================

ST Micro provides an [example USB CDC class to UART bridge implementation](http://www.st.com/web/en/catalog/tools/PF260612), but I found their code to be inadequate.  There were some performance/speed issues and it was only written to support one UART.

This code re-writes the USB CDC implementation to hopefully overcome these limitations, but tries to not otherwise 'reinvent the wheel' where ST's existing code (V1.2.1) is serviceable as-is.

