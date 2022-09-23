/*
 * Lion readable Hygrometer display
 *
 * Copyright (C) 2022 Adam Williams <broadcast at earthling dot net>
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



// CONFIG1H
#pragma config OSC = HS         // Oscillator Selection bits (HS oscillator)
#pragma config OSCS = OFF       // Oscillator System Clock Switch Enable bit (Oscillator system clock switch option is disabled (main oscillator is source))

// CONFIG2L
#pragma config PWRT = ON        // Power-up Timer Enable bit (PWRT enabled)
#pragma config BOR = ON         // Brown-out Reset Enable bit (Brown-out Reset enabled)
#pragma config BORV = 25        // Brown-out Reset Voltage bits (VBOR set to 2.5V)

// CONFIG2H
#pragma config WDT = ON         // Watchdog Timer Enable bit (WDT enabled)
#pragma config WDTPS = 128      // Watchdog Timer Postscale Select bits (1:128)

// CONFIG4L
#pragma config STVR = ON        // Stack Full/Underflow Reset Enable bit (Stack Full/Underflow will cause Reset)
#pragma config LVP = OFF        // Low-Voltage ICSP Enable bit (Low-Voltage ICSP disabled)

// CONFIG5L
#pragma config CP0 = OFF        // Code Protection bit (Block 0 (000200-001FFFh) not code protected)
#pragma config CP1 = OFF        // Code Protection bit (Block 1 (002000-003FFFh) not code protected)
#pragma config CP2 = OFF        // Code Protection bit (Block 2 (004000-005FFFh) not code protected)
#pragma config CP3 = OFF        // Code Protection bit (Block 3 (006000-007FFFh) not code protected)

// CONFIG5H
#pragma config CPB = OFF        // Boot Block Code Protection bit (Boot Block (000000-0001FFh) not code protected)
#pragma config CPD = OFF        // Data EEPROM Code Protection bit (Data EEPROM not code protected)

// CONFIG6L
#pragma config WRT0 = OFF       // Write Protection bit (Block 0 (000200-001FFFh) not write protected)
#pragma config WRT1 = OFF       // Write Protection bit (Block 1 (002000-003FFFh) not write protected)
#pragma config WRT2 = OFF       // Write Protection bit (Block 2 (004000-005FFFh) not write protected)
#pragma config WRT3 = OFF       // Write Protection bit (Block 3 (006000-007FFFh) not write protected)

// CONFIG6H
#pragma config WRTC = OFF       // Configuration Register Write Protection bit (Configuration registers (300000-3000FFh) not write protected)
#pragma config WRTB = OFF       // Boot Block Write Protection bit (Boot Block (000000-0001FFh) not write protected)
#pragma config WRTD = OFF       // Data EEPROM Write Protection bit (Data EEPROM not write protected)

// CONFIG7L
#pragma config EBTR0 = OFF      // Table Read Protection bit (Block 0 (000200-001FFFh) not protected from Table Reads executed in other blocks)
#pragma config EBTR1 = OFF      // Table Read Protection bit (Block 1 (002000-003FFFh) not protected from Table Reads executed in other blocks)
#pragma config EBTR2 = OFF      // Table Read Protection bit (Block 2 (004000-005FFFh) not protected from Table Reads executed in other blocks)
#pragma config EBTR3 = OFF      // Table Read Protection bit (Block 3 (006000-007FFFh) not protected from Table Reads executed in other blocks)

// CONFIG7H
#pragma config EBTRB = OFF      // Boot Block Table Read Protection bit (Boot Block (000000-0001FFh) not protected from Table Reads executed in other blocks)

// #pragma config statements should precede project file includes.
// Use project enums instead of #define for ON and OFF.

#include <xc.h>


#include <pic18lf458.h>
#include <stdint.h>
#include <string.h>



// AHT20 stuff
#define I2C_ADDRESS (0x38 << 1)
#define AHTX0_I2CADDR_DEFAULT 0x38   ///< AHT default i2c address
#define AHTX0_CMD_CALIBRATE 0xE1     ///< Calibration command
#define AHTX0_CMD_TRIGGER 0xAC       ///< Trigger reading command
#define AHTX0_CMD_SOFTRESET 0xBA     ///< Soft reset command
#define AHTX0_STATUS_BUSY 0x80       ///< Status bit for busy
#define AHTX0_STATUS_CALIBRATED 0x08 ///< Status bit for calibrated

#define CLK_LAT LATA5
#define DAT_LAT LATA4

#define CLK_PORT PORTAbits.RA5
#define DAT_PORT PORTAbits.RA4

#define CLK_TRIS TRISA5
#define DAT_TRIS TRISA4


typedef struct
{
    float h;
    float t;
} aht20_t;
aht20_t sensor;

uint8_t i2c_buffer[8];
#define I2C_OAR1_ADD0 ((uint32_t)0x00000001)
#define I2C_7BIT_ADD_WRITE(__ADDRESS__) ((uint8_t)((__ADDRESS__) & (~I2C_OAR1_ADD0)))
#define I2C_7BIT_ADD_READ(__ADDRESS__) ((uint8_t)((__ADDRESS__) | I2C_OAR1_ADD0))


uint8_t tick;
#define HZ 16

typedef union 
{
	struct
	{
		unsigned interrupt_complete : 1;
	};
	
	uint8_t value;
} flags_t;
flags_t flags;

// masks for the LEDs
#define ALL_LEDS0 0b00111111 // PORTC
#define ALL_LEDS1 0b11111111 // PORTD

uint8_t led_mask0;
uint8_t led_mask1;

// left digit
const uint8_t led_masks1[][2] = 
{
//  port  C,          D,
	{ 0b00100000, 0b01111100, }, // 0
	{ 0b00100000, 0b00001000, }, // 1
	{ 0b00110000, 0b00110100, }, // 2
	{ 0b00110000, 0b00011100, }, // 3
	{ 0b00110000, 0b01001000, }, // 4
	{ 0b00010000, 0b01011100, }, // 5
	{ 0b00010000, 0b01111100, }, // 6
	{ 0b00100000, 0b00011000, }, // 7
	{ 0b00110000, 0b01111100, }, // 8
	{ 0b00110000, 0b01011100, }, // 9
};

// right digit
const uint8_t led_masks2[][2] = 
{
//  port  C,          D,      
	{ 0b00001101, 0b10000011, }, // 0
	{ 0b00000100, 0b10000000, }, // 1
	{ 0b00001111, 0b00000001, }, // 2
	{ 0b00001111, 0b10000000, }, // 3
	{ 0b00000110, 0b10000010, }, // 4
	{ 0b00001011, 0b10000010, }, // 5
	{ 0b00001011, 0b10000011, }, // 6
	{ 0b00001100, 0b10000000, }, // 7
	{ 0b00001111, 0b10000011, }, // 8
	{ 0b00001111, 0b10000010, }, // 9
};


#define UART_BUFSIZE 64
uint8_t uart_buffer[UART_BUFSIZE];
uint8_t uart_size = 0;
uint8_t uart_position1 = 0;
uint8_t uart_position2 = 0;


// send a UART char
void handle_uart()
{
// must use TRMT instead of TXIF on the 16F1508
    if(uart_size > 0 && TXIF)
    {
        TXIF = 0;
        TXREG = uart_buffer[uart_position2++];
		uart_size--;
		if(uart_position2 >= UART_BUFSIZE)
		{
			uart_position2 = 0;
		}
    }
}

void flush_uart()
{
    while(uart_size > 0)
        handle_uart();
}


void send_uart(uint8_t c)
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

static uint16_t number;
static int force;
void print_digit(uint16_t place)
{
	if(number >= place || force)
	{ 
		force = 1; 
		send_uart('0' + number / place); 
		number %= place; 
	}
}

void print_number(uint16_t number_arg)
{
	number = number_arg;
	force = 0;
	print_digit(10000000);
	print_digit(1000000);
	print_digit(100000);
	print_digit(10000);
	print_digit(1000);
	print_digit(100);
	print_digit(10);
	send_uart('0' + (number % 10)); 
	send_uart(' ');
}

void print_text(const uint8_t *s)
{
	while(*s != 0)
	{
		send_uart(*s);
		s++;
	}
}

void draw_value()
{
    uint8_t value = (uint8_t)(sensor.h + 0.5);
    led_mask0 = 0;
    led_mask1 = 0;
#define SET_LEDS(src) \
    led_mask0 |= src[0]; \
    led_mask1 |= src[1]; \

    SET_LEDS(led_masks1[value / 10]);
    SET_LEDS(led_masks2[value % 10]);

// DEBUG
//     static uint8_t test_value = 0;
//     SET_LEDS(led_masks1[test_value / 10]);
//     SET_LEDS(led_masks2[test_value % 10]);
//     test_value += 11;
//     if(test_value > 99)
//         test_value = 0;

    LATC = led_mask0;
    LATD = led_mask1;
}

#define I2C_DELAY \
{  \
    asm("nop");  \
    asm("nop");  \
    asm("nop");  \
    asm("nop");  \
    asm("nop");  \
    asm("nop");  \
    asm("nop");  \
    asm("nop");  \
    asm("nop");  \
    asm("nop");  \
}

void set_data(uint8_t value)
{
    DAT_TRIS = value;
    I2C_DELAY
}

void set_clock(uint8_t value)
{
    CLK_TRIS = value;
    I2C_DELAY
}

uint8_t get_data()
{
    return DAT_PORT;
}

uint8_t get_clock()
{
    return CLK_PORT;
}

void i2c_write(uint8_t value)
{
    uint8_t i = 0;
    for(i = 0; i < 8; i++)
    {
        if((value & 0x80))
        {
            set_data(1);
        }
        else
        {
            set_data(0);
        }
        set_clock(1);
        set_clock(0);
        value <<= 1;
    }

// read ACK
    set_clock(1);
// wait for clock to rise
    while(!get_clock())
    {
        ;
    }
    uint8_t ack = get_data();
    set_clock(0);
}

void i2c_read(uint8_t bytes)
{
    uint8_t i, j;
    for(i = 0; i < bytes; i++)
    {
        uint8_t value = 0;

/* data must rise before clock to read the byte */
        set_data(1);

        for(j = 0; j < 8; j++)
        {
            value <<= 1;
            set_clock(1);
            while(!get_clock())
            {
            }
            
            value |= get_data();
            set_clock(0);
        }
        
        i2c_buffer[i] = value;
        
