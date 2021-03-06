; HeroineClock master program for 16f877
; (C) 2004 Heroine Virtual Ltd.






include "p16f877a.inc"
include "clock2vars.inc"
include "clock2util.inc"

	ORG RESETVECTOR                     ; jump to start on reset
	goto start

	ORG INTVECTOR                       ; jump to interrupt handler on interrupt
	goto interrupt






start:
	BANKSEL PORTA
	clrf PORTA
	clrf PORTB
	movlw H'ff'
	movwf SWITCH_STATUS
	clrf SWITCH0_ACCUM
	clrf SWITCH1_ACCUM
	clrf SWITCH2_ACCUM
	clrf SWITCH3_ACCUM
	clrf SWITCH4_ACCUM
	clrf BEEP_DURATION_FLAG

	BANKSEL TRISA                       ; select register bank 1

	movlw B'11001110'                   ; set led register pins to output
	movwf TRISB                         ; move accumulator to register

	movlw B'11111101'                   ; set display enable to output
	movwf TRISA


; set default variables
	BANKSEL OPERATION
	movlw OPERATION_CURRENT_TIME
;	movlw OPERATION_TEST
	movwf OPERATION  

	clrf SUBOPERATION
	movlw D'12'
	movwf CURRENT_HOUR
	movlw D'0'
	movwf CURRENT_MINUTE
	movlw D'0'
	movwf CURRENT_FRACSECONDH
	movwf CURRENT_FRACSECONDL
	movlw D'0'
	movwf CURRENT_AMPM
	movlw D'1'
	movwf CURRENT_DOTS
	clrf ALARM_ON
	clrf ALARM_PREVIEW

	movlw D'12'
	movwf ALARM_HOUR
	clrf ALARM_MINUTE
	clrf ALARM_AMPM

	clrf ALARM_ENABLED
	movlw H'1'
	movwf SWITCH_COUNTDOWNH
	movlw H'ff'
	movwf SWITCH_COUNTDOWNL
	
	SET_REGISTER ALARM_SONGH, HIGH(alarm_song1)
	SET_REGISTER ALARM_SONGL, LOW(alarm_song1)
	SET_REGISTER ALARM_ENDH, HIGH(alarm_end1)
	SET_REGISTER ALARM_ENDL, LOW(alarm_end1)

	clrf THERMOMETER_COUNTER
	clrf THERMOMETER_INPUT
	clrf THERMOMETER_VALUEH
	clrf THERMOMETER_VALUEL

	clrf IS_SET

; set timer reset value
; can't use interrupts because porta switches must be polled
	clrf TMR1H                          ; reset timer
	clrf TMR1L
	movlw B'00110001'                   ; set timer1 to maximum prescale and enable
	movwf T1CON

	movlw TIMER_SCALE2
	movwf TIMER_COUNTER
	movlw HIGH(TIMER_SCALE1)            ; load timeout value
	movwf CCPR1H
	movlw LOW(TIMER_SCALE1)
	movwf CCPR1L
	movlw B'00001010'                   ; set to set interrupt on every event
	movwf CCP1CON
	bcf PIR1, CCP1IF

	call update_display

; play startup sound
	SET_REGISTER BEEP_SONGH, HIGH(beep_song10)
	SET_REGISTER BEEP_SONGL, LOW(beep_song10)
	SET_REGISTER BEEP_ENDH, HIGH(beep_end10)
	SET_REGISTER BEEP_ENDL, LOW(beep_end10)
	call start_beep



loop:
	clrwdt                              ; clear watchdog

; test mode
	IF 0
	SKIP_EQUAL2 OPERATION, OPERATION_TEST
	goto loop2

	call get_switches                   ; compare new switch status to old status
	SKIP_NOTEQUAL SWITCH_STATUS, SWITCH_STATUS_NEW
	goto loop

	COPY_REGISTER SWITCH_STATUS, SWITCH_STATUS_NEW

	call update_display

	goto loop
	ENDIF








; handle change in switches
loop2:
	call get_switches

	SKIP_NOTEQUAL SWITCH_STATUS, SWITCH_STATUS_NEW
	goto loop4

; handle alarm enable change
		COPY_REGISTER OLD_ALARM_ENABLED, ALARM_ENABLED
		bcf ALARM_ENABLED, 0             ; default to off
		btfsc SWITCH_STATUS_NEW, SWITCH_ALARM_ENABLE  ; skip if really off
		bsf ALARM_ENABLED, 0             ; set to on

		SKIP_ZERO SWITCH_COUNTDOWNH         ; init countdown must be 0 to play alarm beep
		goto alarm_change_done
		SKIP_ZERO SWITCH_COUNTDOWNL
		goto alarm_change_done
		SKIP_NOTEQUAL ALARM_ENABLED, OLD_ALARM_ENABLED
		goto alarm_change_done

		btfsc ALARM_ENABLED,0            ; play sound for alarm disabled
		goto alarm_enabled
			SET_REGISTER BEEP_SONGH, HIGH(beep_song7) ; play sound for alarm disabled
			SET_REGISTER BEEP_SONGL, LOW(beep_song7)
			SET_REGISTER BEEP_ENDH, HIGH(beep_end7)
			SET_REGISTER BEEP_ENDL, LOW(beep_end7)
			call start_beep
			goto alarm_change_done

alarm_enabled:
			SET_REGISTER BEEP_SONGH, HIGH(beep_song8) ; play sound for alarm enabled
			SET_REGISTER BEEP_SONGL, LOW(beep_song8)
			SET_REGISTER BEEP_ENDH, HIGH(beep_end8)
			SET_REGISTER BEEP_ENDL, LOW(beep_end8)
			call start_beep

alarm_change_done:









; handle entry into set time mode
		btfss SWITCH_STATUS_NEW, SWITCH_TIME_SET    ; time button down
		goto set_time_done
		btfsc SWITCH_STATUS, SWITCH_TIME_SET        ; previously up
		goto set_time_done
			SKIP_NOTEQUAL2 OPERATION, OPERATION_SET_TIME
			goto exit_set_time

enter_set_time:
				SET_REGISTER BEEP_SONGH, HIGH(beep_song2)
				SET_REGISTER BEEP_SONGL, LOW(beep_song2)
				SET_REGISTER BEEP_ENDH, HIGH(beep_end2)
				SET_REGISTER BEEP_ENDL, LOW(beep_end2)
				call start_beep
				SKIP_NOTEQUAL2 OPERATION, OPERATION_SET_ALARM ; store previous operation if not a set mode
				goto enter_set_time2
					COPY_REGISTER PREV_OPERATION, OPERATION
enter_set_time2:
				SET_REGISTER OPERATION, OPERATION_SET_TIME
				SET_REGISTER SUBOPERATION, SUBOPERATION_NONE
				bsf CURRENT_DOTS, 0                 ; solid dots
				goto set_time_done

