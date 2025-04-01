/*
 * HEROINECLOCK 2
 * Copyright (C) 2020-2025 Adam Williams <broadcast at earthling dot net>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 */


// build in MPLab

// testclock.c tests clock drift
// miditoclock.c converts midi files into data for the oscillators


// remote is PWM modulated
// press tuner button to engage tuner codes
// power - toggle alarm
// 1 - toggle setting alarm
//      alarm up - increase hour
//      alarm down - decrease hour
//      minute up - increase minute
//      minute down - decrease minute
// 2 - toggle setting time
//      alarm up - increase hour
//      alarm down - decrease hour
//      minute up - increase minute
//      minute down - decrease minute
// 3 - toggle test mode
//      hour up/hour down - select single segment
//      minute up/minute down - play alarm song



// pins:
// 
// A0 - LED
// A1 - LED
// A2 - LED
// A3 - LED
// A5 - LED
// B3 - IR RX
// C0 - LED
// C1 - LED
// C2/P1A - speaker
// C3 - LED
// C4 - LED
// C5 - LED
// C6/TX - DEBUG UART
// C7/RX - 
// E0 - LED
// E1 - LED
// E2 - LED
// E3 - LED
// E4 - LED
// E5 - LED
// E6 - LED
// E7 - LED

// F0 - LED
// F1 - LED
// F2 - LED
// F3 - LED
// F4 - LED
// F5 - LED
// F6 - LED


// G0 - LED
// G1 - LED

// LED masks:       	0	 	 1         2     3
// colon top:        00000000 00000000 00100000 000
// colon bottom:     00000000 00001000 00000000 000
// PM:               10000000 00000000 00000000 000
// alarm:            00000000 00000001 00000000 000

// digit 1 top:      00010000 00000000 00000000 000
// digit 1 bottom:   00000100 00000000 00000000 000

// digit 2 topleft:  01000000 00000000 00000000 000
// digit 2 topright: 00000000 00000000 00010000 000
// digit 2 botleft:  00000010 00000000 00000000 000
// digit 2 botright: 00100000 00000000 00000000 000
// digit 2 top:      00001000 00000000 00000000 000
// digit 2 center:   00000001 00000000 00000000 000
// digit 2 bottom:   00000000 00000010 00000000 000

// digit 3 topleft:  00000000 00000000 00000000 001
// digit 3 topright: 00000000 00000000 00000010 000
// digit 3 botleft:  00000000 00000000 01000000 000
// digit 3 botright: 00000000 00000000 00000001 000
// digit 3 top:      00000000 00000000 00001000 000
// digit 3 center:   00000000 00000000 10000000 000
// digit 3 bottom:   00000000 00010000 00000000 000

// digit 4 topleft:  00000000 10000000 00000000 000
// digit 4 topright: 00000000 00000000 00000000 010
// digit 4 botleft:  00000000 00100000 00000000 000
// digit 4 botright: 00000000 00000000 00000000 100
// digit 4 top:      00000000 00000000 00000100 000
// digit 4 center:   00000000 01000000 00000000 000
// digit 4 bottom:   00000000 00000100 00000000 000


#pragma config OSC = HSPLL  // Oscillator (HS oscillator with 4x PLL)
#pragma config LVP = OFF    // Low Voltage Program (Low-voltage ICSP disabled)


#include <p18f6585.h>
#include <stdint.h>






#define ENABLE_DEBUG

#ifdef ENABLE_DEBUG
#define DEBUG_TRIS TRISDbits.TRISD7
#define DEBUG_LAT LATDbits.LATD7

// the clockspeed
#define CLOCKSPEED 40000000

// 115200 baud for debug
#define BAUD_CODE 87   // 40Mhz
//#define BAUD_CODE 104  // 48Mhz

#define UART_BUFSIZE 1024
uint8_t uart_buffer[UART_BUFSIZE];
uint16_t uart_size = 0;
uint16_t uart_position1 = 0;
uint16_t uart_position2 = 0;
#endif // ENABLE_DEBUG


// indexes for different notes
#define _C1 0
#define _D1 2
#define _E1 4
#define _F1 5
#define _G1 7
#define _A1 9
#define _B1 11

#define _C2 12
#define _D2 14
#define _Eb2 15
#define _E2 16
#define _F2 17
#define _Gb2 18
#define _G2 19
#define _Ab2 20
#define _A2 21
#define _Bb2 22
#define _B2 23

#define _C3 24
#define _D3 26
#define _Eb3 27
#define _E3 28
#define _F3 29
#define _Gb3 30
#define _G3 31
#define _Ab3 32
#define _A3 33
#define _Bb3 34
#define _B3 35

#define _C4 36

// index to freq in CPU clocks
// divide by 4 to account for TMR2 postscaler
const uint16_t freqs[] = 
{
// C1
	(uint16_t)(CLOCKSPEED / 4 / 130.81 / 4), // 0
	(uint16_t)(CLOCKSPEED / 4 / 138.59 / 4),
	(uint16_t)(CLOCKSPEED / 4 / 146.83 / 4),
	(uint16_t)(CLOCKSPEED / 4 / 155.56 / 4),
	(uint16_t)(CLOCKSPEED / 4 / 164.81 / 4),
	(uint16_t)(CLOCKSPEED / 4 / 174.61 / 4),
	(uint16_t)(CLOCKSPEED / 4 / 185.00 / 4),
	(uint16_t)(CLOCKSPEED / 4 / 196.00 / 4),
	(uint16_t)(CLOCKSPEED / 4 / 207.65 / 4),
	(uint16_t)(CLOCKSPEED / 4 / 220.00 / 4),
	(uint16_t)(CLOCKSPEED / 4 / 233.08 / 4),
	(uint16_t)(CLOCKSPEED / 4 / 246.94 / 4),

// C2
	(uint16_t)(CLOCKSPEED / 4 / 261.63 / 4),  // 12
	(uint16_t)(CLOCKSPEED / 4 / 277.18 / 4),
	(uint16_t)(CLOCKSPEED / 4 / 293.66 / 4),
	(uint16_t)(CLOCKSPEED / 4 / 311.13 / 4),
	(uint16_t)(CLOCKSPEED / 4 / 329.63 / 4),
	(uint16_t)(CLOCKSPEED / 4 / 349.23 / 4),
	(uint16_t)(CLOCKSPEED / 4 / 369.99 / 4),
	(uint16_t)(CLOCKSPEED / 4 / 392.00 / 4),
	(uint16_t)(CLOCKSPEED / 4 / 415.30 / 4),
	(uint16_t)(CLOCKSPEED / 4 / 440.00 / 4),
	(uint16_t)(CLOCKSPEED / 4 / 466.16 / 4),
	(uint16_t)(CLOCKSPEED / 4 / 493.88 / 4),

// C3
	(uint16_t)(CLOCKSPEED / 4 / 523.251 / 4),  // 24
	(uint16_t)(CLOCKSPEED / 4 / 554.365 / 4),
	(uint16_t)(CLOCKSPEED / 4 / 587.330 / 4),
	(uint16_t)(CLOCKSPEED / 4 / 622.254 / 4),
	(uint16_t)(CLOCKSPEED / 4 / 659.255 / 4),
	(uint16_t)(CLOCKSPEED / 4 / 698.456 / 4),
	(uint16_t)(CLOCKSPEED / 4 / 739.989 / 4),
	(uint16_t)(CLOCKSPEED / 4 / 783.991 / 4),
	(uint16_t)(CLOCKSPEED / 4 / 830.609 / 4),
	(uint16_t)(CLOCKSPEED / 4 / 880.000 / 4),
	(uint16_t)(CLOCKSPEED / 4 / 932.328 / 4),
	(uint16_t)(CLOCKSPEED / 4 / 987.767 / 4),

	(uint16_t)(CLOCKSPEED / 4 / 1046.50 / 4),   // 36
};