// write ACK
        if(i >= bytes - 1)
        {
            set_data(1);
        }
        else
        {
            set_data(0);
        }

// pulse clock
        set_clock(1);
        set_clock(0);
    }
}

void i2c_start()
{
	set_clock(1);
	set_data(1);
    set_data(0); 
	set_clock(0);
}

void i2c_stop()
{
    set_data(0);
    set_clock(1);
    set_data(1);
}



void i2c_read_device(unsigned char reg, uint8_t bytes)
{
	uint8_t i;
	for(i = 0; i < bytes; i++)
	{
		i2c_buffer[i] = 0xff;
	}

    i2c_start();
// write device address & reg
    i2c_write(I2C_7BIT_ADD_WRITE(I2C_ADDRESS));
    i2c_write(reg);

    i2c_start();
    i2c_write(I2C_7BIT_ADD_READ(I2C_ADDRESS));
    i2c_read(bytes);
    i2c_stop();
}

void i2c_read_device2(uint8_t bytes)
{
	uint8_t i;
	for(i = 0; i < bytes; i++)
	{
		i2c_buffer[i] = 0xff;
	}

    i2c_start();

    i2c_write(I2C_7BIT_ADD_READ(I2C_ADDRESS));
    i2c_read(bytes);

    i2c_stop();
}