exit_set_time:
				SET_REGISTER BEEP_SONGH, HIGH(beep_song4)
				SET_REGISTER BEEP_SONGL, LOW(beep_song4)
				SET_REGISTER BEEP_ENDH, HIGH(beep_end4)
				SET_REGISTER BEEP_ENDL, LOW(beep_end4)
				call start_beep
				COPY_REGISTER OPERATION, PREV_OPERATION ; revert to previous operation
				SET_REGISTER SUBOPERATION, SUBOPERATION_TIME
				SET_REGISTER AUTO_COUNTER, AUTO_DURATION
set_time_done:





; handle entry into set alarm mode
		btfss SWITCH_STATUS_NEW, SWITCH_ALARM_SET   ; alarm button down
		goto set_alarm_done
		btfsc SWITCH_STATUS, SWITCH_ALARM_SET       ; previously up
		goto set_alarm_done
			SKIP_NOTEQUAL2 OPERATION, OPERATION_SET_ALARM
			goto exit_set_alarm

enter_set_alarm:
				SET_REGISTER BEEP_SONGH, HIGH(beep_song3)
				SET_REGISTER BEEP_SONGL, LOW(beep_song3)
				SET_REGISTER BEEP_ENDH, HIGH(beep_end3)
				SET_REGISTER BEEP_ENDL, LOW(beep_end3)
				call start_beep
				SKIP_NOTEQUAL2 OPERATION, OPERATION_SET_TIME ; store previous operation if not a set mode
				goto enter_set_alarm2
					COPY_REGISTER PREV_OPERATION, OPERATION

enter_set_alarm2:
				SET_REGISTER OPERATION, OPERATION_SET_ALARM
				SET_REGISTER SUBOPERATION, SUBOPERATION_NONE
				bsf CURRENT_DOTS, 0
				goto set_alarm_done

exit_set_alarm:
				SET_REGISTER BEEP_SONGH, HIGH(beep_song5)
				SET_REGISTER BEEP_SONGL, LOW(beep_song5)
				SET_REGISTER BEEP_ENDH, HIGH(beep_end5)
				SET_REGISTER BEEP_ENDL, LOW(beep_end5)
				call start_beep
				COPY_REGISTER OPERATION, PREV_OPERATION ; revert to previous operation
				SET_REGISTER SUBOPERATION, SUBOPERATION_TIME
				SET_REGISTER AUTO_COUNTER, AUTO_DURATION
set_alarm_done:










; handle mode cycling
	btfss SWITCH_STATUS_NEW, SWITCH_MODE
	goto change_mode_done
	btfsc SWITCH_STATUS, SWITCH_MODE
	goto change_mode_done

; mode button pressed
	SKIP_EQUAL2 OPERATION, OPERATION_SET_ALARM ; skip if set alarm mode
	goto change_mode3
		SKIP_ZERO ALARM_PREVIEW ; skip if not in alarm preview already
		goto change_mode4
			clrf ALARM_ON    ; enter alarm preview mode
			bsf ALARM_PREVIEW, 0
			call start_alarm
			goto change_mode_done

; advance current song and restart alarm playback
change_mode4:
		call next_song
		call start_alarm
		goto change_mode_done

change_mode3:
	SET_REGISTER BEEP_SONGH, HIGH(beep_song1)
	SET_REGISTER BEEP_SONGL, LOW(beep_song1)
	SET_REGISTER BEEP_ENDH, HIGH(beep_end1)
	SET_REGISTER BEEP_ENDL, LOW(beep_end1)
	call start_beep
	SKIP_EQUAL2 OPERATION, OPERATION_THERMOMETER ; skip if thermometer mode
	goto change_mode1
; currently in thermometer mode
		SET_REGISTER OPERATION, OPERATION_TEST ; advance mode
		goto change_mode_done



change_mode1:
	SKIP_EQUAL2 OPERATION, OPERATION_TEST ; skip if test mode
	goto change_mode2
		SET_REGISTER OPERATION, OPERATION_AUTO
		SET_REGISTER SUBOPERATION, SUBOPERATION_TEMP
		SET_REGISTER AUTO_COUNTER, AUTO_DURATION
		call start_thermometer
		goto change_mode_done

change_mode2:
	SKIP_EQUAL2 OPERATION, OPERATION_AUTO ; skip if automatic operation
	goto change_mode5
		SET_REGISTER OPERATION, OPERATION_CURRENT_TIME
		goto change_mode_done

change_mode5:
	SET_REGISTER OPERATION, OPERATION_THERMOMETER ; any other modes go to thermometer
	call start_thermometer

change_mode_done:






test_set_hour:
	btfss SWITCH_STATUS_NEW, SWITCH_HOUR   ; set hour down
	goto test_release_hour
	btfsc SWITCH_STATUS, SWITCH_HOUR       ; previously up
	goto test_release_hour

		SET_REGISTER BEEP_SONGH, HIGH(beep_song6)
		SET_REGISTER BEEP_SONGL, LOW(beep_song6)
		SET_REGISTER BEEP_ENDH, HIGH(beep_end6)
		SET_REGISTER BEEP_ENDL, LOW(beep_end6)
		call start_beep
		SET_REGISTER SUBOPERATION, SUBOPERATION_SET_HOUR  ; set suboperation
		SET_REGISTER SUBOPERATION_COUNTER, SUBOPERATION_DELAY1  ; set delay until autoadvance

; increase the hour for current time
		SKIP_EQUAL2 OPERATION, OPERATION_SET_TIME
		goto increase_alarm_hour1
increase_time_hour1:
			bsf IS_SET, 0
			incf CURRENT_HOUR, F
			SKIP_EQUAL2 CURRENT_HOUR, D'12'    ; toggle ampm at 12 hours
			goto increase_time_hour2
			movlw D'1'
			xorwf CURRENT_AMPM, F
increase_time_hour2:
			SKIP_EQUAL2 CURRENT_HOUR, D'13'    ; wrap hour
			goto increase_hour_done1
			SET_REGISTER CURRENT_HOUR, D'1'

increase_alarm_hour1:
		SKIP_EQUAL2 OPERATION, OPERATION_SET_ALARM
		goto increase_hour_done1
			incf ALARM_HOUR, F
			SKIP_EQUAL2 ALARM_HOUR, D'12'
			goto increase_alarm_hour2
			movlw D'1'
			xorwf ALARM_AMPM, F
increase_alarm_hour2:
			SKIP_EQUAL2 ALARM_HOUR, D'13'
			goto increase_hour_done1
			SET_REGISTER ALARM_HOUR, D'1'

increase_hour_done1:






test_release_hour:
	btfsc SWITCH_STATUS_NEW, SWITCH_HOUR   ; set hour up
	goto test_set_minute
	btfss SWITCH_STATUS, SWITCH_HOUR       ; previously up
	goto test_set_minute
		SET_REGISTER SUBOPERATION, SUBOPERATION_NONE








