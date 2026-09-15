; process_asm.s - Context switch and user-mode entry for RedLion OS
; Assemble with: nasm -f elf process_asm.s

global process_context_switch
global switch_to_user_mode

; void process_context_switch(process_t *next)
; Saves current registers, loads next process's registers, and returns.
process_context_switch:
    ; [esp+4] = pointer to next process's regs_t
    mov eax, [esp+4]        ; eax = next->regs

    ; Load next process's registers
    mov ebx, [eax + 0]      ; eax
    mov ecx, [eax + 4]      ; ebx (we'll load eax last)
    mov edx, [eax + 8]      ; ecx
    mov esi, [eax + 16]     ; esi
    mov edi, [eax + 20]     ; edi
    mov ebp, [eax + 24]     ; ebp

    ; Load eip and jump to it
    mov esp, [eax + 28]     ; esp (user stack)
    push dword [eax + 36]   ; push eflags
    push dword [eax + 40]   ; push cs
    push dword [eax + 32]   ; push eip

    ; Load segment registers
    mov gs, [eax + 56]
    mov fs, [eax + 52]
    mov es, [eax + 48]
    mov ds, [eax + 44]

    ; Load eax last
    mov eax, [eax + 0]

    iretd

; void switch_to_user_mode(void *entry, uint32 stack)
; Jumps to user mode by setting up an iret frame on the kernel stack.
switch_to_user_mode:
    ; [esp+4] = entry point
    ; [esp+8] = user stack pointer
    cli
    mov ax, 0x23            ; user data segment (GDT index 4, RPL 3)
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    mov eax, [esp+8]        ; user stack
    push dword 0x23         ; ss (user stack segment)
    push eax                ; esp
    pushfd                  ; eflags
    pop eax
    or eax, 0x202           ; enable interrupts (IF=1)
    push eax
    push dword 0x1B         ; cs (user code segment, ring 3)
    push dword [esp+20]     ; entry point (4 dwords pushed above = 16 bytes)
    iretd
