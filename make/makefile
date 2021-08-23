# Author: strawberryhacker

project_path = $(shell pwd)/..
output_path = $(project_path)/output
output_name = bootloader

#---------------------------------------------------------------------------------------------------

source += $(project_path)/main.c
source += $(project_path)/startup.c
source += $(project_path)/print.c
source += $(project_path)/gmac.c
source += $(project_path)/time.c
source += $(project_path)/gpio.c
source += $(project_path)/clock.c
source += $(project_path)/network.c
source += $(project_path)/utilities.c

include += $(project_path)/include/utilities.h
include += $(project_path)/include/registers.h
include += $(project_path)/include/print.h
include += $(project_path)/include/gmac.h
include += $(project_path)/include/time.h
include += $(project_path)/include/gpio.h
include += $(project_path)/include/clock.h
include += $(project_path)/include/list.h
include += $(project_path)/include/network.h

dependencies += $(project_path)/make/makefile
dependencies += $(project_path)/linker/linker.ld

#---------------------------------------------------------------------------------------------------

compile_flags += -x c
compile_flags += -O3
compile_flags += -g3
compile_flags += -Wall
compile_flags += -Wno-unused-variable
compile_flags += -Wno-unused-function
compile_flags += -std=c11
compile_flags += -ffreestanding
compile_flags += -mcpu=cortex-m7
compile_flags += -mthumb
compile_flags += -I$(project_path)/include

linker_flags += -nostartfiles
linker_flags += -mthumb
linker_flags += -mcpu=cortex-m4
linker_flags += -T$(project_path)/linker/linker.ld

#---------------------------------------------------------------------------------------------------

objects = $(addprefix $(output_path), $(subst $(project_path),, $(patsubst %.c, %.o, $(source)))) 

.PHONY: main clean count network
.SECONDARY: $(objects)

#---------------------------------------------------------------------------------------------------

main: $(output_path)/$(output_name).elf $(output_path)/$(output_name).lss $(output_path)/$(output_name).bin
	@sudo edbg -t sam4e -F w,1,1
	@sudo edbg -t sam4e -pv -f $(output_path)/$(output_name).bin

clean:
	@rm -r -f $(output_path)

count: 
	@cloc -no3 --by-file *.c

network:
	@sudo ifconfig enx00e04c361496 192.168.101.1/24 up
	@sudo systemctl restart isc-dhcp-server.service
	@sudo systemctl restart tftpd-hpa

%.elf: $(objects)
	@mkdir -p $(dir $@)
	@arm-none-eabi-gcc $(linker_flags) $^ -o $@

%.lss: %.elf
	@mkdir -p $(dir $@)
	@arm-none-eabi-objdump -h -S $< > $@

%.bin: %.elf
	@mkdir -p $(dir $@)
	@arm-none-eabi-objcopy -O binary $< $@

$(output_path)/%.o: $(project_path)/%.c $(include) $(dependencies)
	@mkdir -p $(dir $@)
	@arm-none-eabi-gcc $(compile_flags) -c $< -o $@
