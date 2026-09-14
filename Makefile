OBJECTS = loader.o kmain.o io.o fb.o serial.o string.o vfs.o lineedit.o gdt.o gdt_s.o idt.o idt_s.o keyboard.o shell.o snake.o
AS = nasm
ASFLAGS = -f elf

UNAME_S := $(shell uname -s)
ifneq ($(findstring MINGW,$(UNAME_S))$(findstring MSYS,$(UNAME_S)),)
CC = clang
LD = ld.lld
MKISO = xorriso -as mkisofs
CFLAGS = -m32 --target=i386-elf -nostdlib -nostdinc -fno-builtin -fno-stack-protector \
         -nodefaultlibs -Wall -Wextra -Werror -c
LDFLAGS = -m elf_i386
else
CC = gcc
LD = ld
MKISO = genisoimage
CFLAGS = -m32 -nostdlib -nostdinc -fno-builtin -fno-stack-protector \
         -nostartfiles -nodefaultlibs -Wall -Wextra -Werror -c
LDFLAGS = -T link.ld -melf_i386
endif

all: kernel.elf

kernel.elf: $(OBJECTS)
	$(LD) -T link.ld $(LDFLAGS) $(OBJECTS) -o kernel.elf

redlion.iso: kernel.elf
	cp kernel.elf iso/boot/kernel.elf
	$(MKISO) -R \
	            -b boot/grub/stage2_eltorito \
	            -no-emul-boot \
	            -boot-load-size 4 \
	            -A os \
	            -input-charset utf8 \
	            -quiet \
	            -boot-info-table \
	            -o redlion.iso \
	            iso

run: redlion.iso
	bochs -f bochsrc.txt -q

gdt_s.o: gdt_asm.s
	$(AS) $(ASFLAGS) $< -o $@

idt_s.o: idt_asm.s
	$(AS) $(ASFLAGS) $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) $< -o $@

%.o: %.s
	$(AS) $(ASFLAGS) $< -o $@

clean:
	rm -rf *.o kernel.elf redlion.iso