test_set_minute:
	btfss SWITCH_STATUS_NEW, SWITCH_MINUTE ; set minute down
	goto test_release_minute
	btfsc SWITCH_STATUS, SWITCH_MINUTE  ; previously up
	goto test_release_minute

		SET_REGISTER BEEP_SONGH, HIGH(beep_song9)
		SET_REGISTER BEEP_SONGL, LOW(beep_song9)
		SET_REGISTER BEEP_ENDH, HIGH(beep_end9)
		SET_REGISTER BEEP_ENDL, LOW(beep_end9)
		call start_beep
		SET_REGISTER SUBOPERATION, SUBOPERATION_SET_MINUTE
		SET_REGISTER SUBOPERATION_COUNTER, SUBOPERATION_DELAY1

; increase minute for current time
		SKIP_EQUAL2 OPERATION, OPERATION_SET_TIME
		goto increase_alarm_minute1
increase_time_minute1:
			bsf IS_SET, 0
			incf CURRENT_MINUTE, F
			SKIP_GREATEREQUAL CURRENT_MINUTE, D'60'
			goto increase_minute_done1
			SET_REGISTER CURRENT_MINUTE, D'0'
			clrf CURRENT_FRACSECONDH    ; reset seconds
			clrf CURRENT_FRACSECONDL

increase_alarm_minute1:
		SKIP_EQUAL2 OPERATION, OPERATION_SET_ALARM
		goto increase_minute_done1
			incf ALARM_MINUTE, F
			SKIP_GREATEREQUAL ALARM_MINUTE, D'60'
			goto increase_minute_done1
			SET_REGISTER ALARM_MINUTE, D'0'

increase_minute_done1:






test_release_minute:
	btfsc SWITCH_STATUS_NEW, SWITCH_MINUTE   ; set minute up
	goto switches_done
	btfss SWITCH_STATUS, SWITCH_MINUTE       ; previously down
	goto switches_done
		SET_REGISTER SUBOPERATION, SUBOPERATION_NONE


	




switches_done:
	COPY_REGISTER SWITCH_STATUS, SWITCH_STATUS_NEW

; stop thermometer if not a set, thermometer or auto mode
	SKIP_NOTEQUAL2 OPERATION, OPERATION_THERMOMETER
	goto skip_thermometer
	SKIP_NOTEQUAL2 OPERATION, OPERATION_SET_TIME
	goto skip_thermometer
	SKIP_NOTEQUAL2 OPERATION, OPERATION_SET_ALARM
	goto skip_thermometer
	SKIP_NOTEQUAL2 OPERATION, OPERATION_AUTO
	goto skip_thermometer
		bcf ADCON0, 0                       ; disable A/D converter

skip_thermometer:
	call update_display







; poll timer
loop4:
	BANKSEL OPERATION
	btfss PIR1, CCP1IF                  ; skip if timer not triggered
	goto loop5




timer_event:
	bcf PIR1, CCP1IF                    ; reset interrupt
	movlw LOW(TIMER_SCALE1)             ; increase timer position
	addwf CCPR1L, F
	btfsc STATUS, C
	incf CCPR1H, F                      ; carry the 256
	movlw HIGH(TIMER_SCALE1)
	addwf CCPR1H, F                     ; let wrap around
	decfsz TIMER_COUNTER, F             ; decrease timer counter.  skip if 0
	goto loop5

; half second event
		SET_REGISTER TIMER_COUNTER, TIMER_SCALE2      ; reset timer counter
		SET_REGISTER NEED_UPDATE_DISPLAY, H'0'

		incf CURRENT_FRACSECONDL, F         ; increase fractional seconds lo
		btfsc STATUS, Z                     ; skip if not 0
		incf CURRENT_FRACSECONDH, F         ; increase fractional seconds hi


		decf ALARM_TIMEOUT_COUNTERL, F      ; decrease alarm timeout
		SKIP_EQUAL2 ALARM_TIMEOUT_COUNTERL, H'ff'
		decf ALARM_TIMEOUT_COUNTERH, F

		decfsz AUTO_COUNTER, F    ; decrease auto timeout
		goto timer_event2
		SKIP_NOTEQUAL2 SUBOPERATION, SUBOPERATION_TIME ; toggle auto operation
		goto toggle_auto1
		SKIP_EQUAL2 SUBOPERATION, SUBOPERATION_TEMP
		goto timer_event2

		SET_REGISTER SUBOPERATION, SUBOPERATION_TIME
		SET_REGISTER AUTO_COUNTER, AUTO_DURATION
		bsf NEED_UPDATE_DISPLAY, 0
		goto timer_event2

toggle_auto1:
		SET_REGISTER SUBOPERATION, SUBOPERATION_TEMP
		SET_REGISTER AUTO_COUNTER, AUTO_DURATION
		bsf NEED_UPDATE_DISPLAY, 0

timer_event2:
; solid dots in set time and set alarm modes
		SKIP_NOTEQUAL2 OPERATION, OPERATION_SET_TIME
		goto handle_soliddots
		SKIP_NOTEQUAL2 OPERATION, OPERATION_SET_ALARM
		goto handle_soliddots
			goto handle_halfsecond

handle_soliddots:
		bsf CURRENT_DOTS, 0
		bsf NEED_UPDATE_DISPLAY, 0
		goto done_halfsecond

handle_halfsecond:
		btfss CURRENT_FRACSECONDL, 2        ; skip if odd half second
		goto even_halfsecond

odd_halfsecond:
			btfss CURRENT_DOTS, 0           ; skip if dots on
			goto done_halfsecond            ; dots already off

				bsf NEED_UPDATE_DISPLAY, 0  ; set dots off
				bcf CURRENT_DOTS, 0
				goto done_halfsecond

even_halfsecond:
			btfsc CURRENT_DOTS, 0           ; skip if dots off
			goto done_halfsecond            ; dots already on

				bsf NEED_UPDATE_DISPLAY, 0  ; set dots on
				bsf CURRENT_DOTS, 0
				goto done_halfsecond




done_halfsecond:
; skip if fractional seconds not equal to total
		SKIP_EQUAL2 CURRENT_FRACSECONDH, HIGH(FRACSECONDS)
		goto timer_event_done
		SKIP_EQUAL2 CURRENT_FRACSECONDL, LOW(FRACSECONDS)
		goto timer_event_done

; hit total FRACSECONDS per minute
			clrf CURRENT_FRACSECONDH            ; reset seconds
			clrf CURRENT_FRACSECONDL            ; reset seconds
			incf CURRENT_MINUTE, F              ; increase minutes

			SKIP_GREATEREQUAL CURRENT_MINUTE, D'60'
			goto timer_event_done

				clrf CURRENT_MINUTE                 ; reset minutes
				incf CURRENT_HOUR, F                ; increase hours

				SKIP_EQUAL2 CURRENT_HOUR, D'12'
				goto timer_event1
					movlw D'1'
					xorwf CURRENT_AMPM, F               ; toggle AM/PM at hour 12

timer_event1:
				SKIP_EQUAL2 CURRENT_HOUR, D'13'
				goto timer_event_done               ; update display
					SET_REGISTER CURRENT_HOUR, D'1' ; reset current hour to 1


