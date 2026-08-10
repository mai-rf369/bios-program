all: os.img
boot.bin: boot.asm
	nasm -f bin boot.asm -o boot.bin
kernel_entry.o: kernel_entry.asm
	nasm -f elf32 kernel_entry.asm -o kernel_entry.o
kernel.o: kernel.c
	gcc -m32 -ffreestanding -fno-pie -fno-stack-protector -nostdlib -Wall -Wextra -c kernel.c -o kernel.o
kernel.bin: kernel_entry.o kernel.o linker.ld
	ld -m elf_i386 -T linker.ld --nmagic kernel_entry.o kernel.o -o kernel.elf
	objcopy -O binary kernel.elf kernel.bin
os.img: boot.bin kernel.bin
	cat boot.bin kernel.bin > os.img
	dd if=/dev/zero bs=1024 count=1024 >> os.img