void i2c_write_device(unsigned char reg, unsigned char value)
{
// start
    i2c_start();

// write device address
    i2c_write(I2C_7BIT_ADD_WRITE(I2C_ADDRESS));
    i2c_write(reg);
    i2c_write(value);
    
    i2c_stop();
}

void i2c_write_device2(uint8_t len)
{
// start
    i2c_start();

// write device address
    i2c_write(I2C_7BIT_ADD_WRITE(I2C_ADDRESS));
    uint8_t i;
    for(i = 0; i < len; i++)
    {
        i2c_write(i2c_buffer[i]);
    }
    
    i2c_stop();
}

uint8_t aht20_status()
{
    i2c_read_device2(1);
// TRACE
// print_number(i2c_buffer[0]);
// print_lf();
// flush_uart();
    return i2c_buffer[0];
}

void read_aht20(aht20_t *ptr)
{
    i2c_buffer[0] = AHTX0_CMD_TRIGGER;
    i2c_buffer[1] = 0x33;
    i2c_buffer[2] = 0x00;
    i2c_write_device2(3);

// poll just the status
    while(aht20_status() & AHTX0_STATUS_BUSY)
    {
        ;
    }

// read status + 5 bytes of data without the CRC
    i2c_read_device2(6);
    int32_t x = i2c_buffer[1];
    x <<= 8;
    x |= i2c_buffer[2];
    x <<= 4;
    x |= i2c_buffer[3] >> 4;
    ptr->h = (float)(x * 100) / 0x100000;

    x = i2c_buffer[3] & 0x0f;
    x <<= 8;
    x |= i2c_buffer[4];
    x <<= 8;
    x |= i2c_buffer[5];
    ptr->t = (float)(x * 200) / 0x100000 - 50;
}