#define SPEAKER_TRIS TRISCbits.TRISC2
// period of the audio PWM
#define AUDIO_PERIOD 0xff
// change this to change the volume.  It determines the DC offset 0 ... 0x7f.
#define MAX_VOLUME 0x68
#define NOTE_VOLUME (MAX_VOLUME / 4)
// periods for the oscillators in CPU clocks
uint16_t osc1_period;
uint16_t osc2_period;
uint16_t osc3_period;
// volume of the oscillators
uint8_t osc1_volume;
uint8_t osc2_volume;
uint8_t osc3_volume;
uint16_t decay_count = 0;
uint16_t powerdown_count = 0;
// current time of the oscillator waveform in CPU clocks
uint16_t osc1_time = 0;
uint16_t osc2_time = 0;
uint16_t osc3_time = 0;
// value to load into the duty cycle register during the next interrupt
uint8_t next_duty = 0;
// audio interrupt fired
uint8_t need_audio = 0;




typedef struct
{
// HZ / 10 after last command
	uint8_t delay;
// oscillator
	uint8_t osc;
// freq index to set oscillator to
	uint8_t freq;
// volume to set oscillator to
	uint8_t volume;
} song_t;


const song_t test_tone[] = 
{
	{ 0, 0, _A3, NOTE_VOLUME },
	{ 0, 1, _C4, NOTE_VOLUME },
    { 0xff, 0xff, 0xff, 0xff },
};

const song_t alarm_off_tone[] = 
{
	{ 0, 0, _C3, NOTE_VOLUME },
	{ 3, 1, _D3, NOTE_VOLUME },
	{ 3, 2, _Eb3, NOTE_VOLUME },
	{ 3, 0, _F3, NOTE_VOLUME },
	{ 3, 1, _G3, NOTE_VOLUME },
	{ 3, 2, _Ab3, NOTE_VOLUME },
	{ 3, 0, _B3, NOTE_VOLUME },
	{ 3, 1, _C4, NOTE_VOLUME },
    { 0xff, 0xff, 0xff, 0xff },
};

const song_t alarm_on_tone[] = 
{
 	{ 0, 0, _C4, NOTE_VOLUME },
	{ 3, 1, _B3, NOTE_VOLUME },
	{ 3, 2, _Ab3, NOTE_VOLUME },
	{ 3, 0, _G3, NOTE_VOLUME },
	{ 3, 1, _F3, NOTE_VOLUME },
	{ 3, 2, _Eb3, NOTE_VOLUME },
	{ 3, 0, _D3, NOTE_VOLUME },
	{ 3, 1, _C3, NOTE_VOLUME },
    { 0xff, 0xff, 0xff, 0xff },
};

const song_t set_alarm_tone1[] = 
{
	{ 0, 0, _C3,  NOTE_VOLUME },
	{ 3, 1, _Eb3, NOTE_VOLUME },
	{ 3, 2, _Gb3, NOTE_VOLUME },
	{ 3, 0, _C4,  NOTE_VOLUME },
    { 0xff, 0xff, 0xff, 0xff },
};

const song_t set_alarm_tone2[] = 
{
	{ 0, 0, _C4,  NOTE_VOLUME },
	{ 3, 1, _Gb3, NOTE_VOLUME },
	{ 3, 2, _Eb3, NOTE_VOLUME },
	{ 3, 0, _C3,  NOTE_VOLUME },
    { 0xff, 0xff, 0xff, 0xff },
};

const song_t set_time_tone1[] = 
{
	{ 0, 0, _C3,  NOTE_VOLUME },
	{ 3, 1, _E3,  NOTE_VOLUME },
	{ 3, 2, _G3,  NOTE_VOLUME },
	{ 3, 0, _C4,  NOTE_VOLUME },
    { 0xff, 0xff, 0xff, 0xff },
};

const song_t set_time_tone2[] = 
{
	{ 0, 0, _C4,  NOTE_VOLUME },
	{ 3, 1, _G3,  NOTE_VOLUME },
	{ 3, 2, _E3,  NOTE_VOLUME },
	{ 3, 0, _C3,  NOTE_VOLUME },
    { 0xff, 0xff, 0xff, 0xff },
};

const song_t seconds_tone1[] =
{
	{ 0, 0, _F2,  NOTE_VOLUME },
	{ 3, 1, _G2,  NOTE_VOLUME },
	{ 3, 2, _Ab2,  NOTE_VOLUME },
	{ 3, 0, _Bb2,  NOTE_VOLUME },
	{ 3, 1, _B2,  NOTE_VOLUME },
    { 0xff, 0xff, 0xff, 0xff },
};

const song_t seconds_tone2[] =
{
	{ 0, 0, _B2,  NOTE_VOLUME },
	{ 3, 1, _Bb2,  NOTE_VOLUME },
	{ 3, 2, _Ab2,  NOTE_VOLUME },
	{ 3, 0, _G2,  NOTE_VOLUME },
	{ 3, 1, _F2,  NOTE_VOLUME },
    { 0xff, 0xff, 0xff, 0xff },
};


const song_t up_tone[] = 
{
	{ 0, 0, _C3,  NOTE_VOLUME },
	{ 3, 1, _C4,  NOTE_VOLUME },
    { 0xff, 0xff, 0xff, 0xff },
};

const song_t dn_tone[] = 
{
	{ 0, 0, _C4,  NOTE_VOLUME },
	{ 3, 1, _C3,  NOTE_VOLUME },
    { 0xff, 0xff, 0xff, 0xff },
};

const song_t alarm_song[] = 
{
    { 0, 0, _C2, NOTE_VOLUME },
    { 6, 1, _E2, NOTE_VOLUME },
    { 6, 2, _G2, NOTE_VOLUME },
    { 6, 0, _B2, NOTE_VOLUME },

    { 6, 1, _C3, NOTE_VOLUME },
    { 6, 1, _E3, NOTE_VOLUME },
    { 6, 2, _G3, NOTE_VOLUME },
    { 6, 0, _B3, NOTE_VOLUME },
	
    { 6, 1, _C4, NOTE_VOLUME },
    { 6, 0, _B3, NOTE_VOLUME },
    { 6, 2, _G3, NOTE_VOLUME },
    { 6, 1, _E3, NOTE_VOLUME },
	
    { 6, 1, _C3, NOTE_VOLUME },
    { 6, 0, _B2, NOTE_VOLUME },
    { 6, 2, _G2, NOTE_VOLUME },
    { 6, 1, _E2, NOTE_VOLUME },

    { 6, 0, _C2, NOTE_VOLUME },
    { 6 * 16, 0, 0, 0 },
    { 0xff, 0xff, 0xff, 0xff },
};

