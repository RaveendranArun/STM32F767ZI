
CC=arm-none-eabi-gcc
OBJCOPY=arm-none-eabi-objcopy
SIZE=arm-none-eabi-size
RENODE=/Applications/Renode.app/Contents/MacOS/renode
MACH=cortex-m7

# Directories
APP_DIR          = app
DRIVERS_DIR      = drivers
BSP_DIR          = bsp
PLATFORM_DIR     = platform
CMSIS_DEVICE_DIR = ThirdParty/CMSIS/Device/ST/STM32F7xx
CMSIS_CORE_DIR   = ThirdParty/CMSIS/Core
BUILD_DIR        = build
IMAGE_DIR        = image
LINKER_SCRIPT    = $(PLATFORM_DIR)/linker/stm32_ls.ld

INCLUDES = -I$(APP_DIR)/inc \
           -I$(DRIVERS_DIR)/inc \
           -I$(BSP_DIR)/inc \
           -I$(CMSIS_DEVICE_DIR)/Include \
           -I$(CMSIS_CORE_DIR)/Include

CFLAGS  = -mcpu=$(MACH) -mthumb -mfloat-abi=soft -std=gnu11 -Wall -O0 -g3 \
          -ffreestanding -fno-builtin -DSTM32F767xx $(INCLUDES)
LDFLAGS = -mcpu=$(MACH) -mthumb -mfloat-abi=soft -T $(LINKER_SCRIPT)

# Sources: every .c in app/src, drivers/src and bsp/src is picked up automatically
COMMON_SOURCES = $(wildcard $(APP_DIR)/src/*.c) \
                 $(wildcard $(DRIVERS_DIR)/src/*.c) \
                 $(wildcard $(BSP_DIR)/src/*.c) \
                 $(PLATFORM_DIR)/startup/stm32_startup.c \
                 $(CMSIS_DEVICE_DIR)/Source/system_stm32f7xx.c

# Normal build: newlib-nano + local syscall stubs
SOURCES      = $(COMMON_SOURCES) $(PLATFORM_DIR)/syscalls/syscalls.c
# Semihosting build: rdimon provides the syscalls, printf goes to the OpenOCD console
SOURCES_SEMI = $(COMMON_SOURCES)

OBJECTS      = $(patsubst %.c,$(BUILD_DIR)/obj/%.o,$(SOURCES))
OBJECTS_SEMI = $(patsubst %.c,$(BUILD_DIR)/obj_sh/%.o,$(SOURCES_SEMI))

all: $(IMAGE_DIR)/final.bin

semi: $(IMAGE_DIR)/final_sh.bin

$(BUILD_DIR)/obj/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/obj_sh/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -DSEMIHOSTING -c $< -o $@

$(BUILD_DIR)/final.elf: $(OBJECTS) $(LINKER_SCRIPT)
	$(CC) $(LDFLAGS) --specs=nano.specs -Wl,-Map=$(BUILD_DIR)/final.map -o $@ $(OBJECTS)
	$(SIZE) $@

$(BUILD_DIR)/final_sh.elf: $(OBJECTS_SEMI) $(LINKER_SCRIPT)
	$(CC) $(LDFLAGS) --specs=rdimon.specs -Wl,-Map=$(BUILD_DIR)/final_sh.map -o $@ $(OBJECTS_SEMI)
	$(SIZE) $@

$(IMAGE_DIR)/%.bin: $(BUILD_DIR)/%.elf
	@mkdir -p $(IMAGE_DIR)
	cp -f $< $(BUILD_DIR)/$*.map $(IMAGE_DIR)/
	$(OBJCOPY) -O binary $< $@
	$(OBJCOPY) -O ihex $< $(IMAGE_DIR)/$*.hex

clean:
	rm -rf $(BUILD_DIR) $(IMAGE_DIR)

load:
	openocd -f ./tools/openocd/st_nucleo_f7.cfg

# One-shot: program, verify, reset and exit (stop any running OpenOCD first)
flash: $(IMAGE_DIR)/final.bin
	openocd -f ./tools/openocd/st_nucleo_f7.cfg -c "program $(BUILD_DIR)/final.elf verify reset exit"

# Run in the Renode emulator: USART3 output prints here, GDB server on :3333, type "quit" to exit
renode: $(IMAGE_DIR)/final.bin
	$(RENODE) --disable-gui --console -e "i @$(CURDIR)/tools/renode/nucleo_f767zi.resc; start"

.PHONY: all semi clean load flash renode
