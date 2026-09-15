global outb             ; make the label outb visible outside this file
global outw             ; make the label outw visible outside this file
global outl             ; make the label outl visible outside this file
global inb              ; make the label inb visible outside this file
global inw              ; make the label inw visible outside this file
global inl              ; make the label inl visible outside this file

; outb - send a byte to an I/O port
; stack: [esp + 8] the data byte
;        [esp + 4] the I/O port
;        [esp    ] return address
outb:
    mov al, [esp + 8]    ; move the data to be sent into the al register
    mov dx, [esp + 4]    ; move the address of the I/O port into the dx register
    out dx, al           ; send the data to the I/O port
    ret                  ; return to the calling function

; outw - send a word (16-bit) to an I/O port
; stack: [esp + 8] the data word
;        [esp + 4] the I/O port
;        [esp    ] return address
outw:
    mov ax, [esp + 8]    ; move the data to be sent into the ax register
    mov dx, [esp + 4]    ; move the address of the I/O port into the dx register
    out dx, ax           ; send the data to the I/O port
    ret                  ; return to the calling function

; inb - returns a byte from the given I/O port
; stack: [esp + 4] The address of the I/O port
;        [esp    ] The return address
inb:
    mov dx, [esp + 4]    ; move the address of the I/O port to the dx register
    in al, dx            ; read a byte from the I/O port and store it in the al register
    ret                  ; return the read byte

; inw - returns a word (16-bit) from the given I/O port
; stack: [esp + 4] The address of the I/O port
;        [esp    ] The return address
inw:
    mov dx, [esp + 4]    ; move the address of the I/O port to the dx register
    in ax, dx            ; read a word from the I/O port
    ret                  ; return the read word

; outl - send a dword (32-bit) to an I/O port
; stack: [esp + 8] the data dword
;        [esp + 4] the I/O port
;        [esp    ] return address
outl:
    mov eax, [esp + 8]   ; move the data to be sent into eax
    mov dx, [esp + 4]    ; move the address of the I/O port into dx
    out dx, eax          ; send the dword to the I/O port
    ret                  ; return

; inl - returns a dword (32-bit) from the given I/O port
; stack: [esp + 4] The address of the I/O port
;        [esp    ] The return address
inl:
    mov dx, [esp + 4]    ; move the address of the I/O port to the dx register
    in eax, dx           ; read a dword from the I/O port
    ret                  ; return the read dword