# ClaudeSys — Makefile
# Gera disk.img: um "disco" de 1.44MB pronto a arrancar no QEMU
# (ou a gravar num USB, ver README.md).

CC      = gcc
AS      = as
LD      = ld
OBJCOPY = objcopy

CFLAGS  = -m16 -ffreestanding -fno-pic -fno-stack-protector \
          -fno-asynchronous-unwind-tables -O0 -Wall -Wextra

all: disk.img

boot.o: boot.S
	$(AS) -32 boot.S -o boot.o

boot.bin: boot.o
	$(LD) -m elf_i386 --oformat binary -Ttext 0x7c00 -o boot.bin boot.o

kernel.o: kernel.c
	$(CC) $(CFLAGS) -c kernel.c -o kernel.o

kernel.elf: kernel.o kernel.ld
	$(LD) -m elf_i386 -T kernel.ld -o kernel.elf kernel.o

kernel.bin: kernel.elf
	$(OBJCOPY) -O binary kernel.elf kernel.bin

# Junta bootloader (512B) + kernel (arredondado a múltiplos de 512B,
# no máximo 16 setores = 8KB, conforme lido pelo bootloader) e preenche
# até 1.44MB para simular uma disquete/imagem USB completa.
disk.img: boot.bin kernel.bin
	@echo "A criar imagem de disco..."
	@if [ $$(wc -c < kernel.bin) -gt 16384 ]; then \
		echo "ERRO: kernel.bin ultrapassa 16384 bytes (32 setores)."; \
		exit 1; \
	fi
	dd if=/dev/zero of=disk.img bs=512 count=2880 2>/dev/null
	dd if=boot.bin of=disk.img conv=notrunc 2>/dev/null
	dd if=kernel.bin of=disk.img seek=1 conv=notrunc 2>/dev/null
	cat boot.bin kernel.bin > disk.img
	truncate -s 1474560 disk.img
	@echo "disk.img criado: $$(stat -c%s disk.img) bytes"

run: disk.img
	qemu-system-i386 -fda disk.img

clean:
	rm -f *.o *.bin *.elf disk.img

.PHONY: all run clean
