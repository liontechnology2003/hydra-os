; user_hello_data.inc - Embedded user-space ELF binary
; This file is auto-generated - do not edit manually
; Source: /tmp/user_hello.elf

section .rodata
global user_hello_elf_data
global user_hello_elf_size

user_hello_elf_data:
    incbin "bin/user_hello.elf"
user_hello_elf_size equ $ - user_hello_elf_data