// pointer to next note
const song_t *song_ptr = 0;
// downsample the time
uint8_t song_counter = 0;
uint8_t song_delay = 0;








// digit 2
const uint8_t led_masks2[][4] = 
{
// mask   0            1        2         3
	{ 0b01101010, 0b00000010, 0b00010000, 0b00000000 }, // 0
	{ 0b00100000, 0b00000000, 0b00010000, 0b00000000 }, // 1
	{ 0b00001011, 0b00000010, 0b00010000, 0b00000000 }, // 2
	{ 0b00101001, 0b00000010, 0b00010000, 0b00000000 }, // 3
	{ 0b01100001, 0b00000000, 0b00010000, 0b00000000 }, // 4
	{ 0b01101001, 0b00000010, 0b00000000, 0b00000000 }, // 5
	{ 0b01101011, 0b00000010, 0b00000000, 0b00000000 }, // 6
	{ 0b00101000, 0b00000000, 0b00010000, 0b00000000 }, // 7
	{ 0b01101011, 0b00000010, 0b00010000, 0b00000000 }, // 8
	{ 0b01101001, 0b00000010, 0b00010000, 0b00000000 }, // 9
};

// digit 3
const uint8_t led_masks3[][4] = 
{
// mask   0            1        2         3
	{ 0b00000000, 0b00010000, 0b01001011, 0b00000001 }, // 0
	{ 0b00000000, 0b00000000, 0b00000011, 0b00000000 }, // 1
	{ 0b00000000, 0b00010000, 0b11001010, 0b00000000 }, // 2
	{ 0b00000000, 0b00010000, 0b10001011, 0b00000000 }, // 3
	{ 0b00000000, 0b00000000, 0b10000011, 0b00000001 }, // 4
	{ 0b00000000, 0b00010000, 0b10001001, 0b00000001 }, // 5
	{ 0b00000000, 0b00010000, 0b11001001, 0b00000001 }, // 6
	{ 0b00000000, 0b00000000, 0b00001011, 0b00000000 }, // 7
	{ 0b00000000, 0b00010000, 0b11001011, 0b00000001 }, // 8
	{ 0b00000000, 0b00010000, 0b10001011, 0b00000001 }, // 9
};

// digit 4
const uint8_t led_masks4[][4] = 
{
	{ 0b00000000, 0b10100100, 0b00000100, 0b00000110 }, // 0
	{ 0b00000000, 0b00000000, 0b00000000, 0b00000110 }, // 1
	{ 0b00000000, 0b01100100, 0b00000100, 0b00000010 }, // 2
	{ 0b00000000, 0b01000100, 0b00000100, 0b00000110 }, // 3
	{ 0b00000000, 0b11000000, 0b00000000, 0b00000110 }, // 4
	{ 0b00000000, 0b11000100, 0b00000100, 0b00000100 }, // 5
	{ 0b00000000, 0b11100100, 0b00000100, 0b00000100 }, // 6
	{ 0b00000000, 0b00000000, 0b00000100, 0b00000110 }, // 7
	{ 0b00000000, 0b11100100, 0b00000100, 0b00000110 }, // 8
	{ 0b00000000, 0b11000100, 0b00000100, 0b00000110 }, // 9
};






// mane timer interrupts at this rate
#define HZ 250
// mane timer period - 1 to account for interrupt behavior
#define TIMER_PERIOD (CLOCKSPEED / 4 / 8 / HZ - 1)

#define ABS(x) ((x) < 0 ? (-(x)) : (x))

// counter for crystal tweeking.  Incremented HZ times per second
uint16_t crystal_time = 0;
// counter for flashing the display
uint8_t display_flash = 0;
// counter for 1 second.  Incremented HZ times per second
uint8_t time_hz = 0;
// the display
uint8_t seconds = 0;
uint8_t minutes = 0;
uint8_t hours = 12;
uint8_t ampm = 0;
// whether the alarm is enabled
uint8_t alarm = 0;
uint8_t colon = 1;
// whether the time has been set
uint8_t not_set = 1;
// interrupt fired
//volatile uint8_t have_time = 0;


// the modes
#define MODE_TIME 0 // show the current time
#define MODE_SET_TIME 1 // set the time
#define MODE_SET_ALARM 2 // set the alarm
#define MODE_TEST 3 // test mode
#define MODE_SECONDS 4 // seconds
uint8_t mode = MODE_TIME;


uint8_t alarm_hours = 12;
uint8_t alarm_minutes = 0;
uint8_t alarm_ampm = 0;
uint8_t alarm_sounding = 0;
// number of seconds the alarm has been sounding
#define MAX_ALARM_TIME (4 * 60)
uint8_t alarm_time = 0;



//volatile uint8_t have_serial = 0;
//volatile uint8_t serial_in = 0;



uint8_t led_mask0 = 0;
uint8_t led_mask1 = 0;
uint8_t led_mask2 = 0;
uint8_t led_mask3 = 0;


#define ENABLE_ALARM 0x00
#define HOUR_UP 0x01
#define HOUR_DN 0x02
#define MINUTE_UP 0x03
#define MINUTE_DN 0x04
#define SET_ALARM 0x05 // 1
#define SET_TIME 0x06  // 2
#define SET_TEST 0x07  // 3, 4
#define SECONDS 0x08  // 5


typedef struct
{
    const uint8_t *data;
    const uint8_t value;
} ir_code_t;


// remote control codes.  Discard 1st symbol received since we
// only capture the rising edge.
const uint8_t power_data[] = 
{ 0,0,1,0,1,0,0,0,0,1,0,1,0,1,1,1,1,0,0,0,0,0,0,1,1,1,1,1,1,1,1,0,0,0 };
const uint8_t volume_up_data[] = 
{ 0,0,1,0,1,0,0,0,0,1,0,1,0,1,1,1,1,0,1,0,1,0,0,1,1,1,0,1,0,1,1,0,0,0 };
const uint8_t volume_dn_data[] = 
{ 0,0,1,0,1,0,0,0,0,1,0,1,0,1,1,1,1,0,0,0,1,0,0,1,1,1,1,1,0,1,1,0,0,0 };
const uint8_t number1_data[] = 
{ 0,0,1,0,1,0,0,0,0,1,0,1,0,1,1,1,1,0,0,1,0,1,0,1,1,1,1,0,1,0,1,0,0,0 };
const uint8_t number2_data[] = 
{ 0,0,1,0,1,0,0,0,0,1,0,1,0,1,1,1,1,0,1,0,0,1,0,1,1,1,0,1,1,0,1,0,0,0 };
const uint8_t number3_data[] = 
{ 0,0,1,0,1,0,0,0,0,1,0,1,0,1,1,1,1,0,1,1,1,0,0,1,1,1,0,0,0,1,1,0,0,0 };
const uint8_t number4_data[] = 
{ 0,0,1,0,1,0,0,0,0,1,0,1,0,1,1,1,1,0,0,0,0,1,0,1,1,1,1,1,1,0,1,0,0,0 };
const uint8_t number5_data[] = 
{ 0,0,1,0,1,0,0,0,0,1,0,1,0,1,1,1,1,0,1,1,0,1,0,1,0,1,0,0,1,0,1,0,1,0 };
const uint8_t number6_data[] = 
{ 0,0,1,0,1,0,0,0,0,1,0,1,0,1,1,1,1,0,0,0,0,0,1,1,1,1,1,1,1,1,0,0,0,0 };