timer_event_done:
; update display if current time || auto mode && time mode
	SKIP_NOTEQUAL2 OPERATION, OPERATION_CURRENT_TIME
	goto timer_event_done1
	SKIP_EQUAL2 OPERATION, OPERATION_AUTO
	goto autoadvance_hour
	SKIP_EQUAL2 SUBOPERATION, SUBOPERATION_TIME
	goto autoadvance_hour

timer_event_done1:
	btfsc NEED_UPDATE_DISPLAY, 0        ; skip if display update not needed
	call update_display                 ; only display if current time displayed
	goto loop5





; handle autoadvance events
autoadvance_hour:
	SKIP_EQUAL2 SUBOPERATION, SUBOPERATION_SET_HOUR ; skip if setting hour
	goto autoadvance_minute

		decfsz SUBOPERATION_COUNTER, F ; decrease suboperation counter and skip if 0
		goto loop5

			SKIP_EQUAL2 OPERATION, OPERATION_SET_TIME ; skip if setting time
			goto autoadvance_alarm_hour1
autoadvance_time_hour1:
				incf CURRENT_HOUR, F
				SKIP_EQUAL2 CURRENT_HOUR, D'12' ; toggle am/pm
				goto autoadvance_time_hour2
				movlw D'1'
				xorwf CURRENT_AMPM, F
autoadvance_time_hour2:
				SKIP_EQUAL2 CURRENT_HOUR, D'13' ; wrap hour
				goto autoadvance_hour_done
				SET_REGISTER CURRENT_HOUR, D'1'
				goto autoadvance_hour_done

			SKIP_EQUAL2 OPERATION, OPERATION_SET_ALARM
			goto autoadvance_minute
autoadvance_alarm_hour1:
				incf ALARM_HOUR, F
				SKIP_EQUAL2 ALARM_HOUR, D'12'
				goto autoadvance_alarm_hour2
				movlw D'1'
				xorwf ALARM_AMPM, F
autoadvance_alarm_hour2:
				SKIP_EQUAL2 ALARM_HOUR, D'13'
				goto autoadvance_hour_done
				SET_REGISTER ALARM_HOUR, D'1'
				goto autoadvance_hour_done

autoadvance_hour_done:
			SET_REGISTER SUBOPERATION_COUNTER, SUBOPERATION_DELAY2 ; set new delay
			call update_display

autoadvance_minute:
	SKIP_EQUAL2 SUBOPERATION, SUBOPERATION_SET_MINUTE
	goto timer_event_done2

		decfsz SUBOPERATION_COUNTER, F ; decrease suboperation counter and skip if 0
		goto loop5

			SKIP_EQUAL2 OPERATION, OPERATION_SET_TIME ; skip if setting time
			goto autoadvance_alarm_minute1
autoadvance_time_minute1:
				incf CURRENT_MINUTE, F
				SKIP_EQUAL2 CURRENT_MINUTE, D'60'
				goto autoadvance_minute_done
				SET_REGISTER CURRENT_MINUTE, D'0'
				goto autoadvance_minute_done

autoadvance_alarm_minute1:
				incf ALARM_MINUTE, F
				SKIP_EQUAL2 ALARM_MINUTE, D'60'
				goto autoadvance_minute_done
				SET_REGISTER ALARM_MINUTE, D'0'
				goto autoadvance_minute_done

autoadvance_minute_done:
			SET_REGISTER SUBOPERATION_COUNTER, SUBOPERATION_DELAY2 ; set new delay
			call update_display
	goto loop5


timer_event_done2:







; update alarm status and play alarm sound
loop5:
	BANKSEL ALARM_ON
	btfsc ALARM_ON, 0                   ; skip if alarm not playing
	goto alarm_loop                     ; update alarm playback status

; test if alarm is enabled and current time matches alarm time
		btfss ALARM_ENABLED, 0          ; skip if alarm enabled
		goto alarm_done
		SKIP_EQUAL CURRENT_HOUR, ALARM_HOUR ; skip if hour matches
		goto alarm_done
		SKIP_EQUAL CURRENT_MINUTE, ALARM_MINUTE
		goto alarm_done
		SKIP_ZERO CURRENT_FRACSECONDH
		goto alarm_done
		SKIP_ZERO CURRENT_FRACSECONDL
		goto alarm_done
		SKIP_EQUAL CURRENT_AMPM, ALARM_AMPM
		goto alarm_done
		btfss IS_SET, 0                     ; skip if time set
		goto alarm_done

; initialize alarm playback
			bsf ALARM_ON, 0
			call start_alarm

alarm_loop:
; test if alarm is enabled and abort if not
	btfsc ALARM_ENABLED, 0              ; skip if alarm disabled
	goto alarm_loop1                    ; alarm still enabled


		call stop_alarm
		clrf ALARM_ON                   ; stop playback
		goto alarm_done

alarm_loop1:
	call update_alarm

alarm_done:






; update alarm preview status
loop6:
	btfss ALARM_PREVIEW, 0  ; skip if alarm previewing
	goto alarm_preview_done
		SKIP_EQUAL2 OPERATION, OPERATION_SET_ALARM ; skip if still in set alarm mode
		goto alarm_preview1
			call update_alarm
			goto alarm_preview_done

; left set alarm mode
alarm_preview1:
		call stop_alarm
		clrf ALARM_PREVIEW
		goto alarm_preview_done

alarm_preview_done:







; update thermometer in thermometer and auto temperature modes
; can't use in auto time mode because of inaccuracy
loop7:
	SKIP_NOTEQUAL2 OPERATION, OPERATION_THERMOMETER
	goto thermometer_event
	SKIP_EQUAL2 OPERATION, OPERATION_AUTO
	goto loop8
	SKIP_EQUAL2 SUBOPERATION, SUBOPERATION_TEMP
	goto loop8

thermometer_event:
	BANKSEL ADCON0
	btfsc ADCON0, 2                 ; skip if conversion complete
	goto loop8

		decfsz THERMOMETER_COUNTER2, F  ; skip if next precharge complete
		goto loop8

		SET_REGISTER THERMOMETER_COUNTER2, THERMOMETER_PRECHARGE ; reset precharge counter
		BANKSEL ADRESH         ; get high byte of a/d conversion
		movf ADRESH, W
		BANKSEL THERMOMETER_INPUT ; convert to temperature
		movwf THERMOMETER_INPUT
		call get_temperature

		addwf THERMOMETER_VALUEL, F ; add temperature to accumulator
		btfsc STATUS, C             ; carry the 256
		incf THERMOMETER_VALUEH, F

		decfsz THERMOMETER_COUNTER, F ; decrease accumulator counter
		goto thermometer2


			call update_display      ; draw temperature on display
			clrf THERMOMETER_VALUEL
			clrf THERMOMETER_VALUEH

			


thermometer2:
			bsf ADCON0, 2               ; start next conversion




; update beep
loop8:
	btfss BEEP_ON, 0
	goto beep_done
		call update_beep

beep_done:





