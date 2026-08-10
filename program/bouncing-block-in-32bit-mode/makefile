all: image.img
bootloader.bin: bootloader.asm
	nasm -f bin bootloader.asm -o bootloader.bin
kernel_entry.o: kernel_entry.asm
	nasm -f elf32 kernel_entry.asm -o kernel_entry.o
kernel.o: kernel.c
	gcc -m32 -ffreestanding -fno-pie -fno-stack-protector -nostdlib -Wall -Wextra -c kernel.c -o kernel.o
kernel.bin: kernel_entry.o kernel.o linker.ld
	ld -m elf_i386 -T linker.ld --nmagic kernel_entry.o kernel.o -o kernel.elf
	objcopy -O binary kernel.elf kernel.bin
image.img: bootloader.bin kernel.bin
	dd if=/dev/zero of=image.img bs=1k count=1024
	dd if=bootloader.bin of=image.img conv=notrunc bs=512 count=1 seek=0
	dd if=kernel.bin of=image.img conv=notrunc bs=512 seek=1
