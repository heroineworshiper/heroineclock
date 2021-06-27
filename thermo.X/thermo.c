/*
 * TEMPERATURE DISPLAY
 * Copyright (C) 2020 Adam Williams <broadcast at earthling dot net>
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

// indoor temperature thermometer
// build in MPLab
// ground PGC to enter test mode

#include <p18f6585.h>
#include <stdint.h>


// send temperature to a radio for logging
// debugging must be enabled to use the serial port
//#define DO_INT_TRANSMITTER


// Receive temperature from radio instead of using ADC
#define DO_RECEIVER


// disable test mode if pickit grounds PGC
#define DISABLE_TEST

//#pragma config OSC = HSPLL  // Oscillator (HS oscillator with 4x PLL)
#pragma config OSC = HS         // Oscillator (HS oscillator)
#pragma config LVP = OFF    // Low Voltage Program (Low-voltage ICSP disabled)


// pins:
// 
// A0/AN0 - THERMISTER
// A1 - LED
// A2 - LED
// A3 - LED
// A5 - LED
// C0 - LED
// C1 - LED
// C2 - LED
// C3 - LED
// C6/TX - DEBUG UART
// C7/RX - 

// F0 - LED
// F1 - LED
// F2 - LED
// F3 - LED
// F4 - LED
// F5 - LED
// G0 - LED
// G1 - LED


// ADC readings
// -11.8C = 59190
// 9.3C =  51000
// 28.4C = 41850 * bang on
// 91.3C = 13119
// 93.4C = 12125


#define MAX_R 18530
#define MIN_R 1190
#define MAX_ADC 51000
#define MIN_ADC 12125

// C * 10 to resistance in ohms
typedef struct
{
    int16_t t;
    uint16_t adc;
} table_t;


// radio shack thermistor
// adc values were calculated by temp_tester.c
table_t temp_table[] =
{
    { -200, 60972 },
    { -150, 59865 },
    { -100, 58572 },
    { -50, 57054 },
    { 0, 55330 },
    { 50, 53369 },
    { 100, 51214 },
    { 150, 48837 },
    { 200, 46316 },
    { 250, 43648 },
    { 300, 40882 },
    { 350, 38057 },
    { 400, 35233 },
    { 450, 32445 },
    { 500, 29737 },
    { 550, 27125 },
    { 600, 24659 },
    { 650, 22335 },
    { 700, 20187 },
    { 750, 18192 },
    { 800, 16385 },
    { 850, 14726 },
    { 900, 14048 },
    { 950, 11876 },
    { 1000, 10342 },
    { 1050, 9589 },
    { 1100, 8618 },
};






#define TABLE_SIZE (sizeof(temp_table) / sizeof(table_t))






#define ENABLE_DEBUG

#ifdef ENABLE_DEBUG
#define DEBUG_TRIS TRISDbits.TRISD7
#define DEBUG_LAT LATDbits.LATD7

// the clockspeed
//#define CLOCKSPEED 40000000
#define CLOCKSPEED 10000000

// 115200 baud for debug
//#define BAUD_CODE 21   // 10Mhz
//#define BAUD_CODE 87   // 40Mhz
//#define BAUD_CODE 104  // 48Mhz

// 8192 baud for radio
#define BAUD_CODE 303

#define UART_BUFSIZE 1024
uint8_t uart_buffer[UART_BUFSIZE];
uint16_t uart_size = 0;
uint16_t uart_position1 = 0;
uint16_t uart_position2 = 0;
#endif // ENABLE_DEBUG

uint32_t adc_accum = 0;
uint32_t adc_count = 0;



#ifdef DO_RECEIVER
// key for packets coming from outside
// const uint8_t PACKET_KEY[] = 
// {
//     0xff, 0x98, 0xdf, 0x72, 0x36, 0xb9, 0x0d, 0x48, 
//     0x82, 0xc9, 0x28, 0x31, 0x2f, 0x56, 0xe5, 0x7c
// };

// key for packets coming from inside, for testing
// const uint8_t PACKET_KEY[] = 
// {
//     0xff, 0x5c, 0xf8, 0x98, 0xc6, 0xe8, 0xdc, 0x41, 
//     0x2b, 0x96, 0xbe, 0x7c, 0xd3, 0x7a, 0xc6, 0xf2
// };


// key for packets from the base station
const uint8_t PACKET_KEY[] = 
{
    0xff, 0x8d, 0x4a, 0xe0, 0x84, 0x09, 0xd6, 0xb2,
    0xd6, 0x70, 0xb1, 0x7b, 0xbd, 0x06, 0x6b, 0x2c
};

// salt for radio data
const uint8_t salt[] = 
{
    0x80, 0x59, 0x4a, 0xb7, 0x39, 0xbe, 0x73, 0x51
};

uint8_t have_serial;
uint8_t serial_in;
uint8_t key_offset;
uint8_t data_offset;
// get the temp but ignore the voltage
#define DATA_SIZE 4
uint8_t serial_data[DATA_SIZE];
void (*handle_serial)();

uint8_t timeout_counter;
// number of seconds without a packet before invalidating temperature
#define TIMEOUT_MAX 60
#endif // DO_RECEIVER


// masks for the LEDs
#define ALL_LEDS0 0b00101110
#define ALL_LEDS1 0b00001111
#define ALL_LEDS2 0b00111111
#define ALL_LEDS3 0b00000011

// current values for ports A, C, F, G
uint8_t led_mask0 = 0;
uint8_t led_mask1 = 0;
uint8_t led_mask2 = 0;
uint8_t led_mask3 = 0;

// ports A, C, F, G

#define SET_LEDS(src) \
led_mask0 |= src[0]; \
led_mask1 |= src[1]; \
led_mask2 |= src[2]; \
led_mask3 |= src[3];


const uint8_t invalid_mask[4] = 
{
//  port  A,          C,           F,         G
    0b00001000, 0b00000010, 0b00000000, 0b00000000 // --
};

// left digit
const uint8_t led_masks1[][4] = 
{
//  port  A,          C,           F,         G
    { 0b00000000, 0b00000000, 0b00110000, 0b00000000 }, // 1
};



// digit 2
const uint8_t led_masks2[][4] = 
{
//  port  A,          C,           F,         G
	{ 0b00000110, 0b00001100, 0b00001100, 0b00000000 }, // 0
	{ 0b00000010, 0b00000100, 0b00000000, 0b00000000 }, // 1
	{ 0b00001010, 0b00001000, 0b00001100, 0b00000000 }, // 2
	{ 0b00001010, 0b00001100, 0b00000100, 0b00000000 }, // 3
	{ 0b00001110, 0b00000100, 0b00000000, 0b00000000 }, // 4
	{ 0b00001100, 0b00001100, 0b00000100, 0b00000000 }, // 5
	{ 0b00001100, 0b00001100, 0b00001100, 0b00000000 }, // 6
	{ 0b00000010, 0b00001100, 0b00000000, 0b00000000 }, // 7
	{ 0b00001110, 0b00001100, 0b00001100, 0b00000000 }, // 8
	{ 0b00001110, 0b00001100, 0b00000100, 0b00000000 }, // 9
};

// digit 3
const uint8_t led_masks3[][4] = 
{
//  port  A,          C,           F,         G
	{ 0b00100000, 0b00000001, 0b00000011, 0b00000011 }, // 0
	{ 0b00000000, 0b00000000, 0b00000000, 0b00000011 }, // 1
	{ 0b00100000, 0b00000010, 0b00000011, 0b00000010 }, // 2
	{ 0b00000000, 0b00000010, 0b00000011, 0b00000011 }, // 3
	{ 0b00000000, 0b00000011, 0b00000000, 0b00000011 }, // 4
	{ 0b00000000, 0b00000011, 0b00000011, 0b00000001 }, // 5
	{ 0b00100000, 0b00000011, 0b00000011, 0b00000001 }, // 6
	{ 0b00000000, 0b00000000, 0b00000001, 0b00000011 }, // 7
	{ 0b00100000, 0b00000011, 0b00000011, 0b00000011 }, // 8
	{ 0b00000000, 0b00000011, 0b00000011, 0b00000011 }, // 9
};





// mane timer interrupts at this rate
#define HZ 250
// mane timer period - 1 to account for interrupt behavior
#define TIMER_PERIOD (CLOCKSPEED / 4 / 8 / HZ - 1)

#define ABS(x) ((x) < 0 ? (-(x)) : (x))

// counter for 1 second.  Incremented HZ times per second
uint16_t time_hz = 0;
// the display
uint8_t degrees = 0xff;
uint8_t invalid = 1;

#ifdef DO_INT_TRANSMITTER
    // updates between transmitter packets
    #define TRANSMITTER_PERIOD 30
    uint16_t transmitter_count;
#endif // DO_INT_TRANSMITTER




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


void print_number_nospace(uint32_t number)
{
	if(number >= 10000000) print_byte('0' + (number / 10000000));
	if(number >= 1000000) print_byte('0' + (number / 1000000));
	if(number >= 100000) print_byte('0' + (number / 100000));
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

void print_number(uint32_t number)
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
    LATA = led_mask0;
    LATC = led_mask1;
    LATF = led_mask2;
    LATG = led_mask3;
}





void draw_degrees()
{
    uint16_t temp = degrees;
	led_mask0 = 0;
	led_mask1 = 0;
	led_mask2 = 0;
	led_mask3 = 0;

    if(invalid)
    {
        SET_LEDS(invalid_mask)
    }
    else
    {

        uint8_t force = 0;
        if(degrees > 99)
        {
            degrees %= 100;
            force = 1;
            SET_LEDS(led_masks1[0])
        }

        if(degrees > 9 || force)
        {
            int index = degrees / 10;
            SET_LEDS(led_masks2[index])
            degrees %= 10;
        }

        SET_LEDS(led_masks3[degrees])
	}

	write_leds();
    degrees = temp;
}



char test_state = 0;
void handle_test()
{
    static uint8_t test_table[] = 
    {
        0, 11, 22, 33, 44, 55, 66, 77, 88, 99, 100
    };
    static char test_index = 0;
    const uint8_t test_mask[][4] = 
    {
//  port  A,          C,           F,         G
        { 0b00000000, 0b00000000, 0b00000000, 0b00000010 }, 
        { 0b00000000, 0b00000000, 0b00000000, 0b00000001 }, 
        { 0b00000000, 0b00000000, 0b00000010, 0b00000000 }, 
        { 0b00100000, 0b00000000, 0b00000000, 0b00000000 }, 
        { 0b00000000, 0b00000001, 0b00000000, 0b00000000 }, 
        { 0b00000000, 0b00000000, 0b00000001, 0b00000000 }, 
        { 0b00000000, 0b00000010, 0b00000000, 0b00000000 }, 
        
        { 0b00000010, 0b00000000, 0b00000000, 0b00000000 }, 
        { 0b00000000, 0b00000100, 0b00000000, 0b00000000 }, 
        { 0b00000000, 0b00000000, 0b00000100, 0b00000000 }, 
        { 0b00000000, 0b00000000, 0b00001000, 0b00000000 }, 
        { 0b00000100, 0b00000000, 0b00000000, 0b00000000 }, 
        { 0b00000000, 0b00001000, 0b00000000, 0b00000000 }, 
        { 0b00001000, 0b00000000, 0b00000000, 0b00000000 }, 
        
        { 0b00000000, 0b00000000, 0b00100000, 0b00000000 }, 
        { 0b00000000, 0b00000000, 0b00010000, 0b00000000 }, 
        
        { 0b00000000, 0b00000000, 0b00100000, 0b00000000 }, 

        { 0b00001000, 0b00000000, 0b00000000, 0b00000000 }, 
        { 0b00000000, 0b00001000, 0b00000000, 0b00000000 }, 
        { 0b00000100, 0b00000000, 0b00000000, 0b00000000 }, 
        { 0b00000000, 0b00000000, 0b00001000, 0b00000000 }, 
        { 0b00000000, 0b00000000, 0b00000100, 0b00000000 }, 
        { 0b00000000, 0b00000100, 0b00000000, 0b00000000 }, 
        { 0b00000010, 0b00000000, 0b00000000, 0b00000000 }, 

        { 0b00000000, 0b00000010, 0b00000000, 0b00000000 }, 
        { 0b00000000, 0b00000000, 0b00000001, 0b00000000 }, 
        { 0b00000000, 0b00000001, 0b00000000, 0b00000000 }, 
        { 0b00100000, 0b00000000, 0b00000000, 0b00000000 }, 
        { 0b00000000, 0b00000000, 0b00000010, 0b00000000 }, 
        { 0b00000000, 0b00000000, 0b00000000, 0b00000001 }, 
        { 0b00000000, 0b00000000, 0b00000000, 0b00000010 }, 


        { 0b00000000, 0b00000000, 0b00000000, 0b00000011 }, 
        { 0b00100000, 0b00000001, 0b00000000, 0b00000000 }, 
        { 0b00000010, 0b00000100, 0b00000000, 0b00000000 }, 
        { 0b00000100, 0b00000000, 0b00001000, 0b00000000 }, 
        { 0b00000000, 0b00000000, 0b00110000, 0b00000000 }, 

        { 0b00000100, 0b00000000, 0b00001000, 0b00000000 }, 
        { 0b00000010, 0b00000100, 0b00000000, 0b00000000 }, 
        { 0b00100000, 0b00000001, 0b00000000, 0b00000000 }, 
        { 0b00000000, 0b00000000, 0b00000000, 0b00000011 }, 


        { 0b00000000, 0b00001000, 0b00000001, 0b00000000 }, 
        { 0b00001000, 0b00000010, 0b00000000, 0b00000000 }, 
        { 0b00000000, 0b00000000, 0b00000110, 0b00000000 }, 
        
        { 0b00001000, 0b00000010, 0b00000000, 0b00000000 }, 
        { 0b00000000, 0b00001000, 0b00000001, 0b00000000 }, 
        
    };
    const char test_mask_size = sizeof(test_mask) / 4;

    switch(test_state)
    {
        case 0:
	        led_mask0 = ALL_LEDS0;
	        led_mask1 = ALL_LEDS1;
	        led_mask2 = ALL_LEDS2;
	        led_mask3 = ALL_LEDS3;
	        write_leds();
            test_state++;
            time_hz = 0;
            break;

        case 1:
            time_hz++;
            if(time_hz >= HZ * 5)
            {
                test_state++;
                time_hz = 0;
                test_index = 0;
            }
            break;

        case 2:
            time_hz++;
            if(time_hz >= HZ / 2)
            {
                if(test_index < sizeof(test_table))
                {
                    time_hz = 0;
                    degrees = test_table[test_index];
                    test_index++;
                    draw_degrees();
                }
                else
                {
                    time_hz = HZ / 4;
                    test_state++;
                    test_index = 0;
                }
            }
            break;
        
        case 3:
            time_hz++;
            if(time_hz >= HZ / 4)
            {
                time_hz = 0;
                if(test_index < test_mask_size)
                {
	                led_mask0 = test_mask[test_index][0];
	                led_mask1 = test_mask[test_index][1];
	                led_mask2 = test_mask[test_index][2];
	                led_mask3 = test_mask[test_index][3];
                    test_index++;
                    write_leds();
                }
                else
                {
                    test_state = 0;
                }
            }
            break;
        
    }

}

void handle_thermo()
{
	time_hz++;


    

// measurement period complete
	if(time_hz >= HZ)
	{
		time_hz = 0;

        
        
#ifdef DO_INT_TRANSMITTER
        adc_accum = (adc_accum << 6) / adc_count;

        print_text("handle_thermo accum=");
        print_number(adc_accum);
        print_text("count=");
        print_number(adc_count);

// linear resistance
        if(adc_accum > temp_table[0].adc ||
            adc_accum < temp_table[TABLE_SIZE - 1].adc)
        {
            invalid = 1;
        }
        else
        {
            uint8_t i;
            invalid = 0;
            int16_t t = temp_table[TABLE_SIZE - 1].t;
            for(i = 1; i < TABLE_SIZE; i++)
            {
                if(temp_table[i].adc < adc_accum)
                {
                    print_text("i=");
                    print_number(i);
                    int32_t t_high = temp_table[i].t;
                    int32_t adc_high = temp_table[i - 1].adc;

                    int32_t t_low = temp_table[i - 1].t;
                    int32_t adc_low = temp_table[i].adc;

                    t = t_high - (adc_accum - adc_low) * (t_high - t_low) / (adc_high - adc_low);
                    break;
                }
            }

// C to F
            degrees = t * 9 / 5 / 10 + 32;

            print_text("C=");
            print_number(t);
            print_text("F=");
            print_number(degrees);

// send packet to radio
            transmitter_count++;
            if(transmitter_count >= TRANSMITTER_PERIOD)
            {
                transmitter_count = 0;
                print_byte(0xff);
                print_byte(0xe7);
                print_byte(degrees);
            }
        }
        print_byte('\n');
#else // DO_INT_TRANSMITTER
        if(timeout_counter < TIMEOUT_MAX)
        {
            timeout_counter++;
        }
        else
        {
            invalid = 1;
        }
#endif


// always erase the test program
        draw_degrees();

        
        adc_accum = 0;
        adc_count = 0;

    }
}


#ifdef DO_RECEIVER

void get_key();
void get_temp()
{
// get data from radio
    serial_data[data_offset] = serial_in ^ salt[data_offset];
    data_offset++;
    if(data_offset >= DATA_SIZE)
    {
        key_offset = 0;
        handle_serial = get_key;
        
        uint8_t i;
        uint8_t failed = 0;
        for(i = 1; i < DATA_SIZE; i++)
        {
            if(serial_data[0] != serial_data[i])
            {
// reject packet if any value is different
                failed = 1;
                break;
            }
        }

// new temperature reading
        if(!failed)
        {
            timeout_counter = 0;
            degrees = serial_data[0];
            if(degrees == 0xff)
            {
                invalid = 1;
            }
            else
            {
                invalid = 0;
            }
            

            print_text("get_temp ");
            print_text("F=");
            print_number(degrees);
            print_byte('\n');
            draw_degrees();
        }
    }
}

void get_key()
{
// get packet key from radio
    if(serial_in == PACKET_KEY[key_offset])
    {
        key_offset++;
        if(key_offset >= sizeof(PACKET_KEY))
        {
            handle_serial = get_temp;
            data_offset = 0;
        }
    }
    else
    if(serial_in == PACKET_KEY[0])
    {
        key_offset = 1;
    }
    else
    if(key_offset > 0)
    {
//         print_text("get_key ");
//         print_number(key_offset);
//         print_byte('\n');
        key_offset = 0;
    }
}
#endif // DO_RECEIVER


uint8_t interrupt_done = 0;
void interrupt isr()
{
    interrupt_done = 0;
    while(!interrupt_done)
    {
        interrupt_done = 1;

#ifdef DO_RECEIVER
// UART receive
        if(PIR1bits.RCIF)
        {
            PIR1bits.RCIF = 0;
            serial_in = RCREG;
            have_serial = 1;
			interrupt_done = 0;
        }
#endif // DO_RECEIVER
    }
    
    
}


void main()
{
// all digital except AN0
    ADCON1 = 0b00001110; 
    DEBUG_TRIS = 0; // output
    DEBUG_LAT = 1;


#ifdef DO_INT_TRANSMITTER
    ADCON0 = 0b00000001;
    ADCON2 = 0b10111110;
// start the conversion
    ADCON0bits.GO = 1;
#endif // DO_INT_TRANSMITTER


// initialize serial port
    TXSTA = 0b00100100;
    BAUDCTL = 0x08;
    SPBRGH = BAUD_CODE >> 8;
    SPBRG = BAUD_CODE & 0xff;
    PIR1bits.RCIF = 0;
#ifdef DO_INT_TRANSMITTER
    RCSTA = 0b10000000; // disable receive
    PIE1bits.RCIE = 0; // disable interrupt
#else
    have_serial = 0;
    handle_serial = get_key;
    key_offset = 0;
    RCSTA = 0b10010000; // enable receive
    PIE1bits.RCIE = 1; // enable interrupt
    timeout_counter = 0;
#endif


	
	
// the mane timer
	T1CON = 0xb1; // 8:1 prescale
	CCPR2H = TIMER_PERIOD >> 8;
	CCPR2L = TIMER_PERIOD & 0xff;
	CCP2CON = 0x0b;   // compare mode, reset timer1 on interrupt
//	PIE2bits.CCP2IE = 1;




// enable the LEDs
    TRISA = ~ALL_LEDS0;
    TRISC = ~ALL_LEDS1;
    TRISF = ~ALL_LEDS2;
    TRISG = ~ALL_LEDS3;
	handle_test();

// Enable test mode by pulling down PGC
    TRISBbits.TRISB6 = 1;
    LATBbits.LATB6 = 1;
    INTCON2bits.RBPU = 0;

#ifdef DO_INT_TRANSMITTER
    transmitter_count = 0;
#endif // DO_INT_TRANSMITTER





    // enable all interrupts
    INTCONbits.PEIE = 1;
    INTCONbits.GIE = 1;
    
    // wait a while
    uint16_t i;
    for(i = 0; i < 32768; i++)
    {
        ClrWdt();
    }
	
	
	print_text("Welcome to Thermometer 2\n");

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

#ifdef DO_RECEIVER
        if(have_serial)
 		{
//print_byte(serial_in);
 			have_serial = 0;
            handle_serial();
 		}
#endif // DO_RECEIVER


// mane timer

		if(PIR2bits.CCP2IF)
        {
			PIR2bits.CCP2IF = 0;

// not in test mode
#ifndef DISABLE_TEST
            if(PORTBbits.RB6)
#else
            if(1)
#endif
            {
// mane time loop
			    handle_thermo();
            }
            else
            {
                handle_test();
            }
        }


#ifdef DO_INT_TRANSMITTER
// ADC
        if(PIR1bits.ADIF)
        {
            PIR1bits.ADIF = 0;
            
            int16_t value = ADRESL | (ADRESH << 8);
            adc_accum += value;
            adc_count++;
            
            
            
            ADCON0bits.GO = 1;
        }
#endif // DO_INT_TRANSMITTER



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








