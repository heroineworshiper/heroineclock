
; RB4 display clock
; RB5 display data
; RA1 display enable
; RA0 thermometer
; RA2 thermometer low
; RA3 thermometer high
; RA7 alarm enable
; RD6 debounced button
; RD5 debounced button
; RD4 debounced button
; RD3 debounced button
; RD1 debounced button
; RB0 speaker

; unused:
; 50

; 4 byte LED bitmask
DISPLAY_BITMASK0 = H'20'
DISPLAY_BITMASK1 = H'21'
DISPLAY_BITMASK2 = H'22'
DISPLAY_BITMASK3 = H'23'

; if 0 is leading
DISPLAY_0 = H'44'

; counter for shifting loop
COUNTER = H'24'

; time has been set
IS_SET = H'47'

; current time
CURRENT_HOUR = H'25'                    ; 1 - 12
CURRENT_MINUTE = H'26'                  ; 0 - 59
CURRENT_FRACSECONDH = H'3c'              ; 0 - total fracseconds per minute
CURRENT_FRACSECONDL = H'3d'              ; 0 - total fracseconds per minute
CURRENT_AMPM = H'28'                    ; 0 - am  1 - pm
CURRENT_DOTS = H'29'                    ; 0 - dots off 1 - dots on

; alarm time
ALARM_HOUR = H'2a'
ALARM_MINUTE = H'2b'
ALARM_AMPM = H'2d'
ALARM_ENABLED = H'2e'                   ; 0 - 1
OLD_ALARM_ENABLED = H'34'

; ones and tens result and input
ONES = H'2f'
TENS = H'30'
HUNDREDS = H'43'


; number of counter overflows per TIMER_SCALE2
TIMER_SCALE1 = D'3125'
;TIMER_SCALE1 = D'600'


; number of TIMER_SCALE1 iterations per fractional second
TIMER_SCALE2 = D'25'
;TIMER_SCALE2 = D'10'


; scale2 counter
TIMER_COUNTER = H'31'


; total fracseconds per minute
FRACSECONDS = D'480'

; switch status masks
SWITCH_STATUS = H'32'                   ; old switch status
SWITCH_STATUS_NEW = H'33'               ; new switch status

; switch status bits
; SWITCH_MINUTE and SWITCH_ALARM_SET are inverted
SWITCH_ALARM_ENABLE = D'7'
SWITCH_HOUR = D'5'
SWITCH_MINUTE = D'1'
SWITCH_TIME_SET = D'3'
SWITCH_ALARM_SET = D'4'
SWITCH_MODE = D'6'


SWITCH0_ACCUM = H'35'                   ; accumulators for debouncing
SWITCH1_ACCUM = H'36'
SWITCH2_ACCUM = H'37'
SWITCH3_ACCUM = H'38'
SWITCH4_ACCUM = H'39'
SWITCH5_ACCUM = H'3a'

SWITCH_COUNTER = H'4c'                  ; count until next switch sample
SWITCH_SAMPLERATE = H'40'               ; number of get_switches before next sample
SWITCH_COUNTDOWNH = H'65'    ; count down to warm up switch accumulators
SWITCH_COUNTDOWNL = H'66'    ; count down to warm up switch accumulators

; arguments for the debounce function
DEBOUNCE_MASK = H'3b'                   ; mask with relevant bit


; current operation
OPERATION = H'7e'
; previous operation for escaping set modes
PREV_OPERATION = h'2c'

; current operation constants
OPERATION_CURRENT_TIME = D'0'            ; current time
OPERATION_SET_TIME = D'1'                ; set time sleep
OPERATION_SET_ALARM = D'4'				  ; set alarm sleep
OPERATION_THERMOMETER = D'7'			  ; thermometer
OPERATION_TEST = D'8'        			  ; led test
OPERATION_AUTO = D'9'         ; cycle between time and temp after AUTO_DELAY

NEED_UPDATE_DISPLAY = H'4d'    ; update display this frac second


; sub operation
SUBOPERATION = H'7f'

