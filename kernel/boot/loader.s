global loader                   ; the entry symbol for ELF

MAGIC_NUMBER equ 0x1BADB002     ; define the magic number constant
FLAGS        equ 0x0            ; multiboot flags (no VBE — use default text mode)
CHECKSUM     equ -MAGIC_NUMBER  ; calculate the checksum

KERNEL_STACK_SIZE equ 0x40000   ; 256 KB stack (WM compose() needs >4 KB alone)

section .text                   ; start of the text (code) section
align 4                         ; the code must be 4 byte aligned
    dd MAGIC_NUMBER             ; write the magic number to the machine code,
    dd FLAGS                    ; the flags,
    dd CHECKSUM                 ; and the checksum

loader:                         ; the loader label (defined as entry point in linker script)
    extern __stack_top
    mov esp, __stack_top        ; point esp to the top of the kernel stack

    extern kmain
    push ebx                    ; multiboot info pointer
    push eax                    ; multiboot magic
    call kmain                  ; call the C function

.loop:
    jmp .loop                   ; loop forever

section .stack nobits align=16
kernel_stack:
    resb KERNEL_STACK_SIZE      ; reserve stack for the kernel
