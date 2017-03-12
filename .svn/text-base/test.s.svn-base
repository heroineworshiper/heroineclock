include         "p16f877a.inc"





	clrf PORTB
	BANKSEL TRISB
	movlw B'11111110'
	movwf TRISB
	movlw B'11111111'
	movwf TRISA

	BANKSEL PORTB

loop:
	clrwdt                              ; clear watchdog
	btfsc PORTD, 0
	bsf PORTB, 0
	btfss PORTD, 0
	bcf PORTB, 0
	goto loop


END
