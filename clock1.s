; clock firmware for 68hc11.  Unfinished.



include "registers.s"



	ldx  #REGS
	lds	#STACK_BASE        ; Set stack pointer

;	ldaa #0x10 + 0x08                   ; store bitmask for leading 1
;	oraa display_bitmask
;	staa display_bitmask

;	ldd bitmask2
;	addd display_bitmask
;	std display_bitmask

;	ldd bitmask3
;	addd display_bitmask + 1
;	std display_bitmask + 1

;	ldd bitmask4
;	addd display_bitmask + 2
;	std display_bitmask + 2


; Convert display_number to bitmask and display it
	ldaa display_bitmask                ; clear numeric part of display
	anda #0x80 + 0x40 + 0x20 + 0x04;
	staa display_bitmask
	clr display_bitmask + 1
	clr display_bitmask + 2
	clr display_bitmask + 3

	ldd display_number
	ldy #0x0                            ; skip leading 0's
	ldx #1000                           ; print thousands
	idiv                                ; divide D by X.  Quotient -> X  Remainder -> D
	beq skip1                           ; got 0

	ldy #0x1                            ; draw 0's
	xgdx                                ; store remainder
	ldaa #0x10 + 0x08                   ; store bitmask for leading 1
	oraa display_bitmask
	staa display_bitmask
	xgdx                                ; get remainder







skip1:
	ldx #100                            ; print hundreds
	idiv
	beq skip2                           ; got 0
	ldy #0x1
skip2:
	cpy #0x1
	bne skip3                           ; skip 0

	psha
	pshb
	xgdx
	aslb
	xgdx
	ldd bitmask0,x                      ; load bitmask of quotient
	addd display_bitmask                ; add bitmask to display
	std display_bitmask
	pulb                                ; get remainder
	pula

skip3:
	ldx #10                             ; print tens
	idiv
	beq skip4                           ; got 0
	ldy #0x1
skip4:
	cpy #0x1
	bne skip5                           ; skip 0

	psha
	pshb
	xgdx
	aslb
	xgdx
	ldd bitmask0,x                      ; load bitmask of quotient
	addd display_bitmask + 1            ; add bitmask to display
	std display_bitmask + 1
	pulb                                ; get remainder
	pula

skip5:
	aslb
	xgdx                                ; print ones
	ldd bitmask0,x
	addd display_bitmask + 2
	std display_bitmask + 2

test:
	ldx #REGS







; Shift display_bitmask into register.
; By shifting 32 bits, the last 4 bits are actually shifted off the register.
	ldy #0x4
shift1:
; shift byte into register
	ldaa display_bitmask - 1,y
	eora #0xff
	ldab #0x8

shift2:
; shift bit into register
	lsra                                ; shift bit into carry
	bclr PORTB,x PB0              	    ; clock down
	bcs shift3                    	    ; got a 1
	bclr PORTB,x PB1              	    ; Got a 0.  Set serial data to 0.
	bra shift4
shift3:
	bset PORTB,x PB1              	    ; Got a 1.  Set serial data to 1.
shift4:
	bset PORTB,x PB0              	    ; clock up


	decb                          	    ; decrease bit counter
	bne shift2                    	    ; not done.


; get next byte
	dey
	bne shift1                    	    ; send next byte











; RAM

; Current time in seconds since midnight
current_time:         .asciz        "\0\0\0"

; Binary coded decimal of number to display.
display_number:       .word         1888

; Which transistors are on.  Directly accessed for dots.  Indirectly
; accessed for number.
display_bitmask:      .int          0x40000000



; FLASH


; Bitmasks for numbers
bitmask0: .word 0x200 + 0x080 + 0x040 + 0x010 + 0x008 + 0x004
bitmask1: .word 0x040 + 0x010
bitmask2: .word 0x080 + 0x040 + 0x020 + 0x008 + 0x004
bitmask3: .word 0x080 + 0x040 + 0x020 + 0x010 + 0x008
bitmask4: .word 0x200 + 0x040 + 0x020 + 0x010
bitmask5: .word 0x200 + 0x080 + 0x020 + 0x010 + 0x008
bitmask6: .word 0x200 + 0x080 + 0x020 + 0x010 + 0x008 + 0x004
bitmask7: .word 0x080 + 0x040 + 0x010
bitmask8: .word 0x200 + 0x080 + 0x040 + 0x020 + 0x010 + 0x008 + 0x004
bitmask9: .word 0x200 + 0x080 + 0x040 + 0x020 + 0x010

; 80000000 - alarm
; 40000000 - heroine
; 20000000 - pm
; 10000000 - 1 top
; 08000000 - 1 bottom
; 04000000 - :

; Accidentally the first bit in each chip was sacrificed for cascading even
; though it didn't need to be.  Fortunately this accident aligned the digits
; to bytes.
; 02000000 - digit 3 segment 1
; 00800000 - digit 3 segment 2
; 00400000 - digit 3 segment 3
; 00200000 - digit 3 segment 4
; 00100000 - digit 3 segment 5
; 00080000 - digit 3 segment 6
; 00040000 - digit 3 segment 7

; 00020000 - digit 2 segment 1
; 00008000 - digit 2 segment 2
; 00004000 - digit 2 segment 3
; 00002000 - digit 2 segment 4
; 00001000 - digit 2 segment 5
; 00000800 - digit 2 segment 6
; 00000400 - digit 2 segment 7

; 00000200 - digit 1 segment 1
; 00000080 - digit 1 segment 2
; 00000040 - digit 1 segment 3
; 00000020 - digit 1 segment 4
; 00000010 - digit 1 segment 5
; 00000008 - digit 1 segment 6
; 00000004 - digit 1 segment 7











