// these buttons alternate between 2 codes
const uint8_t tv_up_data[] = 
{ 0,0,1,0,0,0,0,0,0,0,0,0 };
const uint8_t tv_up_data2[] = 
{ 0,1,0,0,0,0,0,0,0,0,0,0 };
const uint8_t tv_down_data[] = 
{ 0,0,1,0,0,0,0,0,0,0,0,1 };
const uint8_t tv_down_data2[] = 
{ 0,1,0,0,0,0,0,0,0,0,0,1 };


#define CODE_SIZE sizeof(power_data)
#define CODE_SIZE2 sizeof(tv_up_data)



// translate the remote control buttons to clock buttons
const ir_code_t ir_codes[] = { 
    { power_data,      ENABLE_ALARM },
	{ number1_data,    SET_ALARM },
	{ number2_data,    SET_TIME },
	{ number3_data,    SET_TEST },
    { number5_data,    SECONDS },
	{ volume_up_data,  HOUR_UP },
	{ volume_dn_data,  HOUR_DN },
};

const ir_code_t ir_codes2[] = { 
	{ tv_up_data,      MINUTE_UP },
	{ tv_down_data,    MINUTE_DN },
	{ tv_up_data2,     MINUTE_UP },
	{ tv_down_data2,   MINUTE_DN },
};

// ticks
// 120ms observed.
#define IR_TIMEOUT (120 * HZ / 1000)
#define TOTAL_CODES (sizeof(ir_codes) / sizeof(ir_code_t))
#define TOTAL_CODES2 (sizeof(ir_codes2) / sizeof(ir_code_t))
// 1 & 0 bit
#define IR_THRESHOLD 135

uint8_t code_buffer[CODE_SIZE];
uint8_t code_offset = 0;
volatile uint8_t got_ir_int = 0;
volatile uint8_t ir_time = 0;
volatile uint8_t ir_timeout = 0;
// IR is transmitting repeats
uint8_t have_ir = 0;
// last IR code received
uint8_t ir_code = 0;
// delay before 1st repeat
uint8_t repeat_delay = 0;
#define REPEAT_DELAY (HZ / 2)
#define REPEAT_DELAY2 (HZ / 4)



#ifdef ENABLE_DEBUG


void print_byte(uint8_t c)
{
	if(uart_size < UART_BUFSIZE)
	{
		uart_buffer[uart_position1++] = c;
		uart_size++;
		if(uart_position1 >= UART_BUFSIZE)
		{
			uart_position1 = 0;
		}
	}
}

void print_text(const uint8_t *s)
{
	while(*s != 0)
	{
		print_byte(*s);
		s++;
	}
}


void print_number_nospace(uint16_t number)
{
	if(number >= 10000) print_byte('0' + (number / 10000));
	if(number >= 1000) print_byte('0' + ((number / 1000) % 10));
	if(number >= 100) print_byte('0' + ((number / 100) % 10));
	if(number >= 10) print_byte('0' + ((number / 10) % 10));
	print_byte('0' + (number % 10));
}

void print_number2(uint8_t number)
{
	print_byte('0' + ((number / 10) % 10));
	print_byte('0' + (number % 10));
}

void print_number(uint16_t number)
{
    print_number_nospace(number);
   	print_byte(' ');
 
}

const char hex_table[] = 
{
	'0', '1', '2', '3', '4', '5', '6', '7', 
	'8', '9', 'a', 'b', 'c', 'd', 'e', 'f'
};

void print_hex2(uint8_t number)
{
	print_byte(hex_table[(number >> 4) & 0xf]);
	print_byte(hex_table[number & 0xf]);
	print_byte(' ');
}

void print_bin_nospace(uint8_t number)
{
	print_byte((number & 0x80) ? '1' : '0');
	print_byte((number & 0x40) ? '1' : '0');
	print_byte((number & 0x20) ? '1' : '0');
	print_byte((number & 0x10) ? '1' : '0');
	print_byte((number & 0x8) ? '1' : '0');
	print_byte((number & 0x4) ? '1' : '0');
	print_byte((number & 0x2) ? '1' : '0');
	print_byte((number & 0x1) ? '1' : '0');
}








#else // ENABLE_DEBUG


#define print_text(x) ;
#define print_hex2(x) ;
#define print_number(x) ;
#define print_number2(x) ;
#define print_number_nospace(x) ;
#define print_byte(x) ;








#endif // !ENABLE_DEBUG


void dump_leds()
{
	print_bin_nospace(led_mask0);
	print_byte(' ');
	print_bin_nospace(led_mask1);
	print_byte(' ');
	print_bin_nospace(led_mask2);
	print_byte(' ');
	print_bin_nospace(led_mask3);
	print_byte('\n');

}

// convert the led_mask into pin values
void write_leds()
{
	LATAbits.LATA0 = led_mask0 & 0x01;
	LATAbits.LATA1 = (led_mask0 & 0x02) ? 1 : 0;
	LATAbits.LATA2 = (led_mask0 & 0x04) ? 1 : 0;
	LATAbits.LATA3 = (led_mask0 & 0x08) ? 1 : 0;
	LATAbits.LATA5 = (led_mask0 & 0x10) ? 1 : 0;
	LATCbits.LATC0 = (led_mask0 & 0x20) ? 1 : 0;
	LATCbits.LATC1 = (led_mask0 & 0x40) ? 1 : 0;
	LATCbits.LATC3 = (led_mask0 & 0x80) ? 1 : 0;
	LATCbits.LATC4 = led_mask1 & 0x01;
	LATCbits.LATC5 = (led_mask1 & 0x02) ? 1 : 0;
	LATEbits.LATE0 = (led_mask1 & 0x04) ? 1 : 0;
	LATEbits.LATE1 = (led_mask1 & 0x08) ? 1 : 0;
	LATEbits.LATE2 = (led_mask1 & 0x10) ? 1 : 0;
	LATEbits.LATE3 = (led_mask1 & 0x20) ? 1 : 0;
	LATEbits.LATE4 = (led_mask1 & 0x40) ? 1 : 0;
	LATEbits.LATE5 = (led_mask1 & 0x80) ? 1 : 0;
	LATEbits.LATE6 = led_mask2 & 0x01;
	LATEbits.LATE7 = (led_mask2 & 0x02) ? 1 : 0;
	LATFbits.LATF0 = (led_mask2 & 0x04) ? 1 : 0;
	LATFbits.LATF1 = (led_mask2 & 0x08) ? 1 : 0;
	LATFbits.LATF2 = (led_mask2 & 0x10) ? 1 : 0;
	LATFbits.LATF3 = (led_mask2 & 0x20) ? 1 : 0;
	LATFbits.LATF4 = (led_mask2 & 0x40) ? 1 : 0;
	LATFbits.LATF5 = (led_mask2 & 0x80) ? 1 : 0;
	LATFbits.LATF6 = led_mask3 & 0x01;
	LATGbits.LATG0 = (led_mask3 & 0x02) ? 1 : 0;
	LATGbits.LATG1 = (led_mask3 & 0x04) ? 1 : 0;
}





