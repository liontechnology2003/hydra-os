; process_asm.s - Context switch and user-mode entry for RedLion OS
; Assemble with: nasm -f elf process_asm.s
;
; regs_t layout (must match kernel/process.h):
;   offset  0: eax     offset  4: ebx     offset  8: ecx     offset 12: edx
;   offset 16: esi     offset 20: edi     offset 24: ebp     offset 28: esp
;   offset 32: eip     offset 36: eflags  offset 40: cs      offset 44: ss
;   offset 48: ds      offset 52: es      offset 56: fs      offset 60: gs

global process_context_switch
global switch_to_user_mode

%define REGS_EAX     0
%define REGS_EBX     4
%define REGS_ECX     8
%define REGS_EDX     12
%define REGS_ESI     16
%define REGS_EDI     20
%define REGS_EBP     24
%define REGS_ESP     28
%define REGS_EIP     32
%define REGS_EFLAGS  36
%define REGS_CS      40
%define REGS_SS      44
%define REGS_DS      48
%define REGS_ES      52
%define REGS_FS      56
%define REGS_GS      60

; Process structure fields (from process.h)
%define PROC_PID            0
%define PROC_STATE          4
%define PROC_NAME           8
%define PROC_REGS          40
%define PROC_PAGE_DIR      104
%define PROC_KERNEL_ESP    108
%define PROC_KERNEL_STACK  112
%define PROC_STACK_BASE    116

; Kernel stack interrupt frame offsets (what the CPU + ISR pushes):
;   [0]  ss       [4]  esp      [8]  eflags   [12] cs
;   [16] eip      [20] err_code [24] int_num
;   [28] eax(pusha) [32] ecx    [36] edx      [40] ebx
;   [44] esp_orig [48] ebp      [52] esi      [56] edi
;   [60] ds       [64] es       [68] fs       [72] gs

%define KSTK_SS      0
%define KSTK_ESP     4
%define KSTK_EFLAGS  8
%define KSTK_CS      12
%define KSTK_EIP     16
%define KSTK_ERR     20
%define KSTK_INT     24
%define KSTK_EAX     28
%define KSTK_ECX     32
%define KSTK_EDX     36
%define KSTK_EBX     40
%define KSTK_ESPRAW  44
%define KSTK_EBP     48
%define KSTK_ESI     52
%define KSTK_EDI     56
%define KSTK_DS      60
%define KSTK_ES      64
%define KSTK_FS      68
%define KSTK_GS      72

; void process_context_switch(process_t *next)
; Saves current process registers from the interrupt frame on the kernel stack,
; loads next process's registers, and returns via iretd.
process_context_switch:
    mov eax, [esp+4]            ; eax = &next->regs (pointer to regs_t)

    extern current_proc
    cmp dword [current_proc], 0
    je .initial_launch

    ; --- Save current process registers from the kernel interrupt frame ---
    ; The interrupt frame is at the bottom of the current kernel stack.
    ; current_proc->kernel_stack_base points to the bottom of the kernel stack.

    mov ebx, [current_proc]
    mov ecx, [ebx + PROC_KERNEL_STACK]   ; ecx = kernel_esp (top)
    sub ecx, 4096                         ; ecx = bottom of kernel stack (interrupt frame start)

    ; Save from interrupt frame [ecx] to regs_t [ebx]
    mov edi, [ecx + KSTK_EAX]
    mov [ebx + PROC_REGS + REGS_EAX], edi
    mov edi, [ecx + KSTK_EBX]
    mov [ebx + PROC_REGS + REGS_EBX], edi
    mov edi, [ecx + KSTK_ECX]
    mov [ebx + PROC_REGS + REGS_ECX], edi
    mov edi, [ecx + KSTK_EDX]
    mov [ebx + PROC_REGS + REGS_EDX], edi
    mov edi, [ecx + KSTK_ESI]
    mov [ebx + PROC_REGS + REGS_ESI], edi
    mov edi, [ecx + KSTK_EDI]
    mov [ebx + PROC_REGS + REGS_EDI], edi
    mov edi, [ecx + KSTK_EBP]
    mov [ebx + PROC_REGS + REGS_EBP], edi

    ; User esp (saved by CPU when entering ring 0)
    mov edi, [ecx + KSTK_ESP]
    mov [ebx + PROC_REGS + REGS_ESP], edi

    ; EIP (where user code was when interrupted)
    mov edi, [ecx + KSTK_EIP]
    mov [ebx + PROC_REGS + REGS_EIP], edi

    ; EFLAGS
    mov edi, [ecx + KSTK_EFLAGS]
    mov [ebx + PROC_REGS + REGS_EFLAGS], edi

    ; Segment registers (16-bit, stored as 32-bit in regs_t)
    movzx edi, word [ecx + KSTK_CS]
    mov [ebx + PROC_REGS + REGS_CS], edi
    movzx edi, word [ecx + KSTK_SS]
    mov [ebx + PROC_REGS + REGS_SS], edi
    movzx edi, word [ecx + KSTK_DS]
    mov [ebx + PROC_REGS + REGS_DS], edi
    movzx edi, word [ecx + KSTK_ES]
    mov [ebx + PROC_REGS + REGS_ES], edi
    movzx edi, word [ecx + KSTK_FS]
    mov [ebx + PROC_REGS + REGS_FS], edi
    movzx edi, word [ecx + KSTK_GS]
    mov [ebx + PROC_REGS + REGS_GS], edi

    ; --- Load next process's registers ---
    mov eax, [esp+4]            ; reload eax = &next->regs