loop9:
	SKIP_ZERO SWITCH_COUNTDOWNH      ; update switch counter if > 0
	goto decrease_countdown
	SKIP_ZERO SWITCH_COUNTDOWNL
	goto decrease_countdown
	goto loop

decrease_countdown:
	movlw H'1'
	subwf SWITCH_COUNTDOWNL, F ; decrease switch countdown low
	btfsc STATUS, C            ; skip if result negative
	goto loop

	subwf SWITCH_COUNTDOWNH, F ; decrease switch countdown high
	goto loop





start_beep:
	btfsc ALARM_ON, 0         ; skip if alarm not playing
	return                    ; abort if alarm playing
	btfsc ALARM_PREVIEW, 0    ; skip if alarm not previewing
	return                    ; abort if alarm previewing

	bsf BEEP_ON, 0
	SET_REGISTER CCP2CON, B'00001010' ; set capture/compare 2 for interrupt on match
	COPY_REGISTER BEEP_PTRH, BEEP_SONGH ; point to first note of current song
	COPY_REGISTER BEEP_PTRL, BEEP_SONGL
	SET_REGISTER T2CON, B'01111111' ; set timer 2 to time notes
	BANKSEL PR2
	SET_REGISTER PR2, NOTE_DURATION_TIMER
	BANKSEL BEEP_PERIODH
	bsf PORTB, 0
	call get_beep_note
; set first half period and force first half period update
	COPY_REGISTER CCPR2H, TMR1H
	COPY_REGISTER CCPR2L, TMR1L
	bsf BEEP_DURATION_FLAG, 1
	return








update_beep:
	btfsc PIR1, TMR2IF              ; skip if note duration timer flag is off
	goto update_beep2
	btfss BEEP_DURATION_FLAG, 0     ; skip if copy of note duration timer flag is on
	goto update_beep3

update_beep2:
		bcf PIR1, TMR2IF            ; reset timer flag
		bcf BEEP_DURATION_FLAG, 0
		movlw H'1'                  ; decrease note duration
		subwf BEEP_DURATIONL, F
		btfss STATUS, C
		decf BEEP_DURATIONH, F

		SKIP_EQUAL2 BEEP_DURATIONH, H'ff'  ; test if duration 0
		goto update_beep3
		SKIP_EQUAL2 BEEP_DURATIONL, H'ff'
		goto update_beep3

			call get_beep_note            ; read next beep note

update_beep3:
	btfss BEEP_ON, 0                 ; skip if beep still on
	return

; test if half period finished and abort if not
	btfss PIR2, CCP2IF               ; skip if next half period
	return

		SKIP_EQUAL2 BEEP_PERIODH, HIGH(REST) ; silence speaker during rest
		goto update_beep4
		SKIP_EQUAL2 BEEP_PERIODL, LOW(REST)
		goto update_beep4

		bcf PORTB, 0
		goto update_beep5

update_beep4:
		movlw H'1'                     ; invert speaker voltage
		xorwf PORTB, f

update_beep5:
		bcf PIR2, CCP2IF               ; clear interrupt

		movf BEEP_PERIODL, W           ; add half period to capture/compare 2
		addwf CCPR2L, F
		btfsc STATUS, C
		incf CCPR2H, F                 ; carry the 256
		movf BEEP_PERIODH, W
		addwf CCPR2H, F                ; let wrap around
		return








start_alarm:
	SET_REGISTER CCP2CON, B'00001010' ; set capture/compare 2 for interrupt on match
	COPY_REGISTER ALARM_PTRH, ALARM_SONGH ; point to first note of current song
	COPY_REGISTER ALARM_PTRL, ALARM_SONGL
	SET_REGISTER T2CON, B'01111111' ; set timer 2 to time notes
	BANKSEL PR2
	SET_REGISTER PR2, NOTE_DURATION_TIMER

	BANKSEL ALARM_PERIODH
	call get_alarm_note
; set first half period and force first half period update
	COPY_REGISTER CCPR2H, TMR1H
	COPY_REGISTER CCPR2L, TMR1L
	bsf PIR1, TMR2IF

; cause alarm to stop playing after a certain amount of time
	SET_REGISTER ALARM_TIMEOUT_COUNTERH, HIGH(ALARM_TIMEOUT)
	SET_REGISTER ALARM_TIMEOUT_COUNTERL, LOW(ALARM_TIMEOUT)
	return






stop_alarm:
	bcf PORTB, 0
	clrf T2CON      ; stop timer
	return





update_alarm:
	SKIP_ZERO ALARM_TIMEOUT_COUNTERH     ; skip if timeout counter 0
	goto update_alarm5
	SKIP_ZERO ALARM_TIMEOUT_COUNTERL     ; skip if timeout counter 0
	goto update_alarm5
		clrf ALARM_ON        ; disable alarm if timed out
		clrf ALARM_PREVIEW
		call stop_alarm
		return

update_alarm5:
	btfss PIR1, TMR2IF                  ; skip if note timer flag
	goto update_alarm2

		bcf PIR1, TMR2IF
		bsf BEEP_DURATION_FLAG, 0       ; propagate interrupt to beep routine
		movlw H'1'
		subwf ALARM_DURATIONL, F
		btfss STATUS, C                 ; skip if positive
		decf ALARM_DURATIONH, F         ; carry the 256
		
		SKIP_EQUAL2 ALARM_DURATIONH, H'ff'  ; test if duration empty
		goto update_alarm2
		SKIP_EQUAL2 ALARM_DURATIONL, H'ff'
		goto update_alarm2

			call get_alarm_note                   ; read next note


update_alarm2:
; test if half period finished and abort if not
	btfss PIR2, CCP2IF                  ; skip if next half period
	return

		SKIP_EQUAL2 ALARM_PERIODH, HIGH(REST) ; keep speaker off during rest
		goto update_alarm3
		SKIP_EQUAL2 ALARM_PERIODL, LOW(REST)
		goto update_alarm3

		bcf PORTB, 0
		goto update_alarm4

update_alarm3:
		movlw H'1'                      ; invert speaker voltage
		xorwf PORTB, F

update_alarm4:
		bcf PIR2, CCP2IF                ; clear interrupt

		movf ALARM_PERIODL, W           ; add half period to capture/compare 2
		addwf CCPR2L, F
		btfsc STATUS, C
		incf CCPR2H, F                  ; carry the 256
		movf ALARM_PERIODH, W
		addwf CCPR2H, F                 ; let wrap around
		return



; change alarm song
next_song:
	SKIP_EQUAL2 ALARM_SONGH, HIGH(alarm_song1)
	goto next_song1
	SKIP_EQUAL2 ALARM_SONGL, LOW(alarm_song1)
	goto next_song1

		SET_REGISTER ALARM_SONGH, HIGH(alarm_song2)
		SET_REGISTER ALARM_SONGL, LOW(alarm_song2)
		SET_REGISTER ALARM_ENDH, HIGH(alarm_end2)
		SET_REGISTER ALARM_ENDL, LOW(alarm_end2)
		return