void handle_song()
{
	do
	{
		if(song_delay >= song_ptr->delay)
		{
			PIE1bits.TMR2IE = 0; // disable interrupt
			switch(song_ptr->osc)
			{
				case 0:
					osc1_period = freqs[song_ptr->freq];
					osc1_volume = song_ptr->volume;
					break;
				case 1:
					osc2_period = freqs[song_ptr->freq];
					osc2_volume = song_ptr->volume;
					break;
				case 2:
					osc3_period = freqs[song_ptr->freq];
					osc3_volume = song_ptr->volume;
					break;
			}
			PIE1bits.TMR2IE = 1; // enable interrupt


			song_ptr++;
			song_delay = 0;

			if(song_ptr->osc == 0xff)
			{
				if(alarm_sounding)
				{
// repeat or stop if alarm
					if(alarm_time > MAX_ALARM_TIME)
					{
						song_ptr = 0;
						alarm_sounding = 0;
						alarm_time = 0;
					}
					else
					{
						song_ptr = alarm_song;
					}
				}
				else
				{
					song_ptr = 0;
				}
			}
		}
	} while(song_ptr && song_ptr->delay == 0);
}



void stop_song()
{
	osc1_volume = 0;
	osc2_volume = 0;
	osc3_volume = 0;
	song_ptr = 0;
}


void play_song(const song_t *ptr)
{
	PIE1bits.TMR2IE = 0; // disable interrupt
	osc1_period = 0;
	osc1_volume = 0;
	osc2_period = 0;
	osc2_volume = 0;
	osc3_period = 0;
	osc3_volume = 0;
	PIE1bits.TMR2IE = 1; // enable interrupt

	song_ptr = ptr;
	song_counter = 0;
	song_delay = 0;
	handle_song();
}


void draw_time()
{

	led_mask0 = 0;
	led_mask1 = 0;
	led_mask2 = 0;
	led_mask3 = 0;

// print the time
    if(!not_set || colon || mode == MODE_SET_TIME)
    {
        uint8_t hours10 = hours;
        if(hours > 9)
        {
            hours10 -= 10;
            led_mask0 = 0b00010100;
        }

        const uint8_t *ptr = &led_masks2[hours10];
        led_mask0 |= ptr[0];
        led_mask1 |= ptr[1];
        led_mask2 |= ptr[2];
        led_mask3 |= ptr[3];

        uint8_t minutes10 = minutes / 10;
        ptr = &led_masks3[minutes10];
        led_mask0 |= ptr[0];
        led_mask1 |= ptr[1];
        led_mask2 |= ptr[2];
        led_mask3 |= ptr[3];

        minutes10 = minutes - minutes10 * 10;
        ptr = &led_masks4[minutes10];
        led_mask0 |= ptr[0];
        led_mask1 |= ptr[1];
        led_mask2 |= ptr[2];
        led_mask3 |= ptr[3];

        if(colon || mode == MODE_SET_TIME)
        {
            led_mask1 |= 0b00001000;
            led_mask2 |= 0b00100000;
        }

        if(ampm)
        {
            led_mask0 |= 0b10000000;
        }

        if(alarm)
        {
            led_mask1 |= 0b00000001;
        }
    }
	
	write_leds();
}

void draw_seconds()
{

	led_mask0 = 0;
	led_mask1 = 0;
	led_mask2 = 0;
	led_mask3 = 0;

    if(!not_set || colon)
    {
        uint8_t seconds10 = seconds / 10;
        const uint8_t *ptr = &led_masks3[seconds10];
        led_mask0 |= ptr[0];
        led_mask1 |= ptr[1];
        led_mask2 |= ptr[2];
        led_mask3 |= ptr[3];

        seconds10 = seconds - seconds10 * 10;
        ptr = &led_masks4[seconds10];
        led_mask0 |= ptr[0];
        led_mask1 |= ptr[1];
        led_mask2 |= ptr[2];
        led_mask3 |= ptr[3];


        if(ampm)
        {
            led_mask0 |= 0b10000000;
        }

        if(alarm)
        {
            led_mask1 |= 0b00000001;
        }
    }
	write_leds();
}



void draw_alarm()
{

	led_mask0 = 0;
	led_mask1 = 0;
	led_mask2 = 0;
	led_mask3 = 0;


	uint8_t hours10 = alarm_hours;
	if(alarm_hours > 9)
	{
		hours10 -= 10;
		led_mask0 = 0b00010100;
	}
	
	const uint8_t *ptr = &led_masks2[hours10];
	led_mask0 |= ptr[0];
	led_mask1 |= ptr[1];
	led_mask2 |= ptr[2];
	led_mask3 |= ptr[3];
	
	uint8_t minutes10 = alarm_minutes / 10;
	ptr = &led_masks3[minutes10];
	led_mask0 |= ptr[0];
	led_mask1 |= ptr[1];
	led_mask2 |= ptr[2];
	led_mask3 |= ptr[3];
	
	minutes10 = alarm_minutes - minutes10 * 10;
	ptr = &led_masks4[minutes10];
	led_mask0 |= ptr[0];
	led_mask1 |= ptr[1];
	led_mask2 |= ptr[2];
	led_mask3 |= ptr[3];
	
	led_mask1 |= 0b00001000;
	led_mask2 |= 0b00100000;
	
	if(alarm_ampm)
	{
		led_mask0 |= 0b10000000;
	}
	
	if(alarm)
	{
		led_mask1 |= 0b00000001;
	}
	
	write_leds();
}

void draw_test()
{
	led_mask3 = 0xff;
	led_mask2 = 0xff;
	led_mask1 = 0xff;
	led_mask0 = 0xff;
	write_leds();
}


void increment_time_minutes()
{
	crystal_time = 0;
	display_flash = 0;
	time_hz = 0;
	seconds = 0;
	minutes++;
	if(minutes == 60)
	{
		minutes = 0;
	}
    not_set = 0;
	draw_time();
}

void increment_alarm_minutes()
{
	alarm_minutes++;
	if(alarm_minutes == 60)
	{
		alarm_minutes = 0;
	}
	draw_alarm();
}

