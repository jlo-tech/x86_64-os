NASM=nasm
ASMFLAGS=-f elf64
CC=gcc
CFLAGS=-g -c -ffreestanding -nostdlib -nostartfiles -fno-builtin -fno-stack-protector -Wall -Wextra -I include
LD=ld
QEMU=qemu-system-x86_64

OBJC = $(patsubst %.c, %.o, $(wildcard src/*.c))
OBJA = $(patsubst %.asm, %.o, $(wildcard src/asm/*.asm))

src/%.o: src/%.c
	@$(CC) $(CFLAGS) -o $@ $^

src/asm/%.o: src/asm/%.asm
	@$(NASM) $(ASMFLAGS) $^

kernel: $(OBJC) $(OBJA)
	@$(LD) -r -o kernel.o src/asm/*.o src/*.o
	@$(LD) -n -o kernel.bin -T linker.ld kernel.o

iso: kernel
	@cp kernel.bin iso/boot/
	@grub-mkrescue -o os.iso iso

run: iso
	@$(QEMU) -cdrom os.iso -m 4G

debug:
	@$(QEMU) -s -S -cdrom os.iso -m 4G

clean:
	@rm -r src/asm/*.o
	@rm -r src/*.o
	@rm -r *.o
	@rm -r kernel.bin
	@rm -r iso/boot/kernel.bin
	@rm -r os.iso