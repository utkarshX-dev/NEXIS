ASM = nasm
CXX = g++
LD = ld
QEMU = qemu-system-i386

CXXFLAGS = -m32 -ffreestanding -fno-exceptions -fno-rtti -nostdlib -fno-builtin
LDFLAGS = -m elf_i386

.PHONY: all iso run run-tty clean

all: kernel.bin

boot.o: boot/boot.asm
	$(ASM) -f elf32 boot/boot.asm -o boot.o

kernel.o: kernel/kernel.cpp kernel/terminal.cpp kernel/io.cpp kernel/serial.cpp
	$(CXX) $(CXXFLAGS) -c kernel/kernel.cpp -o kernel.o
	$(CXX) $(CXXFLAGS) -c kernel/terminal.cpp -o terminal.o
	$(CXX) $(CXXFLAGS) -c kernel/io.cpp -o io.o
	$(CXX) $(CXXFLAGS) -c kernel/serial.cpp -o serial.o

kernel.bin: boot.o kernel.o terminal.o io.o serial.o linker.ld
	$(LD) $(LDFLAGS) -T linker.ld boot.o kernel.o terminal.o io.o serial.o -o kernel.bin

iso: kernel.bin
	mkdir -p iso/boot/grub
	cp kernel.bin iso/boot/kernel.bin
	cp grub.cfg iso/boot/grub/grub.cfg
	grub-mkrescue -o mykernel.iso iso

run: run-tty

run-tty: iso
	$(QEMU) -cdrom mykernel.iso -display none -serial stdio -monitor none

clean:
	rm -f *.o kernel.bin mykernel.iso
	rm -rf iso/boot