void decrement_time_minutes()
{
	crystal_time = 0;
	display_flash = 0;
	time_hz = 0;
	seconds = 0;
	minutes--;
	if(minutes == 0xff)
	{
		minutes = 59;
	}
    not_set = 0;

	draw_time();
}

void decrement_alarm_minutes()
{
	alarm_minutes--;
	if(alarm_minutes == 0xff)
	{
		alarm_minutes = 59;
	}
	draw_alarm();
}

void increment_time_hours()
{
	crystal_time = 0;
	display_flash = 0;
	time_hz = 0;
	seconds = 0;
	hours++;
	if(hours == 12)
	{
		ampm = !ampm;
	}
	if(hours == 13)
	{
		hours = 1;
	}
    not_set = 0;
	draw_time();
}

void increment_alarm_hours()
{
	alarm_hours++;
	if(alarm_hours == 12)
	{
		alarm_ampm = !alarm_ampm;
	}
	if(alarm_hours == 13)
	{
		alarm_hours = 1;
	}
	
	draw_alarm();
}

void decrement_time_hours()
{
	crystal_time = 0;
	display_flash = 0;
	time_hz = 0;
	seconds = 0;
	hours--;
	if(hours == 0)
	{
		hours = 12;
	}
	if(hours == 11)
	{
		ampm = !ampm;
	}
    not_set = 0;
	draw_time();
}

void decrement_alarm_hours()
{
	alarm_hours--;
	if(alarm_hours == 0)
	{
		alarm_hours = 12;
	}
	if(alarm_hours == 11)
	{
		alarm_ampm = !alarm_ampm;
	}
	draw_alarm();
}

uint8_t do_hour_up()
{
	if(mode == MODE_TEST)
	{
		if(led_mask0 == 0xff)
		{
			led_mask0 = 0x01;
			led_mask1 = 0x00;
			led_mask2 = 0x00;
			led_mask3 = 0x00;
			write_leds();
		}
		else
		{
			led_mask3 <<= 1;
			if((led_mask2 & 0x80)) led_mask3 |= 0x1;
			led_mask2 <<= 1;
			if((led_mask1 & 0x80)) led_mask2 |= 0x1;
			led_mask1 <<= 1;
			if((led_mask0 & 0x80)) led_mask1 |= 0x1;
			led_mask0 <<= 1;
			if((led_mask3 & 0x08)) led_mask0 |= 0x1;
			write_leds();
		}
		dump_leds();
        return 1;
	}
	else
	if(mode == MODE_SET_TIME)
	{
		increment_time_hours();
        return 1;
	}
	else
	if(mode == MODE_SET_ALARM)
	{
		increment_alarm_hours();
        return 1;
	}
    return 0;
}


uint8_t do_hour_down()
{
	if(mode == MODE_TEST)
	{
		if(led_mask0 == 0xff)
		{
			led_mask0 = 0x01;
			led_mask1 = 0x00;
			led_mask2 = 0x00;
			led_mask3 = 0x00;
			write_leds();
		}
		else
		{
			if((led_mask0 & 0x01)) led_mask3 |= 0x08;
			led_mask0 >>= 1;
			if((led_mask1 & 0x01)) led_mask0 |= 0x80;
			led_mask1 >>= 1;
			if((led_mask2 & 0x01)) led_mask1 |= 0x80;
			led_mask2 >>= 1;
			if((led_mask3 & 0x01)) led_mask2 |= 0x80;
			led_mask3 >>= 1;
			write_leds();
		}
		dump_leds();
        return 1;
	}
	else
	if(mode == MODE_SET_TIME)
	{
		decrement_time_hours();
        return 1;
	}
	else
	if(mode == MODE_SET_ALARM)
	{
		decrement_alarm_hours();
        return 1;
	}
    return 0;
}


uint8_t do_minute_up()
{
	if(mode == MODE_SET_TIME)
	{
		increment_time_minutes();
        return 1;
	}
	else
	if(mode == MODE_SET_ALARM)
	{
		increment_alarm_minutes();
        return 1;
	}
    return 0;
}

uint8_t do_minute_down()
{
	if(mode == MODE_SET_TIME)
	{
		decrement_time_minutes();
        return 1;
	}
	else
	if(mode == MODE_SET_ALARM)
	{
		decrement_alarm_minutes();
        return 1;
	}
    return 0;
}

void handle_repeat()
{
	switch(ir_code)
	{
		case HOUR_UP:
			do_hour_up();
			break;

		case HOUR_DN:
			do_hour_down();
			break;


 		case MINUTE_UP:
			do_minute_up();
 			break;

 		case MINUTE_DN:
			do_minute_down();
 			break;
	}
}

void handle_ir_code()
{

	switch(ir_code)
	{
		case ENABLE_ALARM:
			alarm = !alarm;
// update the icon
			if(mode == MODE_TIME || 
				mode == MODE_SET_TIME)
			{
				draw_time();
			}
			else
			if(mode == MODE_SET_ALARM)
			{
				draw_alarm();
			}
            else
            if(mode == MODE_SECONDS)
            {
                draw_seconds();
            }

// user interrupted alarm
			if(alarm_sounding)
			{
//								stop_song();
				play_song(alarm_off_tone);
				alarm_sounding = 0;
				alarm_time = 0;
			}
			else
			{
				if(alarm)
				{
					play_song(alarm_on_tone);
				}
				else
				{
					play_song(alarm_off_tone);
				}
			}
			break;


		case SET_ALARM:
			if(mode == MODE_SET_ALARM)
			{
				mode = MODE_TIME;
				play_song(set_alarm_tone2);
				draw_time();
			}
			else
			{
				mode = MODE_SET_ALARM;
				play_song(set_alarm_tone1);
				draw_alarm();
			}
			break;


		case SET_TIME:
			if(mode == MODE_SET_TIME)
			{
				mode = MODE_TIME;
				play_song(set_time_tone2);
				draw_time();
			}
			else
			{
				mode = MODE_SET_TIME;
				play_song(set_time_tone1);
				draw_time();
			}
			break;

        case SECONDS:
            if(mode == MODE_SECONDS)
            {
                mode = MODE_TIME;
                play_song(seconds_tone2);
				draw_time();
            }
            else
            {
                mode = MODE_SECONDS;
                play_song(seconds_tone1);
                draw_seconds();
            }
            break;


 		case SET_TEST:
			play_song(test_tone);
			if(mode == MODE_TEST)
			{
				mode = MODE_TIME;
				draw_time();
			}
			else
			{
				mode = MODE_TEST;
				draw_test();
			}
 			break;


		case HOUR_UP:
        {
			uint8_t result = do_hour_up();
			if(result) play_song(up_tone);
			break;
        }

		case HOUR_DN:
        {
			uint8_t result = do_hour_down();
			if(result) play_song(dn_tone);
			break;
        }


 		case MINUTE_UP:
			if(mode == MODE_TEST)
			{
				play_song(alarm_song);
			}
			else
			{
				uint8_t result = do_minute_up();
				if(result) play_song(up_tone);
 			}
			break;

 		case MINUTE_DN:
			if(mode == MODE_TEST)
			{
				play_song(alarm_song);
			}
			else
			{
				uint8_t result = do_minute_down();
				if(result) play_song(dn_tone);
			}
 			break;
	}

// stop alarm for all keys
	if(alarm_sounding)
	{
		alarm_sounding = 0;
		alarm_time = 0;
	}
}

