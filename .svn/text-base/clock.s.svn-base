include "registers.s"

MODE_CLOCK = 0x0
MODE_SET_CLOCK = 0x1


; Which transistors are on.  Must not store states.
; Must be > 0 for shifting to work.
; Must be < 250 for BITMASK_PTR to work
DISPLAY_BITMASK = 0x0001

; argument for do_digit
BITMASK_PTR = 0x0005

	ldx #REGS
	lds	#STACK_BASE                     ; Set stack pointer
	bset TFLG1,x 0xff                   ; Reset timer flags
	ldd #0x0c16                         ; Reset current time
	std CURRENT_HOURS

; main loop
loop:
	brset TFLG1,x OC3F handle_timer     ; timer went off

	brclr PORTC,x PC0 alarm_disable     ; handle alarm disable
	ldaa ALARM_ON
	bne loop                            ; alarm already enabled
	ldaa #0x80
	staa ALARM_ON
	jmp update_display

alarm_disable:
	ldaa ALARM_ON
	beq loop                            ; alarm already disabled
	clr ALARM_ON
	jmp update_display
	bra loop

handle_timer:
	ldd #30000                          ; Make output compare go off at useful point in future
	addd TOC3,x
	std TOC3,x
	bclr TFLG1,x ~OC3F                  ; clear just the one flag

	dec SECOND                          ; count down to next half second
	ldaa SECOND
	cmpa #50
	bne bottom_half
	jmp update_display

bottom_half:
	cmpa #0                             ; increase second and set dots
	bne loop

	ldaa #100                           ; reset seconds
	staa SECOND

	inc CURRENT_SECONDS                 ; increase current time
	ldaa #60
	cmpa CURRENT_SECONDS
	bne update_display

	clr CURRENT_SECONDS
	inc CURRENT_MINUTES
	cmpa CURRENT_MINUTES
	bne update_display

	clr CURRENT_MINUTES
	inc CURRENT_HOURS
	ldaa #13
	cmpa CURRENT_HOURS
	bne update_display

	ldaa #1
	staa CURRENT_HOURS
	ldaa #0x20                          ; toggle pm
	eora IS_PM
	staa IS_PM


	jmp update_display
















update_display:
	ldaa #0x40                          ; heroine is always on
	oraa ALARM_ON                       ; alarm status always displayed
	staa DISPLAY_BITMASK
	clr DISPLAY_BITMASK + 1
	clr DISPLAY_BITMASK + 2
	clr DISPLAY_BITMASK + 3

	


; Calculate number to display based on current operation and store it in D
; Calculate dots

	ldaa SECOND
	cmpa #50
	bgt dots_off

	ldaa #0x04                          ; show dots
	oraa DISPLAY_BITMASK
	staa DISPLAY_BITMASK
	bra calculate_time

dots_off:
    ldaa IS_SET                         ; don't display if not set
	beq display_bitmask


; Display current time
calculate_time:
; calculate flags
	ldaa DISPLAY_BITMASK
	oraa IS_PM
	staa DISPLAY_BITMASK

	ldab CURRENT_HOURS
	ldaa #0x0
	xgdx
	inx
	ldd #0x0
calculate_hours:
	dex
	beq calculate_minutes
	addd #100
	bra calculate_hours

calculate_minutes:
	std TEMP
	ldab CURRENT_MINUTES
	ldaa #0x0
	addd TEMP



; D - number to display
display_number:
	ldy #0x0                            ; skip leading 0's
	ldx #1000                           ; print thousands
	idiv                                ; divide D by X.  Quotient -> X  Remainder -> D
	beq all_digits                      ; got 0

	ldy #0x1                            ; draw 0's
	xgdx                                ; store remainder
	ldaa #0x10 + 0x08                   ; store bitmask for leading 1
	oraa DISPLAY_BITMASK
	staa DISPLAY_BITMASK
	xgdx                                ; get remainder

all_digits:
	ldx #DISPLAY_BITMASK                ; set next bitmask position
	stx BITMASK_PTR
	ldx #100                            ; get hundreds
	jsr do_digit

	inc BITMASK_PTR + 1                 ; only works if we're not on a 256 byte boundary
	ldx #10                             ; get tens
	jsr do_digit

	inc BITMASK_PTR + 1
	ldx #1                              ; get ones
	jsr do_digit
	ldx #REGS




; upload bit mask
display_bitmask:
; Shift DISPLAY_BITMASK into register.
; By shifting 32 bits, the last 4 bits are actually shifted off the register.
	ldy #0x4
shift1:
; shift byte into register
	ldaa DISPLAY_BITMASK - 1,y
	coma
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
	jmp loop







; d - remainder
; x - denominator
; BITMASK_PTR - output bitmask position
; y - print next 0
do_digit:
	idiv
	beq skip_digit
	ldy #0x1
skip_digit:
	cpy #0x1
	bne really_skip_digit

	pshb                                ; store remainder
	psha
	xgdx                                ; convert quotient to bitmask
	aslb                                ; byte offset -> word offset
	addd #bitmask0                      ; get bitmask offset
	xgdx
	
	ldd 0,x                             ; get bitmask
	ldx BITMASK_PTR                     ; store bitmask at desired offset
	addd 0,x
	std 0,x
	pula                                ; get remainder
	pulb

really_skip_digit:
	rts




; RAM

; Current time in seconds since midnight
CURRENT_HOURS:         .byte          12
CURRENT_MINUTES:       .byte          0
CURRENT_SECONDS:       .byte          0
SECOND:                .byte          100

; takes less memory to use single bytes
IS_PM:                 .byte          0
;IS_SET:                .byte          0
IS_SET:                .byte          1
ALARM_ON:              .byte          0
TEMP:                  .word          0x0000
MODE:                  .byte          0







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
bitmask9: .word 0x200 + 0x080 + 0x040 + 0x020 + 0x010 + 0x008;

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



