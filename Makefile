NASM=nasm
ASMFLAGS=-f elf64
CC=gcc
CFLAGS=-g -c -ffreestanding -nostdlib -nostartfiles -fno-builtin -fno-stack-protector -Wall -Wextra -I include
LD=ld
QEMU=qemu-system-x86_64

OBJFS = $(patsubst %.c, %.o, $(wildcard src/fs/*.c))
OBJC  = $(patsubst %.c, %.o, $(wildcard src/*.c))
OBJA  = $(patsubst %.asm, %.o, $(wildcard src/asm/*.asm))

src/%.o: src/%.c
	@$(CC) $(CFLAGS) -o $@ $^

src/fs/%.o: src/fs/%.c
	@$(CC) $(CFLAGS) -o $@ $^

src/asm/%.o: src/asm/%.asm
	@$(NASM) $(ASMFLAGS) $^

kernel: $(OBJC) $(OBJA) $(OBJFS)
	@$(LD) -r -o kernel.o src/asm/*.o src/*.o src/fs/*.o
	@$(LD) -n -o kernel.bin -T linker.ld kernel.o

iso: kernel
	@cp kernel.bin iso/boot/
	@grub2-mkrescue -o os.iso iso

run: iso
	@$(QEMU) -cdrom os.iso -smp 4 -m 8G -drive id=disk,file=disk.img,format=raw,if=none -device virtio-blk-pci,drive=disk

debug: iso
	@$(QEMU) -s -S -cdrom os.iso -smp 4 -m 8G -drive id=disk,file=disk.img,format=raw,if=none -device virtio-blk-pci,drive=disk -no-reboot -no-shutdown -d int,cpu_reset

clean:
	@rm -r src/asm/*.o
	@rm -r src/fs/*.o
	@rm -r src/*.o
	@rm -r *.o
	@rm -r kernel.bin
	@rm -r iso/boot/kernel.bin
	@rm -r os.iso
	@python3 wipe.py
	@rm -r .gdb_history