void handle_ir()
{
    if(ir_timeout == 0)
    {
        code_offset = 0;
        if(have_ir)
        {
            have_ir = 0;
        }
    }

    if(have_ir && 
        repeat_delay == 0 && 
        (ir_code == HOUR_UP ||
        ir_code == HOUR_DN ||
        ir_code == MINUTE_UP ||
        ir_code == MINUTE_DN))
    {
/* print_text("IR repeat\n"); */
        repeat_delay = REPEAT_DELAY2;
        handle_repeat();
    }

    if(got_ir_int)
    {
// uncomment this to capture the raw IR codes
// DEBUG
//print_number_nospace(ir_time);
//print_text(", ");
        got_ir_int = 0;
        ir_timeout = IR_TIMEOUT;
        if(ir_time > IR_THRESHOLD)
            ir_time = 1;
        else
            ir_time = 0;
// uncomment this to get array data
// DEBUG
print_number_nospace(ir_time);
print_text(",");

        if(!have_ir)
        {
            code_buffer[code_offset++] = ir_time;
            const ir_code_t *code;
	        uint8_t got_it = 0;
	        uint8_t i, j;

// end of code
            if(code_offset == CODE_SIZE2)
            {
// search for the code
	            for(i = 0; i < TOTAL_CODES2 && !got_it; i++)
                {
                    code = &ir_codes2[i];
                    const uint8_t *data = code->data;
                    got_it = 1;
                    for(j = 1; j < CODE_SIZE2; j++)
                    {
                        if(data[j] != code_buffer[j])
                        {
                            got_it = 0;
                            break;
                        }
                    }
                }
            }


// end of code
            if(code_offset >= CODE_SIZE)
            {
// search for the code
	            for(i = 0; i < TOTAL_CODES && !got_it; i++)
	            {
                    code = &ir_codes[i];
                    const uint8_t *data = code->data;
                    got_it = 1;
                    for(j = 1; j < CODE_SIZE; j++)
                    {
                        if(data[j] != code_buffer[j])
                        {
                            got_it = 0;
                            break;
                        }
                    }
                }
            }

            if(got_it)
            {
// print_text("i=");
// print_number(i);
// print_text("\n");
				have_ir = 1;
                repeat_delay = REPEAT_DELAY;
				ir_code = code->value;

				print_text("Code=");
				print_number_nospace(ir_code);
				print_byte('\n');
                handle_ir_code();
            }
            else
            if(code_offset >= CODE_SIZE)
            {
                code_offset = 0;
            }
        }
    }
}



void handle_audio()
{

	decay_count++;
	if(decay_count > 512)
	{
		decay_count = 0;
		if(osc1_volume > 0) osc1_volume--;
		if(osc2_volume > 0) osc2_volume--;
		if(osc3_volume > 0) osc3_volume--;
	}

	if(osc1_volume == 0 &&
		osc2_volume == 0 &&
		osc3_volume == 0)
	{
		if(powerdown_count < 8192)
		{
			powerdown_count++;
		}
		else
		if(next_duty > 0)
		{
// decrease duty cycle
			next_duty--;
		}
	}
	else
	{
		powerdown_count = 0;


    	osc1_time += AUDIO_PERIOD;
		osc2_time += AUDIO_PERIOD;
		osc3_time += AUDIO_PERIOD;


#if 1
	// full square wave synthesis
		uint8_t amplitude = MAX_VOLUME;
		if(osc1_time >= osc1_period)
		{
			osc1_time -= osc1_period;
        	amplitude += osc1_volume;
		}
		else
		if(osc1_time >= osc1_period / 2)
		{
			amplitude -= osc1_volume;
		}
		else
		{
			amplitude += osc1_volume;
		}

		if(osc2_time >= osc2_period)
		{
			osc2_time -= osc2_period;
        	amplitude += osc2_volume;
		}
		else
		if(osc2_time >= osc2_period / 2)
		{
			amplitude -= osc2_volume;
		}
		else
		{
			amplitude += osc2_volume;
		}

		if(osc3_time >= osc3_period)
		{
			osc3_time -= osc3_period;
        	amplitude += osc3_volume;
		}
		else
		if(osc3_time >= osc3_period / 2)
		{
			amplitude -= osc3_volume;
		}
		else
		{
			amplitude += osc3_volume;
		}
#endif // 0



// half square wave synthesis
#if 0
		uint8_t amplitude = 0;
		if(osc1_time >= osc1_period)
		{
			osc1_time -= osc1_period;
			amplitude += osc1_volume;
		}
		else
		if(osc1_time <= osc1_period / 2)
		{
			amplitude += osc1_volume;
		}

		if(osc2_time >= osc2_period)
		{
			osc2_time -= osc2_period;
			amplitude += osc2_volume;
		}
		else
		if(osc2_time <= osc2_period / 2)
		{
			amplitude += osc2_volume;
		}

		if(osc3_time >= osc3_period)
		{
			osc3_time -= osc3_period;
			amplitude += osc3_volume;
		}
		else
		if(osc3_time <= osc3_period / 2)
		{
			amplitude += osc3_volume;
		}
#endif // 0


// duty cycle is the amplitude
		next_duty = amplitude;



	}






}


void handle_time()
{
	


// increment the mane time by 1000ms/HZ
	time_hz++;
	if(time_hz == HZ)
	{
		time_hz = 0;
		seconds++;
		if(seconds == 60)
		{
			seconds = 0;
			minutes++;
			if(minutes == 60)
			{
				minutes = 0;
				hours++;
				if(hours == 12)
				{
					ampm = !ampm;
				}
				else
				if(hours >= 13)
				{
					hours = 1;
				}
			}
		}
		
		if(alarm_sounding)
		{
			alarm_time++;
		}

// DEBUG for testclock.c
		print_byte('>');
		print_number2(hours);
		print_byte(':');
		print_number2(minutes);
		print_byte(':');
		print_number2(seconds);
		print_byte('\n');
	}

	display_flash++;
	if(display_flash == HZ / 2)
	{
		display_flash = 0;
		colon = !colon;
		if(mode == MODE_TIME || mode == MODE_SET_TIME)
		{
			draw_time();
		}
        else
        if(mode == MODE_SECONDS)
        {
            draw_seconds();
        }
	}


// update song
	if(song_ptr)
	{
		song_counter++;
		if(song_counter >= 10)
		{
			song_counter = 0;
			song_delay++;
			handle_song();
		}
	}
}


void start_alarm()
{
	alarm_sounding = 1;
	play_song(alarm_song);
	
	
}