; sub operation constants
SUBOPERATION_NONE = D'0'             ; no operation
SUBOPERATION_SET_HOUR = D'1'		 ; hour pressed
SUBOPERATION_SET_MINUTE = D'2' 	     ; minute pressed
SUBOPERATION_TIME = D'3'             ; time part of auto display
SUBOPERATION_TEMP = D'4'             ; temp part of auto display


; number of frac seconds until next auto advance
SUBOPERATION_COUNTER = H'3e'

; constants for delay
SUBOPERATION_DELAY1 = D'8'
SUBOPERATION_DELAY2 = D'1'

; countdown to next auto display
AUTO_COUNTER = H'27'
; frac seconds in each auto display
AUTO_DURATION = D'40'

; A/D accumulation
THERMOMETER_INPUT = H'40'
; number of samples remaining to average.  Must count down from 256
THERMOMETER_COUNTER = H'48'
; precharge counter.  Counts down from THERMOMETER_PRECHARGE
THERMOMETER_COUNTER2 = H'51'

THERMOMETER_PRECHARGE = H'ff'
; temperature result.  The highest 8 bits are taken as the average.
THERMOMETER_VALUEL = H'41'
THERMOMETER_VALUEH = H'42'

ALARM_ON = H'4e'          ; alarm playing due to current time == alarm time
ALARM_PREVIEW = H'54'     ; alarm playing due to preview.  Ignore ALARM_ENABLED

; current note half period from memory in prescaled clock cycles
; 13 : 8 - whole part of half period
; 7 : 0 - fractional part of half period
ALARM_PERIODH = H'45'
ALARM_PERIODL = H'46'

; alarm note duration in half periods
; decreased every half period
ALARM_DURATIONH = H'49'
ALARM_DURATIONL = H'4f'

; value for timer 2 period to get 1000/second
NOTE_DURATION_TIMER = D'20'

; count down until alarm automatically stops in frac seconds
ALARM_TIMEOUT_COUNTERH = H'52'
ALARM_TIMEOUT_COUNTERL = H'53'
; frac seconds until alarm times out.
ALARM_TIMEOUT = D'480'

; start and end of current song for alarm
ALARM_SONGH = H'55'
ALARM_SONGL = H'56'
ALARM_ENDH = H'57'
ALARM_ENDL = H'58'

; pointer to current note excluding start of note table
; units are words so increment 2 words per note
ALARM_PTRH = H'4a'
ALARM_PTRL = H'4b'



BEEP_ON = H'59'              ; beep on due to button press
BEEP_PERIODH = H'5a'         ; current note half period from memory in prescaled clock cycles
BEEP_PERIODL = H'5b'

BEEP_DURATIONH = H'5c'       ; current note duration in half periods
BEEP_DURATIONL = H'5d'

BEEP_SONGH = H'5e'           ; start and end of current song for beep
BEEP_SONGL = H'5f'
BEEP_ENDH = H'60'
BEEP_ENDL = H'61'

BEEP_PTRH = H'62'            ; pointer to current beep note excluding start of
BEEP_PTRL = H'63'            ; note table.  Units are words so increment 2 words 
                             ; per note

BEEP_DURATION_FLAG = H'64'   ; copy of capture/compare flag for note duration timer



RESETVECTOR	= H'00'
INTVECTOR	= H'04'





; note half periods in prescaled CPU cycles
; range is from * 4 to / 4 on all frequencies
NOTE_C  		= D'1194'
NOTE_CS 		= D'1127'
NOTE_DF 		= D'1127'
NOTE_D  		= D'1064'
NOTE_DS 		= D'1004'
NOTE_EF 		= D'1004'
NOTE_E  		= D'948'
NOTE_F  		= D'895'
NOTE_FS 		= D'845'
NOTE_G  		= D'797'
NOTE_GS 		= D'752'
NOTE_AF 		= D'752'
NOTE_A  		= D'710'
NOTE_AS 		= D'670'
NOTE_BF 		= D'670'
NOTE_B  		= D'633'
NOTE_C2 		= D'597'
REST            = D'5000'
