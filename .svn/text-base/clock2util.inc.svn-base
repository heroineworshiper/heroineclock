; skip if the contents of the registers are equal
SKIP_EQUAL macro reg1, reg2
	movf reg1, W
	subwf reg2, W
	btfss STATUS, Z
	ENDM

; skip if the contents of reg == literal
SKIP_EQUAL2 macro reg, literal
	movlw literal
	subwf reg, W
	btfss STATUS, Z
	ENDM

; skip if the contents of reg == 0
SKIP_ZERO macro reg
	movf reg, F
	btfss STATUS, Z
	ENDM

; skip if the contents of reg != 0
SKIP_NONZERO macro reg
	movf reg, F
	btfsc STATUS, Z
	ENDM

; skip if contents of reg1 != contents of reg2
SKIP_NOTEQUAL macro reg1, reg2
	movf reg2, W
	subwf reg1, W
	btfsc STATUS, Z
	ENDM

; skip if contents of reg1 != literal
SKIP_NOTEQUAL2 macro reg1, literal
	movlw literal
	subwf reg1, W
	btfsc STATUS, Z
	ENDM

; skip if contents of reg >= literal
SKIP_GREATEREQUAL macro reg, literal
	movlw literal
	subwf reg, W
	btfss STATUS, C
	ENDM

; skip if contents of reg < literal
SKIP_LESS macro reg, literal
	movlw literal
	subwf reg, W
	btfsc STATUS, C
	ENDM

; debounce a switch
; SWITCH_STATUS_NEW is overwritten with debounced bits
DEBOUNCE macro bitmask, accumulator
	movlw bitmask
	movwf DEBOUNCE_MASK
	movlw accumulator
	call debounce
	ENDM

SET_REGISTER macro reg, literal
	movlw literal
	movwf reg
	ENDM

; copy value of src register to dst register
COPY_REGISTER macro dst, src
	movf src, W
	movwf dst
	ENDM