next_song1:
	SKIP_EQUAL2 ALARM_SONGH, HIGH(alarm_song2)
	goto next_song2
	SKIP_EQUAL2 ALARM_SONGL, LOW(alarm_song2)
	goto next_song2

		SET_REGISTER ALARM_SONGH, HIGH(alarm_song3)
		SET_REGISTER ALARM_SONGL, LOW(alarm_song3)
		SET_REGISTER ALARM_ENDH, HIGH(alarm_end3)
		SET_REGISTER ALARM_ENDL, LOW(alarm_end3)
		return

next_song2:
	SET_REGISTER ALARM_SONGH, HIGH(alarm_song1)
	SET_REGISTER ALARM_SONGL, LOW(alarm_song1)
	SET_REGISTER ALARM_ENDH, HIGH(alarm_end1)
	SET_REGISTER ALARM_ENDL, LOW(alarm_end1)
	return






start_thermometer:
; configure a/d converter
	BANKSEL ADCON1
	SET_REGISTER ADCON1, B'01001111'
	BANKSEL ADCON0
	SET_REGISTER ADCON0, B'11000101'
	SET_REGISTER THERMOMETER_COUNTER, D'1' ; force display update on first cycle
	SET_REGISTER THERMOMETER_COUNTER2, D'1' ; force precharge complete on first cycle
	return




; update display for current operation
update_display:
; switch statement
	BANKSEL OPERATION
	clrf DISPLAY_BITMASK0
	clrf DISPLAY_BITMASK1
	clrf DISPLAY_BITMASK2
	clrf DISPLAY_BITMASK3

; these are always displayed
; set alarm enabled
	bcf DISPLAY_BITMASK0, 7             ; default to 0 for alarm bit
	btfsc ALARM_ENABLED, 0              ; skip if alarm disabled
	bsf DISPLAY_BITMASK0, 7             ; set alarm bit to 1
alarm_enabled1:

; set heroine icon to always on
	bsf DISPLAY_BITMASK0, 6




; operation == current time || set time || auto && suboperation time
	SKIP_NOTEQUAL2 OPERATION, OPERATION_CURRENT_TIME
	goto display_current_time
	SKIP_NOTEQUAL2 OPERATION, OPERATION_SET_TIME
	goto display_current_time
	SKIP_EQUAL2 OPERATION, OPERATION_AUTO
	goto update_display2
	SKIP_EQUAL2 SUBOPERATION, SUBOPERATION_TIME
	goto update_display2

display_current_time:
; only display when dots are on if not set
	btfsc IS_SET, 0
	goto display_current_time1

	btfsc CURRENT_DOTS, 0
	goto display_current_time1
	goto display_current_time2

display_current_time1:
; set ampm
	btfsc CURRENT_AMPM, 0               ; skip if ampm is disabled
	bsf DISPLAY_BITMASK0, 5             ; set ampm bit to 1

; set dots
	btfsc CURRENT_DOTS, 0               ; skip if dots are disabled
	bsf DISPLAY_BITMASK0, 2             ; set dots to 1

; load hours
	COPY_REGISTER ONES, CURRENT_HOUR ; load current hour into argument
	call divide                         ; convert into tens and ones

	movlw B'00011000'                   ; assume hours >= 10
	btfsc TENS, 0                       ; skip if tens == 0
	addwf DISPLAY_BITMASK0, F           ; set hours tens to 1

	movf ONES, W                        ; get ones
	call get_bitmask                    ; get bitmask
	movwf DISPLAY_BITMASK1              ; store in display bitmask

; load minutes
	COPY_REGISTER ONES, CURRENT_MINUTE ; divide minutes into tens and ones
	call divide

	movf TENS, W                        ; set bitmask for tens
	call get_bitmask
	movwf DISPLAY_BITMASK2

	movf ONES, w                        ; set bitmask for ones
	call get_bitmask
	movwf DISPLAY_BITMASK3

display_current_time2:
	call load_display                   ; update display register

	return





update_display2:
	SKIP_EQUAL2 OPERATION, OPERATION_SET_ALARM
	goto update_display6

display_alarm_time:
; set ampm
	btfsc ALARM_AMPM, 0               ; skip if ampm is disabled
	bsf DISPLAY_BITMASK0, 5           ; set ampm bit to 1

; set dots
	btfsc CURRENT_DOTS, 0               ; skip if dots are disabled
	bsf DISPLAY_BITMASK0, 2             ; set dots to 1

; load hours
	COPY_REGISTER ONES, ALARM_HOUR ; load current hour into argument
	call divide                         ; convert into tens and ones

	movlw B'00011000'                   ; assume hours >= 10
	btfsc TENS, 0                       ; skip if tens == 0
	addwf DISPLAY_BITMASK0, F           ; set hours tens to 1

	movf ONES, W                        ; get ones
	call get_bitmask                    ; get bitmask
	movwf DISPLAY_BITMASK1              ; store in display bitmask

; load minutes
	COPY_REGISTER ONES, ALARM_MINUTE ; divide minutes into tens and ones
	call divide

	movf TENS, W                        ; set bitmask for tens
	call get_bitmask
	movwf DISPLAY_BITMASK2

	movf ONES, w                        ; set bitmask for ones
	call get_bitmask
	movwf DISPLAY_BITMASK3

	call load_display                   ; update display register

	return




update_display6:
; operation == thermometer || auto && suboperation temp
	SKIP_NOTEQUAL2 OPERATION, OPERATION_THERMOMETER
	goto update_thermometer
	SKIP_EQUAL2 OPERATION, OPERATION_AUTO
	goto update_display9
	SKIP_EQUAL2 SUBOPERATION, SUBOPERATION_TEMP
	goto update_display9


update_thermometer:
	clrf DISPLAY_0                      ; don't display leading 0

	COPY_REGISTER ONES, THERMOMETER_VALUEH ; load thermometer value into argument
	call divide                         ; convert into hundreds, tens and ones

	movf HUNDREDS, W
	btfsc STATUS, Z                     ; skip if non zero
	goto thermometer_tens               ; ignore leading 0

	call get_bitmask
	movwf DISPLAY_BITMASK1
	bsf DISPLAY_0, 0

thermometer_tens:
	movf TENS, W
	btfss STATUS, Z                     ; skip if zero
	goto thermometer_tens1
	btfss DISPLAY_0, 0                  ; skip if display leading 0
	goto thermometer_ones               ; don't display 0

thermometer_tens1:
	call get_bitmask
	movwf DISPLAY_BITMASK2

thermometer_ones:
	movf ONES, W
	call get_bitmask
	movwf DISPLAY_BITMASK3

	call load_display
	return




update_display9:
	SKIP_EQUAL2 OPERATION, OPERATION_TEST
	return




; set all LEDs to on
	movlw B'11111111'
	movwf DISPLAY_BITMASK0
	movlw B'11111111'
	movwf DISPLAY_BITMASK1
	movlw B'11111111'
	movwf DISPLAY_BITMASK2
	movlw B'11111111'
	movwf DISPLAY_BITMASK3

	
	IF 0
