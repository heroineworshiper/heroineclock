/*
 * RADIO MODULE FOR TEMPERATURE SENSING
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


// Solar powered remote temperature sensor
// Read thermister every minute & transmit the value.
// temp_sensor.pcb

// solar powered exterior sensor
// uses a timer to trigger transmissions
// 150uA idle  300uA during ADC reading  30mA when transmitting
//#define DO_EXT_TRANSMITTER

// manes powered interior board attached to interior readout
// interior readout sends a packet to the uart to trigger a transmission
//#define DO_INT_TRANSMITTER

// manes powered receiver attached to LED panel
// puts the radio in receive mode & sleeps
#define DO_RECEIVER


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <xc.h>
#include <pic18f14k50.h>




// PIC18F14K50 Configuration Bit Settings

// 'C' source line config statements


// CONFIG1H
//#pragma config FOSC = IRCCLKOUT // Oscillator Selection bits (Internal RC oscillator, CLKOUT function on OSC2)
#pragma config FOSC = IRC // Oscillator Selection bits (Internal RC oscillator, GPIO on OSC2)
#pragma config PLLEN = OFF      // 4 X PLL Enable bit (PLL is under software control)

// CONFIG2L
#pragma config PWRTEN = OFF     // Power-up Timer Enable bit (PWRT disabled)
#pragma config BOREN = SBORDIS  // Brown-out Reset Enable bits (Brown-out Reset enabled in hardware only (SBOREN is disabled))
#pragma config BORV = 22        // Brown-out Reset Voltage bits (VBOR set to 3.0 V nominal)

// CONFIG2H
#pragma config WDTEN = ON       // Watchdog Timer Enable bit (WDT is always enabled. SWDTEN bit has no effect.)
#pragma config WDTPS = 32768    // Watchdog Timer Postscale Select bits (1:32768)

// CONFIG3H
#pragma config HFOFST = OFF     // HFINTOSC Fast Start-up bit (The system clock is held off until the HFINTOSC is stable.)

// CONFIG4L
#pragma config LVP = OFF        // Single-Supply ICSP Enable bit (Single-Supply ICSP disabled)




// #pragma config statements should precede project file includes.
// Use project enums instead of #define for ON and OFF.


// delay between ADC readings
#define TIMER0_VALUE 0xe000

#define SYNC_CODE0 0xff
#define SYNC_CODE1 0xe7

#ifdef DO_EXT_TRANSMITTER
    #define CLOCKSPEED 32768

// timer0 interrupts between transmissions
// limited by t_accumulator size
    #define MANE_PERIOD 30

    const uint8_t PACKET_KEY[] = 
    {
        0xff, 0x98, 0xdf, 0x72, 0x36, 0xb9, 0x0d, 0x48, 
        0x82, 0xc9, 0x28, 0x31, 0x2f, 0x56, 0xe5, 0x7c
    };

    #define DEBUG_LAT LATCbits.LATC2
    #define DEBUG_TRIS TRISCbits.TRISC2

    #define RADIO_CS_LAT LATCbits.LATC6
    #define RADIO_CS_TRIS TRISCbits.TRISC6

    #define RADIO_SDO_LAT LATCbits.LATC4
    #define RADIO_SDO_TRIS TRISCbits.TRISC4

    #define RADIO_SCK_LAT LATCbits.LATC3
    #define RADIO_SCK_TRIS TRISCbits.TRISC3

    // ground for thermister
    #define T_LAT LATCbits.LATC5
    #define T_TRIS TRISCbits.TRISC5

    uint8_t adc_skip;
//    uint16_t debug;
#endif

#ifdef DO_INT_TRANSMITTER
    #define CLOCKSPEED 2000000
    #define MANE_PERIOD 2

    #define DEBUG_LAT LATCbits.LATC4
    #define DEBUG_TRIS TRISCbits.TRISC4

    const uint8_t PACKET_KEY[] = 
    {
        0xff, 0x5c, 0xf8, 0x98, 0xc6, 0xe8, 0xdc, 0x41, 
        0x2b, 0x96, 0xbe, 0x7c, 0xd3, 0x7a, 0xc6, 0xf2
    };
#endif

#ifdef DO_RECEIVER
    #define CLOCKSPEED 32768
    #define DEBUG_LAT LATCbits.LATC2
    #define DEBUG_TRIS TRISCbits.TRISC2
#endif


// delay to warm up the radio is 10ms
#define RADIO_DELAY -(CLOCKSPEED / 4 / 100)
// number of times to resend the packet
#define RESENDS 4
// maximum for the solar powered one
#define BAUD 8192

#if defined(DO_RECEIVER) || defined(DO_INT_TRANSMITTER)
    #define RADIO_CS_LAT LATCbits.LATC3
    #define RADIO_CS_TRIS TRISCbits.TRISC3

    #define RADIO_SDO_LAT LATCbits.LATC7
    #define RADIO_SDO_TRIS TRISCbits.TRISC7

    #define RADIO_SCK_LAT LATCbits.LATC6
    #define RADIO_SCK_TRIS TRISCbits.TRISC6
#endif // DO_RECEIVER, DO_INT_TRANSMITTER


// RADIO_CHANNEL is from 96-3903 & set by the user
// data rate must be slow enough to service FIFOs
// kbps = 10000 / (29 * (DRVSREG<6:0> + 1) * (1 + DRPE * 7))
// RADIO_BAUD_CODE = 10000 / (29 * kbps) / (1 + DRPE * 7) - 1
// RADIO_DATA_SIZE is the amount of data to read before resetting the sync code

#define RADIO_CHANNEL 3903
// DEBUG
//#define RADIO_CHANNEL 96

// scan for synchronous code
#define FIFORSTREG 0xCA81
// read continuously
//#define FIFORSTREG              (0xCA81 | 0x0004)
// 915MHz
#define FREQ_BAND 0x0030
// Center Frequency: 915.000MHz
#define CFSREG (0xA000 | RADIO_CHANNEL)
// crystal load 10pF
#define XTAL_LD_CAP 0x0003
// power management page 16
#define PMCREG 0x8201
#define GENCREG (0x8000 | XTAL_LD_CAP | FREQ_BAND)


// +3/-4 Fres
//#define AFCCREG 0xc4f7
// +15/-16 Fres
#define AFCCREG 0xc4d7

// Data Rate
// data rate must be slow enough to service FIFOs
// kbps = 10000 / (29 * (DRVSREG<6:0> + 1) * (1 + DRPE * 7))
// RADIO_BAUD_CODE = 10000 / (29 * kbps) - 1
#define RADIO_BAUD_CODE 3

// data rate prescaler.  Divides data rate by 8 if 1
//#define DRPE (1 << 7)
#define DRPE 0
#define DRVSREG (0xC600 | DRPE | RADIO_BAUD_CODE)


// Page 37 of the SI4421 datasheet gives optimum bandwidth values
// but the lowest that works is 200khz
//#define RXCREG 0x9481     // BW 200KHz, LNA gain 0dB, RSSI -97dBm
//#define RXCREG 0x9440     // BW 340KHz, LNA gain 0dB, RSSI -103dBm
#define RXCREG 0x9420       // BW 400KHz, LNA gain 0dB, RSSI -103dBm

//#define TXCREG 0x9850     // FSK shift: 90kHz
#define TXCREG 0x98f0       // FSK shift: 165kHz
#define STSREG 0x0000
#define RXFIFOREG 0xb000

// analog filter for raw mode
#define BBFCREG                 0xc23c


// C * 10 to resistance in ohms
typedef struct
{
    int16_t t;
    uint16_t adc;
} table_t;


// radio shack thermistor
// adc values were calculated by temp_tester.c
// table_t temp_table[] =
// {
//     { -100, 58629 },
//     { -50,  57109 },
//     { 0,	55383 },
//     { 50,	53421 },
//     { 100,	51263 },
//     { 150,	48884 },
//     { 200,	46361 },
//     { 250,	43690 },
//     { 300,	40921 },
//     { 350,	38093 },
//     { 400,	35267 },
//     { 450,	32476 },
//     { 500,	29766 },
//     { 550,	27152 },
//     { 600,	24682 },
//     { 650,	22357 },
//     { 700,	20207 },
//     { 750,	18210 },
//     { 800,	16400 },
//     { 850,	14740 },
//     { 900,	14062 },
//     { 950,	11888 },
//     { 1000, 10352 },
//     { 1050, 9598  },
//     { 1100, 8627  }
// };


// LITTELFUSE DC103G9G
table_t temp_table[] =
{
    { -200, 61804 },
    { -150, 60734 },
    { -100, 59440 },
    { -50, 57902 },
    { 0, 56109 },
    { 50, 54058 },
    { 100, 51760 },
    { 150, 49233 },
    { 200, 46515 },
    { 250, 43648 },
    { 300, 40681 },
    { 350, 37673 },
    { 400, 34676 },
    { 450, 31740 },
    { 500, 28911 },
    { 550, 26224 },
    { 600, 23700 },
    { 650, 21359 },
    { 700, 19202 },
    { 750, 17238 },
    { 800, 15455 },
    { 850, 13846 },
    { 900, 12406 },
    { 950, 11111 },
    { 1000, 9968 },
    { 1050, 8942 },
    { 1100, 8030 },
};





#define TABLE_SIZE (sizeof(temp_table) / sizeof(table_t))


typedef union 
{
	struct
	{
		unsigned interrupt_complete : 1;
	};
	
	unsigned char value;
} flags_t;

flags_t flags;
uint16_t mane_counter;
// character from the serial port
uint8_t serial_c;
void (*serial_function)();
// temperature accumulator
uint32_t t_accum;
// voltage accumulator
uint32_t v_accum;
// total ADC readings
uint32_t adc_count;
// temperature in F.  0xff is invalid
uint8_t degrees;
// volts * 10.  0xff is invalid
uint8_t volts;


void write_radio(uint16_t data)
{
    ClrWdt();

    RADIO_CS_LAT = 0;
    uint8_t i;
    for(i = 0; i < 16; i++)
    {
        RADIO_SDO_LAT = (uint8_t)((data & 0x8000) ? 1 : 0);
        data <<= 1;
        RADIO_SCK_LAT = 1;
        RADIO_SCK_LAT = 0;
    }
    RADIO_CS_LAT = 1;
}

void radio_on()
{
// enable outputs
    RADIO_CS_LAT = 1;
    RADIO_CS_TRIS = 0;
    
    RADIO_SDO_LAT = 0;
    RADIO_SDO_TRIS = 0;
    
    RADIO_SCK_LAT = 0;
    RADIO_SCK_TRIS = 0;

// scan for synchronous code
    write_radio(FIFORSTREG);
// enable synchron latch
    write_radio(FIFORSTREG | 0x0002);
    write_radio(GENCREG);
    write_radio(AFCCREG);
    write_radio(CFSREG);
    write_radio(DRVSREG);
    write_radio(PMCREG);
    write_radio(RXCREG);
    write_radio(TXCREG);
    write_radio(BBFCREG);
// turn on the transmitter to tune
    write_radio(PMCREG | 0x0020);

// warm up
    DEBUG_LAT = 1;
    INTCONbits.TMR0IF = 0;
    TMR0 = RADIO_DELAY;
    while(!INTCONbits.TMR0IF)
    {
        ClrWdt();
    }
    INTCONbits.TMR0IF = 0;
    DEBUG_LAT = 0;

// receive mode
#ifdef DO_RECEIVER
    write_radio(PMCREG | 0x0080);
#endif
}

#ifndef DO_RECEIVER
void radio_off()
{
    RADIO_CS_LAT = 1;
    RADIO_CS_TRIS = 0;
    
    RADIO_SDO_LAT = 0;
    RADIO_SDO_TRIS = 0;
    
    RADIO_SCK_LAT = 0;
    RADIO_SCK_TRIS = 0;
    
// disable radio
    write_radio(PMCREG);

    RADIO_CS_TRIS = 1;
    RADIO_SDO_TRIS = 1;
    RADIO_SCK_TRIS = 1;
}

void serial_on()
{
// serial port
    TXSTA = 0b00100100;
#ifdef DO_INT_TRANSMITTER
    RCSTA = 0b10010000;
#else
    RCSTA = 0b10000000;
#endif
    BAUDCON = 0b00001000;
// baud = clockspeed / (4 * (SPBRG + 1))
    SPBRG = CLOCKSPEED / 4 / BAUD - 1;
}

void flush_serial()
{
    ClrWdt();
    while(!PIR1bits.TXIF)
    {
    }

    while(!TXSTAbits.TRMT)
    {
    }
}

void serial_off()
{
    flush_serial();

    TXSTA = 0x0;
    RCSTA = 0x0;
}

void write_serial(uint8_t value)
{
    ClrWdt();

    while(!PIR1bits.TXIF)
    {
    }

    TXREG = value;    
}


#endif // !DO_RECEIVER

#ifdef DO_INT_TRANSMITTER
void get_sync_code0();
void get_temp()
{
    serial_function = get_sync_code0;

// send temperature to the radio
    radio_on();
    uint8_t i, j;
// retransmit packet
    for(j = 0; j < RESENDS; j++)
    {
        for(i = 0; i < sizeof(PACKET_KEY); i++)
        {
            write_serial(PACKET_KEY[i]);
        }
        for(i = 0; i < 4; i++)
        {
            write_serial(serial_c);
        }
    }
    flush_serial();
    
    radio_off();
}


void get_sync_code1()
{
    if(serial_c == SYNC_CODE1)
    {
        serial_function = get_temp;
    }
    else
    if(serial_c == SYNC_CODE0)
    {
    }
    else
    {
        serial_function = get_sync_code0;
    }
}

void get_sync_code0()
{
    if(serial_c == SYNC_CODE0)
    {
        serial_function = get_sync_code1;
    }
}
#endif // DO_INT_TRANSMITTER

void main()
{
// sleep mode must be idle
#if defined(DO_EXT_TRANSMITTER) || defined(DO_RECEIVER)
// solar powered
// 32.768 khz 150uA
    OSCCON = 0b10000000;
#endif

#ifdef DO_INT_TRANSMITTER
// slow enough to drive SPI yet fast enough 
// to capture serial packets
// 2Mhz
    OSCCON = 0b11000000;
#endif

    DEBUG_LAT = 0;
    DEBUG_TRIS = 0;

    mane_counter = 0;
	flags.value = 0;

    t_accum = 0;
    adc_count = 0;
    degrees = 0xff;
    volts = 0xff;

// digital mode
#ifndef DO_EXT_TRANSMITTER
    ANSEL = 0b00000000;
    ANSELH = 0b00000000;
#endif


#ifdef DO_EXT_TRANSMITTER
    adc_skip = 0;
    ANSEL = 0b00001000;
    ANSELH = 0b00000010;

// thermister ground
    T_TRIS = 1;
    T_LAT = 0;


// mane timer
    T0CON = 0b10001000;
    TMR0 = TIMER0_VALUE;

    INTCON = 0b11100000;
    
    radio_off();
#endif // DO_EXT_TRANSMITTER


#ifdef DO_INT_TRANSMITTER
    serial_function = get_sync_code0;
// use serial port to receive temperature
    serial_on();
// use mane timer without interrupts to control radio
    T0CON = 0b10001000;
    INTCON = 0b00000000;
    radio_off();
#endif // DO_INT_TRANSMITTER


#ifdef DO_RECEIVER
// mane timer
    T0CON = 0b10001000;
    TMR0 = TIMER0_VALUE;
    radio_on();
#endif // DO_RECEIVER

    while(1)
    {
        ClrWdt();
#ifdef DO_EXT_TRANSMITTER
        Sleep();
#endif

#ifdef DO_INT_TRANSMITTER
// got serial port data
        if(PIR1bits.RCIF)
        {
//DEBUG_LAT = 1;
            serial_c = RCREG;
            serial_function();
        }
#endif
    }
    
}

#ifdef DO_EXT_TRANSMITTER
void interrupt isr()
{
	while(1)
	{
		ClrWdt();
		flags.interrupt_complete = 1;

		if(INTCONbits.TMR0IF)
		{
			INTCONbits.TMR0IF = 0;
			flags.interrupt_complete = 0;
//            DEBUG_LAT = !DEBUG_LAT;


            if(adc_skip < 1)
            {
                adc_skip++;
            }
            else
            {
// take a measurement
                T_TRIS = 0;
    // ADC on
    // Use AN3 for thermister
                ADCON0 = 0b00001101;
                ADCON2 = 0b10111110;
                PIR1bits.ADIF = 0;
                ADCON0bits.GO = 1;
                ClrWdt();
                while(!PIR1bits.ADIF)
                {
                }
                t_accum += ADRES;

    // Use AN9 for voltage
                ADCON0 = 0b00100101;
                PIR1bits.ADIF = 0;
                ADCON0bits.GO = 1;
                ClrWdt();
                while(!PIR1bits.ADIF)
                {
                }
                v_accum += ADRES;

                adc_count++;
                T_TRIS = 1;

    // ADC off
                ADCON0 = 0b00000000;
            }

            mane_counter++;
            if(mane_counter >= MANE_PERIOD)
            {
                mane_counter = 0;

// convert ADC to temperature
                t_accum = (t_accum << 6) / adc_count;
                if(t_accum > temp_table[0].adc ||
                    t_accum < temp_table[TABLE_SIZE - 1].adc)
                {
                    degrees = 0xff;
                }
                else
                {
                    char i;
                    int16_t t = temp_table[TABLE_SIZE - 1].t;
                    for(i = 1; i < TABLE_SIZE; i++)
                    {
                        if(temp_table[i].adc < t_accum)
                        {
                            int32_t t_high = temp_table[i].t;
                            int32_t adc_high = temp_table[i - 1].adc;

                            int32_t t_low = temp_table[i - 1].t;
                            int32_t adc_low = temp_table[i].adc;

                            t = t_high - (t_accum - adc_low) * (t_high - t_low) / (adc_high - adc_low);
                            break;
                        }
                    }

// C to F
                    degrees = t * 9 / 5 / 10 + 32;
                }


// convert ADC to volts * 10
                v_accum = (v_accum << 6) / adc_count;
//                debug = v_accum;
// fudge for resistor error
//                v_accum = v_accum * 985 / 1000;
                volts = v_accum * 33 * 15 / 5 / ((uint16_t)1023 << 6);

                radio_on();

                serial_on();
                uint8_t i, j;
// retransmit packet
                for(j = 0; j < RESENDS; j++)
                {
                    for(i = 0; i < sizeof(PACKET_KEY); i++)
                    {
                        write_serial(PACKET_KEY[i]);
                    }
                    for(i = 0; i < 4; i++)
                    {
                        write_serial(degrees);
                    }
                    for(i = 0; i < 4; i++)
                    {
                        write_serial(volts);
//                        write_serial(debug >> 8);
//                        write_serial(debug);
                    }
                }
                
                t_accum = 0;
                v_accum = 0;
                adc_count = 0;
                adc_skip = 0;
                serial_off();

                radio_off();
                
                
            }
            TMR0 = TIMER0_VALUE;

        }

		if(flags.interrupt_complete) break;
    }
}

#endif // DO_EXT_TRANSMITTER









