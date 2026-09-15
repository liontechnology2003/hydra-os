; user_hello.s - Minimal user-space ELF test program
; Assemble with: nasm -f elf32 user_hello.s
; Link with: ld.lld -T user.ld -m elf_i386

bits 32

section .text

global _start
_start:
    ; Write "Hello from ELF!\n" using syscall 1 (SYS_WRITE), fd=1 (stdout)
    mov eax, 1              ; SYS_WRITE
    mov ebx, 1              ; fd = stdout
    mov ecx, msg            ; buffer
    mov edx, msg_len        ; length
    int 0x80

    ; Exit using syscall 0 (SYS_EXIT)
    mov eax, 0              ; SYS_EXIT
    int 0x80

    ; Should never reach here
    jmp $

section .rodata
msg: db "Hello from ELF!", 10
msg_len equ $ - msg