uint8_t interrupt_done = 0;
void interrupt isr()
{
    interrupt_done = 0;
    while(!interrupt_done)
    {
        interrupt_done = 1;
        
// UART receive
//         if(PIR1bits.RCIF)
//         {
//             PIR1bits.RCIF = 0;
//             serial_in = RCREG;
//             have_serial = 1;
// 			interrupt_done = 0;
//         }



		if(PIR1bits.TMR2IF)
		{
			PIR1bits.TMR2IF = 0;
			CCPR1L = next_duty;
			handle_audio();
 		}


// IR interrupt
		if(INTCON3bits.INT3IF)
		{
			INTCON3bits.INT3IF = 0;
			got_ir_int = 1;
			ir_time = TMR0L;
			ir_time |= ((uint16_t)TMR0H) << 8;
			TMR0H = 0;
			TMR0L = 0;
			interrupt_done = 0;
		}
    }
    
    
}


void main()
{
    
    ADCON1 = 0xff; // all digital
    DEBUG_TRIS = 0; // output
    DEBUG_LAT = 1;
    
// initialize serial port
    TXSTA = 0x24;
    RCSTA = 0x80; // disable receive
    BAUDCTL = 0x08;
    SPBRGH = BAUD_CODE >> 8;
    SPBRG = BAUD_CODE & 0xff;
    PIR1bits.RCIF = 0;
    PIE1bits.RCIE = 0;

// IR interrupt
	INTCON2bits.INTEDG3 = 1; // interrupt on rising edge
	INTCON3bits.INT3IE = 1;
	INTCON3bits.INT3IF = 0;


// IR timer
	T0CON = 0x85;   // 1:64 prescaling
	INTCONbits.TMR0IF = 0;

	
	
// the mane timer
	T1CON = 0xb1; // 8:1 prescale
	CCPR2H = TIMER_PERIOD >> 8;
	CCPR2L = TIMER_PERIOD & 0xff;
	CCP2CON = 0x0b;   // compare mode, reset timer1 on interrupt
//	PIE2bits.CCP2IE = 1;



// the audio PWM
	osc1_period = freqs[_F3];
	osc2_period = freqs[_A3];
	osc3_period = freqs[_C4];
	osc1_volume = NOTE_VOLUME;
	osc2_volume = NOTE_VOLUME;
	osc3_volume = NOTE_VOLUME;
    T2CON = 0x1c;  // 4:1 postscaler
    PR2 = 0xff; // period
 	CCP1CON = 0x0c; // PWM mode
 	CCPR1L = 0x00; // duty cycle
 	SPEAKER_TRIS = 0;  // speaker on/output mode
	PIE1bits.TMR2IE = 1; // enable interrupt



// the LEDs
	TRISGbits.TRISG0 = 0;  // set all to output
	TRISGbits.TRISG1 = 0;
	TRISFbits.TRISF0 = 0;
	TRISFbits.TRISF1 = 0;
	TRISFbits.TRISF2 = 0;
	TRISFbits.TRISF3 = 0;
	TRISFbits.TRISF4 = 0;
	TRISFbits.TRISF5 = 0;
	TRISFbits.TRISF6 = 0;
	TRISE = 0;
	TRISCbits.TRISC0 = 0;
	TRISCbits.TRISC1 = 0;
	TRISCbits.TRISC3 = 0;
	TRISCbits.TRISC4 = 0;
	TRISCbits.TRISC5 = 0;
	TRISAbits.TRISA0 = 0;
	TRISAbits.TRISA1 = 0;
	TRISAbits.TRISA2 = 0;
	TRISAbits.TRISA3 = 0;
	TRISAbits.TRISA5 = 0;

	led_mask0 = 0x1;
	draw_time();
	


    // enable all interrupts
    INTCONbits.PEIE = 1;
    INTCONbits.GIE = 1;
    
    // wait a while
    uint16_t i;
    for(i = 0; i < 32768; i++)
    {
        ClrWdt();
    }
	
	
	print_text("Welcome to Heroineclock 2\n");

//print_number(sizeof(power_data) / 2);
// print_number(sizeof(volume_up_data) / 2);
// print_number(sizeof(volume_dn_data) / 2);
// print_number(sizeof(next_track_data) / 2);
// print_number(sizeof(prev_track_data) / 2);
// print_number(sizeof(number1_data) / 2);
// print_number(sizeof(number2_data) / 2);
// print_number(sizeof(number3_data) / 2);
// print_byte('\n');
// print_number(_C1);
// print_number(_C2);
// print_number(_C3);
// print_byte('\n');

 	print_text("\n");
   
    while(1)
    {
        ClrWdt();

//         if(have_serial)
// 		{
// 			have_serial = 0;
// 		}


// 		uint16_t test_time = TMR0L;
// 		test_time |= ((uint16_t)TMR0H) << 8;
// // IR timed out
// 		if(test_time > IR_TIMEOUT &&
// 			!first_edge)
// 		{
//             print_text("IR timed out\n");
// 
// 
//     		INTCONbits.GIE = 0;
// 			INTCON2bits.INTEDG3 = 0; // interrupt on falling edge
// 			TMR0H = 0;
// 			TMR0L = 0;
//     		INTCONbits.GIE = 1;
// 
// 			uint8_t i;
// 			ir_size = 0;
// 			for(i = 0; i < TOTAL_CODES; i++)
// 			{
// 				ir_code_failed[i] = 0;
// 			}
//             first_edge = 1;
// 			have_ir = 0;
// 		}

// 		if(got_ir_int)
// 		{
//         	got_ir_int = 0;
// // reverse edge
// 			INTCON2bits.INTEDG3 = !INTCON2bits.INTEDG3;
// 			ir_time = ir_time2;
//     		if(first_edge)
//     		{
//         		first_edge = 0;
//     		}
//     		else
//     		{
//  				handle_ir();
//     		}
// 		}





// mane timer
		if(PIR2bits.CCP2IF)
        {
			PIR2bits.CCP2IF = 0;
			
// oscillator error
			crystal_time++;
// add 1 tick after a certain number of ticks, derived from the observed drift
//			if(crystal_time == 10300)
			if(crystal_time == 19230)
			{
				crystal_time = 0;
				handle_time();
			}

// mane time loop
			handle_time();
			
			if(alarm && 
				!alarm_sounding &&
				hours == alarm_hours &&
				minutes == alarm_minutes &&
				seconds == 0 &&
				ampm == alarm_ampm)
			{
				start_alarm();
			}


            if(ir_timeout > 0) ir_timeout--;
            if(repeat_delay > 0) repeat_delay--;
        }


        handle_ir();
        
#ifdef ENABLE_DEBUG
        // send a UART char
        if(uart_size > 0 && PIR1bits.TXIF)
        {
            PIR1bits.TXIF = 0;
            TXREG = uart_buffer[uart_position2++];
			uart_size--;
			if(uart_position2 >= UART_BUFSIZE)
			{
				uart_position2 = 0;
			}
        }
#endif
    }
    
    
}