; subtract leds depending on switch status
; alarm enable
	movlw B'00011000'
	btfsc SWITCH_STATUS, SWITCH_ALARM_ENABLE
		xorwf DISPLAY_BITMASK0, F

; hours
	movlw B'00000110'
	btfsc SWITCH_STATUS, SWITCH_HOUR
		xorwf DISPLAY_BITMASK1, F

; minutes
	movlw B'01010000'
	btfsc SWITCH_STATUS, SWITCH_MINUTE
		xorwf DISPLAY_BITMASK1, F

; set time
	movlw B'00000110'
	btfsc SWITCH_STATUS, SWITCH_TIME_SET
		xorwf DISPLAY_BITMASK2, F

; set alarm
	movlw B'01010000'
	btfsc SWITCH_STATUS, SWITCH_ALARM_SET
		xorwf DISPLAY_BITMASK2, F

; mode
	movlw B'00000110'
	btfsc SWITCH_STATUS, SWITCH_MODE
	xorwf DISPLAY_BITMASK3, F
	ENDIF

	call load_display

	return


; put status of switches in SWITCH_STATUS_NEW
get_switches:
	decfsz SWITCH_COUNTER, F            ; decrease counter.  skip if 0
	return                              ; not ready for next sample
	SET_REGISTER SWITCH_COUNTER, SWITCH_SAMPLERATE
	

	movlw B'00010010'       ; invert logic on switches with inverted logic
	xorwf PORTD, W
	andlw B'11111010'
	
	movwf SWITCH_STATUS_NEW



	DEBOUNCE B'10000000', SWITCH0_ACCUM
	DEBOUNCE B'01000000', SWITCH1_ACCUM
	DEBOUNCE B'00100000', SWITCH2_ACCUM
	DEBOUNCE B'00010000', SWITCH3_ACCUM
	DEBOUNCE B'00001000', SWITCH4_ACCUM
	DEBOUNCE B'00000010', SWITCH5_ACCUM


	return










; debounce a switch.  SWITCH_STATUS_NEW is changed based on switch accumulator
debounce:
	movwf FSR                           ; store debounce accumulator in file select register


	movf DEBOUNCE_MASK, W               ; get bit of switch
	andwf SWITCH_STATUS_NEW, W
	btfsc STATUS, Z                     ; skip if switch on
	goto debounce_off

	SKIP_EQUAL2 INDF, H'ff'              ; increase accumulator.
	incf INDF, F
	goto debounce_done

debounce_off:
	SKIP_ZERO INDF
	decf INDF, F

debounce_done:
	SKIP_GREATEREQUAL INDF, H'80'       ; skip if accumulator >= threshold
	goto debounce_disable

debounce_enable:
	movf DEBOUNCE_MASK, W               ; set switch to on
	iorwf SWITCH_STATUS_NEW, F
	return

debounce_disable:
	movf DEBOUNCE_MASK, W
	xorlw B'11111111'
	andwf SWITCH_STATUS_NEW, F          ; force bit 0 in mask
	return







; get the number in the tens place and the ones place
; ONES set by user with whole number.  Returned with ones.
; TENS Returned with tens.
; HUNDREDS Returned with hundreds
divide:
	BANKSEL ONES
	movlw H'ff'                         ; reset hundreds and tens counters to one less than results
	movwf TENS
	movwf HUNDREDS

	movlw D'100'                        ; load denominator
hundreds_loop:
	incf HUNDREDS, F                    ; assume numerator > 100
	subwf ONES, F                       ; subtract 100 from numerator
	btfsc STATUS, C                     ; skip if remainder < 0
	goto hundreds_loop
	movlw D'100'                        ; restore last positive numerator
	addwf ONES, F

	movlw D'10'                         ; load denominator
tens_loop:
	incf TENS, F                        ; assume numerator > denominator and increase tens
	subwf ONES, F                       ; subtract 10 from numerator
	btfsc STATUS, C                     ; skip if remainder is < 0
	goto tens_loop                      ; remainder is >= 0
	movlw D'10'                         ; restore last positive numerator
	addwf ONES,F
	return

	






; load the display bitmask into the display
; the display bitmask is destroyed
load_display:                           
	BANKSEL PORTA	                    ; select bank 0 for all operations
	bcf PORTA, 1                        ; disable display
	movlw H'20'                         ; set counter to 32
	movwf COUNTER

load_display_loop:
	bcf PORTB, 4                        ; clear display clock

	rrf DISPLAY_BITMASK3, F             ; get bit 0 of display
	bcf PORTB, 5                        ; assume bit is 1.  Set inverted value.
	btfss STATUS,C                      ; skip if bit is 1.
	bsf PORTB, 5                        ; bit is 0.  Set inverted value.

	bsf PORTB, 4                        ; clock in data

	rrf DISPLAY_BITMASK2, F             ; shift rest of register
	bcf DISPLAY_BITMASK3, 7             ; shifted 0 in
	btfsc STATUS,C                      ; shifted 1 in
	bsf DISPLAY_BITMASK3, 7

	rrf DISPLAY_BITMASK1, F             ; shift rest of register
	bcf DISPLAY_BITMASK2, 7             ; shifted 0 in
	btfsc STATUS,C                      ; shifted 1 in
	bsf DISPLAY_BITMASK2, 7

	rrf DISPLAY_BITMASK0, F             ; shift rest of register
	bcf DISPLAY_BITMASK1, 7             ; shifted 0 in
	btfsc STATUS,C                      ; shifted 1 in
	bsf DISPLAY_BITMASK1, 7

	decfsz COUNTER,F                    ; decrease counter and skip if 0
	goto load_display_loop              ; counter not 0

	bsf PORTA, 1                        ; counter 0.  enable display
	return
	






interrupt:
	retfie                              ; return from interrupt





	BANKSEL EECON1
	bsf EECON1, EEPGD                   ; select program memory
	bsf	EECON1, RD                      ; read word





; Get the bitmask for the number 0 - 9 in  and return it in W.
; We could do this by offsetting the program counter but that is less
get_bitmask:
	BANKSEL EEADRH                      ; reset data address

	addlw LOW(bitmasks)                 ; add bitmask table to bitmask number
	movwf EEADR                         ; store low byte of read

	movlw HIGH(bitmasks)                ; load high byte of table
	btfsc STATUS, C                     ; carry the 256 of the addition
	addlw H'1'
	movwf EEADRH                        ; set high byte of read

	BANKSEL EECON1
	bsf EECON1, EEPGD                   ; select progam memory
	bsf EECON1, RD                      ; read word
	nop                                 ; may not be important
	nop

	BANKSEL EEDATA
	movf EEDATA, W                      ; low byte of data is bitmask

	BANKSEL OPERATION                   ; switch bank back to user space
	return








