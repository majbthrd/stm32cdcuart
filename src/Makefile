##############################################################################
BUILD = build
BIN = stm32cdcuart

##############################################################################
.PHONY: all directory clean size

CC = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
SIZE = arm-none-eabi-size

CFLAGS += -W -Wall --std=gnu99 -Os
CFLAGS += -fno-diagnostics-show-caret
CFLAGS += -fdata-sections -ffunction-sections
CFLAGS += -funsigned-char -funsigned-bitfields
CFLAGS += -mcpu=cortex-m0 -mthumb
CFLAGS += -MD -MP -MT $(BUILD)/$(*F).o -MF $(BUILD)/$(@F).d
CFLAGS += -g3
CFLAGS += -Wno-unused-parameter -Wno-unused-function -Wno-unused-variable # suppress warnings that happen OFTEN with STM32 library code

LDFLAGS += -mcpu=cortex-m0 -mthumb
LDFLAGS += -Wl,--gc-sections
LDFLAGS += -Wl,--script=./linker/stm32f072xB.ld

INCLUDES += \
  -I./include \
  -I.

SRCS += \
  ./main.c \
  ./stm32f0xx_hal.c \
  ./stm32f0xx_hal_cortex.c \
  ./stm32f0xx_hal_dma.c \
  ./stm32f0xx_hal_gpio.c \
  ./stm32f0xx_hal_msp.c \
  ./stm32f0xx_hal_pcd.c \
  ./stm32f0xx_hal_pcd_ex.c \
  ./stm32f0xx_hal_rcc.c \
  ./stm32f0xx_hal_rcc_ex.c \
  ./stm32f0xx_hal_uart.c \
  ./stm32f0xx_hal_uart_ex.c \
  ./stm32f0xx_it.c \
  ./system_stm32f0xx.c \
  ./usbd_cdc.c \
  ./usbd_composite.c \
  ./usbd_conf.c \
  ./usbd_core.c \
  ./usbd_ctlreq.c \
  ./usbd_desc.c \
  ./usbd_ioreq.c \
  ./startup_stm32f0xx.c

DEFINES += \
  -DSTM32F072xB

CFLAGS += $(INCLUDES) $(DEFINES)

OBJS = $(addprefix $(BUILD)/, $(notdir %/$(subst .c,.o, $(SRCS))))

all: directory $(BUILD)/$(BIN).elf $(BUILD)/$(BIN).hex $(BUILD)/$(BIN).bin size

$(BUILD)/$(BIN).elf: $(OBJS)
	@echo LD $@
	@$(CC) $(LDFLAGS) $(OBJS) $(LIBS) -o $@

$(BUILD)/$(BIN).hex: $(BUILD)/$(BIN).elf
	@echo OBJCOPY $@
	@$(OBJCOPY) -O ihex $^ $@

$(BUILD)/$(BIN).bin: $(BUILD)/$(BIN).elf
	@echo OBJCOPY $@
	@$(OBJCOPY) -O binary $^ $@

%.o:
	@echo CC $@
	@$(CC) $(CFLAGS) $(filter %/$(subst .o,.c,$(notdir $@)), $(SRCS)) -c -o $@

directory:
	@mkdir -p $(BUILD)

size: $(BUILD)/$(BIN).elf
	@echo size:
	@$(SIZE) -t $^

clean:
	@echo clean
	@-rm -rf $(BUILD)

-include $(wildcard $(BUILD)/*.d)
