# RedLion OS — Microkernel Makefile
# Builds kernel.elf and redlion.iso

NASM = nasm
ASFLAGS = -f elf

UNAME_S := $(shell uname -s)
ifneq ($(findstring MINGW,$(UNAME_S))$(findstring MSYS,$(UNAME_S)),)
CC = clang
CXX = clang++
LD = ld.lld
MKISO = xorriso -as mkisofs
CFLAGS_BASE = -m32 --target=i386-elf -nostdlib -nostdinc -fno-builtin \
              -fno-stack-protector -nodefaultlibs -Wall -Wextra -Werror -c
LDFLAGS = -m elf_i386
else
CC = gcc
CXX = g++
LD = ld
MKISO = genisoimage
CFLAGS_BASE = -m32 -nostdlib -nostdinc -fno-builtin -fno-stack-protector \
              -nostartfiles -nodefaultlibs -Wall -Wextra -Werror -c
LDFLAGS = -T kernel/link.ld -melf_i386
endif

INCLUDES = -Ikernel -Ilib -Iservers -Iinclude
CFLAGS = $(CFLAGS_BASE) $(INCLUDES)
CXXFLAGS = $(CFLAGS_BASE) $(INCLUDES) -fno-exceptions -fno-rtti -fno-threadsafe-statics -std=c++17

# --- Object lists ---
KERNEL_ASM = kernel/boot/loader.o kernel/gdt_asm.o kernel/idt_asm.o kernel/io.o \
             kernel/process_asm.o
KERNEL_C   = kernel/kmain.o kernel/gdt.o kernel/idt.o kernel/fb.o \
             kernel/serial.o kernel/keyboard.o kernel/pmm.o \
             kernel/paging.o kernel/kheap.o kernel/process.o \
             kernel/pit.o kernel/syscall.o kernel/ipc.o \
             kernel/vbe.o kernel/gfx.o kernel/mouse.o kernel/splash.o \
             kernel/rtc.o
LIB_C      = lib/string.o lib/lineedit.o
LIB_CPP    = lib/cpp_runtime.o
SERVER_C   = servers/vfs.o servers/shell.o servers/snake.o servers/system.o \
             servers/display.o servers/input.o servers/storage.o servers/system_srv.o
SERVER_CPP = servers/wm.o servers/terminal.o

OBJECTS = $(KERNEL_ASM) $(KERNEL_C) $(LIB_C) $(LIB_CPP) $(SERVER_C) $(SERVER_CPP)

# --- Targets ---
all: kernel.elf

kernel.elf: $(OBJECTS)
	$(LD) -T kernel/link.ld $(LDFLAGS) -o kernel.elf $(OBJECTS)

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

# --- Assembly rules ---
kernel/boot/loader.o: kernel/boot/loader.s
	$(NASM) $(ASFLAGS) $< -o $@

kernel/gdt_asm.o: kernel/gdt_asm.s
	$(NASM) $(ASFLAGS) $< -o $@

kernel/idt_asm.o: kernel/idt_asm.s
	$(NASM) $(ASFLAGS) $< -o $@

kernel/io.o: kernel/io.s
	$(NASM) $(ASFLAGS) $< -o $@

kernel/process_asm.o: kernel/process_asm.s
	$(NASM) $(ASFLAGS) $< -o $@

# --- C rules ---
kernel/%.o: kernel/%.c
	$(CC) $(CFLAGS) $< -o $@

lib/%.o: lib/%.c
	$(CC) $(CFLAGS) $< -o $@

servers/%.o: servers/%.c
	$(CC) $(CFLAGS) $< -o $@

# --- C++ rules ---
lib/%.o: lib/%.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

servers/%.o: servers/%.c
	$(CC) $(CFLAGS) $< -o $@

servers/%.o: servers/%.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

clean:
	rm -rf kernel/*.o kernel/boot/*.o lib/*.o servers/*.o kernel.elf redlion.iso

.PHONY: all run clean