; W contains A/D result.
; get the temperature out of the thermotable and put it in W
get_temperature:
	BANKSEL EEADR             ; point eeprom address to desired temperature entry
	movwf EEADR
	SET_REGISTER EEADRH, HIGH(thermotable)
	movlw LOW(thermotable)
	addwf EEADR, F            ; add thermotable start to A/D result
	btfsc STATUS, C
	incf EEADRH, F

	BANKSEL EECON1            ; command chip to read program memory
	bsf EECON1, EEPGD
	bsf EECON1, RD
	nop
	nop

	BANKSEL EEDATA
	movf EEDATA, W            ; low byte is temperature
	BANKSEL THERMOMETER_VALUEL
	return








; Get the current alarm note and duration
get_alarm_note:
	BANKSEL ALARM_PTRL
	movf ALARM_PTRL, W

	BANKSEL EEADR                       ; store in low byte register
	movwf EEADR

	BANKSEL ALARM_PTRH                  ; add high byte of table to current note
	movf ALARM_PTRH, W

	BANKSEL EEADRH                      ; store in high byte register
	movwf EEADRH

	BANKSEL EECON1                      ; select program memory
	bsf EECON1, EEPGD
	bsf EECON1, RD                      ; start read
	nop
	nop

	BANKSEL EEDATA
	movf EEDATH, W                      ; get high byte
	BANKSEL ALARM_PERIODH               ; store in current half period
	movwf ALARM_PERIODH
	BANKSEL EEDATA
	movf EEDATA, W                      ; get low byte
	BANKSEL ALARM_PERIODH               ; store in current half period
	movwf ALARM_PERIODL

	BANKSEL EEADR                       ; increase read ptr
	movlw H'1'
	addwf EEADR, F
	btfsc STATUS, C                     ; carry 256
	incf EEADRH, F

	BANKSEL EECON1                      ; start read
	bsf EECON1, RD
	nop
	nop

	BANKSEL EEDATA                      ; get high byte
	movf EEDATH, W

	BANKSEL ALARM_DURATIONH             ; store in duration high byte
	movwf ALARM_DURATIONH

	BANKSEL EEDATA
	movf EEDATA, W                      ; get low byte

	BANKSEL ALARM_DURATIONL
	movwf ALARM_DURATIONL               ; store in duration low byte



	BANKSEL EEADR                       ; increase read ptr for next note
	movlw H'1'
	addwf EEADR, F
	btfsc STATUS, C
	incf EEADRH, F

	movf EEADRH, W                      ; copy read ptr back to variable
	BANKSEL ALARM_PTRH
	movwf ALARM_PTRH
	BANKSEL EEADR
	movf EEADR, W
	BANKSEL ALARM_PTRL
	movwf ALARM_PTRL

	bcf PIR1, TMR2IF
	bcf BEEP_DURATION_FLAG, 0

	SKIP_EQUAL ALARM_PTRH, ALARM_ENDH  ; test for end of song
	return
	SKIP_EQUAL ALARM_PTRL, ALARM_ENDL
	return

	COPY_REGISTER ALARM_PTRH, ALARM_SONGH  ; rewind song pointer
	COPY_REGISTER ALARM_PTRL, ALARM_SONGL
	return










; Get the current alarm note and duration
get_beep_note:
	BANKSEL BEEP_PTRL

	SKIP_EQUAL BEEP_PTRH, BEEP_ENDH  ; test for end of song
	goto get_beep_note2
	SKIP_EQUAL BEEP_PTRL, BEEP_ENDL
	goto get_beep_note2

	clrf BEEP_ON    ; stop beep operation but leave timer on in case alarm is on
	bcf PORTB, 0    ; turn off speaker
	return


get_beep_note2:
	movf BEEP_PTRL, W

	BANKSEL EEADR                       ; store in low byte register
	movwf EEADR

	BANKSEL BEEP_PTRH                  ; add high byte of table to current note
	movf BEEP_PTRH, W

	BANKSEL EEADRH                      ; store in high byte register
	movwf EEADRH

	BANKSEL EECON1                      ; select program memory
	bsf EECON1, EEPGD
	bsf EECON1, RD                      ; start read
	nop
	nop

	BANKSEL EEDATA
	movf EEDATH, W                      ; get high byte
	BANKSEL BEEP_PERIODH               ; store in current half period
	movwf BEEP_PERIODH
	BANKSEL EEDATA
	movf EEDATA, W                      ; get low byte
	BANKSEL BEEP_PERIODH               ; store in current half period
	movwf BEEP_PERIODL

	BANKSEL EEADR                       ; increase read ptr
	movlw H'1'
	addwf EEADR, F
	btfsc STATUS, C                     ; carry 256
	incf EEADRH, F

	BANKSEL EECON1                      ; start read
	bsf EECON1, RD
	nop
	nop

	BANKSEL EEDATA                      ; get high byte
	movf EEDATH, W

	BANKSEL BEEP_DURATIONH             ; store in duration high byte
	movwf BEEP_DURATIONH

	BANKSEL EEDATA
	movf EEDATA, W                      ; get low byte

	BANKSEL BEEP_DURATIONL
	movwf BEEP_DURATIONL               ; store in duration low byte



	BANKSEL EEADR                       ; increase read ptr for next note
	movlw H'1'
	addwf EEADR, F
	btfsc STATUS, C
	incf EEADRH, F

	movf EEADRH, W                      ; copy read ptr back to variable
	BANKSEL BEEP_PTRH
	movwf BEEP_PTRH
	BANKSEL EEADR
	movf EEADR, W
	BANKSEL BEEP_PTRL
	movwf BEEP_PTRL
	return








bitmasks:
	retlw B'11011110'					; 0
	retlw B'01010000'					; 1
	retlw B'11101100'					; 2
	retlw B'11111000'					; 3
	retlw B'01110010'					; 4
	retlw B'10111010'					; 5
	retlw B'10111110'					; 6
	retlw B'11010000'					; 7
	retlw B'11111110'					; 8
	retlw B'11111010'					; 9



include "clock2songs.inc"
include "thermotable.inc"


; display bitmask
; 80000000 - alarm
; 40000000 - heroine
; 20000000 - pm
; 10000000 - 1 top
; 08000000 - 1 bottom
; 04000000 - :

; Accidentally the first bit in each chip was sacrificed for cascading even
; though it didn't need to be.  Fortunately this accident aligned the digits
; to bytes.
; 00020000 - digit 3 segment 1
; 00800000 - digit 3 segment 2
; 00400000 - digit 3 segment 3
; 00200000 - digit 3 segment 4
; 00100000 - digit 3 segment 5
; 00080000 - digit 3 segment 6
; 00040000 - digit 3 segment 7

; 00000200 - digit 2 segment 1
; 00008000 - digit 2 segment 2
; 00004000 - digit 2 segment 3
; 00002000 - digit 2 segment 4
; 00001000 - digit 2 segment 5
; 00000800 - digit 2 segment 6
; 00000400 - digit 2 segment 7

; 00000002 - digit 1 segment 1
; 00000080 - digit 1 segment 2
; 00000040 - digit 1 segment 3
; 00000020 - digit 1 segment 4
; 00000010 - digit 1 segment 5
; 00000008 - digit 1 segment 6
; 00000004 - digit 1 segment 7


END
