#####################################################################################################
# BIOS-PROGRAM - ACPI-Shutdown-in-32bit-Mode
#####################################################################################################
#====================================================================================================
# Configuration & Variable Definition
#====================================================================================================
C_COMPILER = gcc
C_FLAGS = -m32 -ffreestanding -fno-pie -fno-stack-protector -nostdlib -Wall -Wextra
C_OBJECTS = kernel.o font8x8.o timer.o graphic.o acpi.o keyboard.o
ASM_COMPILER = nasm
LINKER = ld
LINKER_FLAGS = -m elf_i386 -T linker.ld --nmagic
#====================================================================================================
# Build Rule
#====================================================================================================
all: program.img
bootloader.bin: bootloader.asm
	$(ASM_COMPILER) -f bin bootloader.asm -o bootloader.bin
kernel_entry.o: kernel_entry.asm
	$(ASM_COMPILER) -f elf32 kernel_entry.asm -o kernel_entry.o
%.o: %.c
	$(C_COMPILER) $(C_FLAGS) -c $< -o $@
kernel.bin: kernel_entry.o $(C_OBJECTS) linker.ld
	$(LINKER) $(LINKER_FLAGS) kernel_entry.o $(C_OBJECTS) -o kernel.elf
	objcopy -O binary kernel.elf kernel.bin
program.img: bootloader.bin kernel.bin
	dd if=/dev/zero of=program.img bs=1k count=1024
	dd if=bootloader.bin of=program.img conv=notrunc bs=512 count=1 seek=0
	dd if=kernel.bin of=program.img conv=notrunc bs=512 seek=1
clean:
	rm -f *.o *.bin *.elf program.img
.PHONY: all clean