void init_aht20(aht20_t *ptr)
{
    i2c_buffer[0] = AHTX0_CMD_SOFTRESET;
    i2c_write_device2(1);

    i2c_buffer[0] = AHTX0_CMD_CALIBRATE;
    i2c_buffer[1] = 0x08;
    i2c_buffer[2] = 0x00;
    i2c_write_device2(3);

    while(aht20_status() & AHTX0_STATUS_BUSY)
    {
        ;
    }
}

void main()
{
// digital pin mode
    ADCON1 = 0b00000111;

// serial port
    TXSTA = 0b00100100;
    RCSTA = 0b10000000;
// 65535 baud with 4.1943 Mhz crystal
    SPBRG = 3;

// mane clock
    T2CON = 0b01111111;
    TMR2IE = 1;
    PR2 = 0xff;

// LED pins
    TRISC = ~ALL_LEDS0;
    TRISD = ~ALL_LEDS1;
// turn them all on
    LATC = ALL_LEDS0;
    LATD = ALL_LEDS1;

    print_text("Welcome to Huge Hygrometer\n");
    flush_uart();

// I2C
    CLK_LAT = 0;
    DAT_LAT = 0;
    CLK_TRIS = 1;
    DAT_TRIS = 1;
    print_text("Starting AHT20\n");
    flush_uart();
    init_aht20(&sensor);

// enable interrupts
    GIE = 1;
    PEIE = 1;
    print_text("MAne loop\n");
    flush_uart();

    while(1)
    {
        asm("clrwdt");
        handle_uart();
        if(tick >= HZ)
        {
            tick = 0;
            
            read_aht20(&sensor);
            print_text("TEMP=");
            print_number((uint16_t)sensor.t);
            print_text("HUMIDITY=");
            print_number((uint16_t)(sensor.h + 0.5));
            print_text("\n");


            draw_value();
//            print_text(".");
        }
    }
}


void interrupt isr()
{
    flags.interrupt_complete = 0;
	while(!flags.interrupt_complete)
	{
		flags.interrupt_complete = 1;
        if(TMR2IF)
        {
            TMR2IF = 0;
            tick++;
            flags.interrupt_complete = 0;
        }
    }
}