.load_next:
    mov ebx, [eax + REGS_EBX]
    mov ecx, [eax + REGS_ECX]
    mov edx, [eax + REGS_EDX]
    mov esi, [eax + REGS_ESI]
    mov edi, [eax + REGS_EDI]
    mov ebp, [eax + REGS_EBP]

    ; Set up iret frame on the kernel stack
    mov esp, [eax + REGS_ESP]          ; user esp
    push dword [eax + REGS_EFLAGS]     ; eflags
    push dword [eax + REGS_CS]         ; cs
    push dword [eax + REGS_EIP]        ; eip

    ; Load segment registers
    mov gs, [eax + REGS_GS]
    mov fs, [eax + REGS_FS]
    mov es, [eax + REGS_ES]
    mov ds, [eax + REGS_DS]

    ; Load eax last
    mov eax, [eax + REGS_EAX]

    iretd

.initial_launch:
    ; No current process to save (first process being scheduled).
    ; Set up iret frame on the kernel stack to transition to ring 3.
    ; eax = &next->regs

    ; Load user entry point and user stack from regs
    mov ecx, [eax + REGS_EIP]         ; ecx = user entry point
    mov edx, [eax + REGS_ESP]         ; edx = user stack top

    ; Switch to this process's kernel stack
    mov ebx, [eax + PROC_KERNEL_ESP - PROC_REGS]  ; ebx = kernel_esp
    lea esp, [ebx - 20]              ; make room for iret frame (5 dwords)

    ; Build iret frame: [esp+0]=eip [esp+4]=cs [esp+8]=eflags [esp+12]=esp [esp+16]=ss
    mov [esp + 0], ecx               ; eip (user entry)
    mov dword [esp + 4], 0x1B        ; cs (user code, ring 3)
    pushfd
    pop ecx
    or ecx, 0x202                    ; enable interrupts (IF=1)
    mov [esp + 8], ecx               ; eflags
    mov [esp + 12], edx              ; esp (user stack top)
    mov dword [esp + 16], 0x23       ; ss (user data)

    ; Load user segment registers
    mov ax, 0x23
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

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

    push dword 0x23         ; ss (user stack segment)
    push dword [esp+8]      ; esp (user stack pointer)
    pushfd                  ; eflags
    pop eax
    or eax, 0x202           ; enable interrupts (IF=1)
    push eax
    push dword 0x1B         ; cs (user code segment, ring 3)
    push dword [esp+24]     ; entry point (offset: 4 pushes above = 16, +8 original = 24)
    iretd
