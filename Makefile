# Makefile Bare-Metal cho STM32F746G-DISCO
TARGET = tft_video_f7

TOOLCHAIN_PATH = D:/STM32CubeIDE_1.19.0/STM32CubeIDE/plugins/com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32.13.3.rel1.win32_1.0.0.202411081344/tools/bin/
PREFIX = $(TOOLCHAIN_PATH)arm-none-eabi-

CC      = $(PREFIX)gcc
AS      = $(PREFIX)gcc -x assembler-with-cpp
CP      = $(PREFIX)objcopy
SZ      = $(PREFIX)size
HEX     = $(CP) -O ihex
BIN     = $(CP) -O binary -S

CPU     = -mcpu=cortex-m7
FPU     = -mfpu=fpv5-sp-d16
FLOAT   = -mfloat-abi=hard
MCU     = $(CPU) -mthumb $(FPU) $(FLOAT)

INCLUDES = -IInc
CFLAGS  = $(MCU) $(INCLUDES) -O2 -Wall -fdata-sections -ffunction-sections

LDSCRIPT = STM32F746NGHX_FLASH.ld
LDFLAGS = $(MCU) -T$(LDSCRIPT) -Wl,-Map=$(TARGET).map,--cref -Wl,--gc-sections

C_SOURCES = \
Src/main.c \
Src/sys_clock.c \
Src/sdram.c \
Src/ltdc.c \
Src/dma2d.c \
Src/sdmmc.c \
Src/media_player.c \
Src/diskio.c \
Src/ff.c

ASM_SOURCES = \
Startup/startup_stm32f746nghx.s

OBJS = $(C_SOURCES:.c=.o) $(ASM_SOURCES:.s=.o)

all: $(TARGET).elf $(TARGET).bin $(TARGET).hex

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

%.o: %.s
	$(AS) -c $(CFLAGS) $< -o $@

$(TARGET).elf: $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) -o $@
	$(SZ) $@

$(TARGET).bin: $(TARGET).elf
	$(BIN) $< $@

$(TARGET).hex: $(TARGET).elf
	$(HEX) $< $@

clean:
	rm -f $(OBJS) $(TARGET).elf $(TARGET).bin $(TARGET).hex $(TARGET).map

flash: $(TARGET).hex
	"C:/Program Files (x86)/STMicroelectronics/STM32 ST-LINK Utility/ST-LINK Utility/ST-LINK_CLI.exe" -c -P $(TARGET).hex 0x08000000 -V -Rst

.PHONY: all clean flash
