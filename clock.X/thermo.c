// thermometer 2
// build in MPLab



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



#pragma config OSC = HSPLL  // Oscillator (HS oscillator with 4x PLL)
#pragma config LVP = OFF    // Low Voltage Program (Low-voltage ICSP disabled)


#include <p18f6585.h>
#include <stdint.h>





#define ENABLE_DEBUG

// the clockspeed
#define CLOCKSPEED 40000000


#ifdef ENABLE_DEBUG
#define DEBUG_TRIS TRISDbits.TRISD7
#define DEBUG_LAT LATDbits.LATD7


// 115200 baud for debug
#define BAUD_CODE 87   // 40Mhz

#define UART_BUFSIZE 1024
uint8_t uart_buffer[UART_BUFSIZE];
uint16_t uart_size = 0;
uint16_t uart_position1 = 0;
uint16_t uart_position2 = 0;
#endif // ENABLE_DEBUG






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





// mane timer period - 1 to account for interrupt behavior
#define TIMER_PERIOD (CLOCKSPEED / 4 / 8 / 250 - 1)
// mane timer interrupts at this rate
#define HZ 250

#define ABS(x) ((x) < 0 ? (-(x)) : (x))

// counter for 1 second.  Incremented HZ times per second
uint8_t time_hz = 0;
// the displayed number
uint8_t degrees = 0;



// values for ports A, C, F, G
uint8_t led_mask0 = 0;
uint8_t led_mask1 = 0;
uint8_t led_mask2 = 0;
uint8_t led_mask3 = 0;



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
	LATA = led_mask0;
	LATC = led_mask1;
	LATF = led_mask2;
	LATG = led_mask3;
}




void draw_degrees()
{

	led_mask0 = 0;
	led_mask1 = 0;
	led_mask2 = 0;
	led_mask3 = 0;

// print the temperature
    if(!not_set || colon || mode == MODE_SET_TIME)
    {
        uint8_t degrees100 = degrees;
        if(degrees100 > 99)
        {
            degrees100 -= 100;
            led_mask0 = 0b00000000;
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

void do_hour_up()
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
	}
	else
	if(mode == MODE_SET_TIME)
	{
		increment_time_hours();
	}
	else
	if(mode == MODE_SET_ALARM)
	{
		increment_alarm_hours();
	}
}


void do_hour_down()
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
	}
	else
	if(mode == MODE_SET_TIME)
	{
		decrement_time_hours();
	}
	else
	if(mode == MODE_SET_ALARM)
	{
		decrement_alarm_hours();
	}
}


void do_minute_up()
{
	if(mode == MODE_SET_TIME)
	{
		increment_time_minutes();
	}
	else
	if(mode == MODE_SET_ALARM)
	{
		increment_alarm_minutes();
	}
}

void do_minute_down()
{
	if(mode == MODE_SET_TIME)
	{
		decrement_time_minutes();
	}
	else
	if(mode == MODE_SET_ALARM)
	{
		decrement_alarm_minutes();
	}
}

void handle_repeat()
{
	if(!have_ir)
	{
		repeating = 0;
		repeat_counter = 0;
	}
	else
	{
		repeat_counter--;
		if(repeat_counter == 0)
		{
			repeat_counter = REPEAT_COUNT2;
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
	}
}

void handle_ir()
{
//print_number_nospace(ir_time);
//print_text(", ");


// biggest error encountered in code
	uint16_t ir_error = 0;

// search for the code
	uint8_t i, j;
// test all bytes so far against every code
	uint8_t got_it = 0;
	for(j = 0; j < sizeof(ir_codes) / sizeof(ir_code_t); j++)
//	for(j = 0; j < 1; j++)
	{
// code has matched all previous bytes
		if(!ir_code_failed[j])
		{
			const ir_code_t *code = &ir_codes[j];
			const int16_t *data = code->data;
			const uint8_t code_size = code->size;
			uint8_t failed = 0;
			ir_error = 0;

// test latest byte
			int16_t error = ABS(data[ir_size] - ir_time);
			if(error > ir_error)
			{
				ir_error = error;
			}

// reject code if latest byte doesn't match
			if(error > IR_MARGIN)
			{
// 	 			print_byte('\n');
// 

// 				print_number_nospace(data[ir_size]);
// 				print_text(" != ");
// 				print_number_nospace(ir_time);
// 				print_text(" ");
//   			print_text("failed at ir_size=");
// 				print_number(ir_size);
// 				print_byte('\n');

// don't search this code anymore
				ir_code_failed[j] = 1;
			}
			else
// all bytes so far matched the current code
			{
				ir_size++;

// complete code was received
				if(ir_size >= code_size)
				{
// reset the code search
					ir_size = 0;
					for(i = 0; i < TOTAL_CODES; i++)
					{
						ir_code_failed[i] = 0;
					}

					have_ir = 1;
					ir_code = code->value;
					print_text("IR code: ");
					print_number(code->value);
					print_text("error=");
					print_number(ir_error);
					print_byte('\n');


					switch(code->value)
					{
						case ENABLE_ALARM:
							alarm = !alarm;
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
							do_hour_up();
							repeat_counter = REPEAT_COUNT1;
							repeating = 1;
							play_song(up_tone);
							break;

						case HOUR_DN:
							do_hour_down();
							repeat_counter = REPEAT_COUNT1;
							repeating = 1;
							play_song(dn_tone);
							break;
							
							
 						case MINUTE_UP:
							if(mode == MODE_TEST)
							{
								play_song(alarm_song);
							}
							else
							{
								do_minute_up();
								repeat_counter = REPEAT_COUNT1;
								repeating = 1;
								play_song(up_tone);
 							}
							break;
							
 						case MINUTE_DN:
							if(mode == MODE_TEST)
							{
								play_song(alarm_song);
							}
							else
							{
								do_minute_down();
								repeat_counter = REPEAT_COUNT1;
								repeating = 1;
								play_song(dn_tone);
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

// exit the code search
				got_it = 1;
				break;
			}
		}
	}

// no partial code was found, so discard
	if(!got_it)
	{
//print_text("no matches\n");
		uint8_t i;
		ir_size = 0;
		for(i = 0; i < TOTAL_CODES; i++)
		{
			ir_code_failed[i] = 0;
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
			ir_time2 = TMR0L;
			ir_time2 |= ((uint16_t)TMR0H) << 8;
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
	INTCON2bits.INTEDG3 = 0; // interrupt on falling edge
	INTCON3bits.INT3IE = 1;
	INTCON3bits.INT3IF = 0;
	ir_size = 0;
    first_edge = 1;


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


		uint16_t test_time = TMR0L;
		test_time |= ((uint16_t)TMR0H) << 8;
// IR timed out
		if(test_time > IR_TIMEOUT &&
			!first_edge)
		{
            print_text("IR timed out\n");


    		INTCONbits.GIE = 0;
			INTCON2bits.INTEDG3 = 0; // interrupt on falling edge
			TMR0H = 0;
			TMR0L = 0;
    		INTCONbits.GIE = 1;

			uint8_t i;
			ir_size = 0;
			for(i = 0; i < TOTAL_CODES; i++)
			{
				ir_code_failed[i] = 0;
			}
            first_edge = 1;
			have_ir = 0;
		}

		if(got_ir_int)
		{
        	got_ir_int = 0;
// reverse edge
			INTCON2bits.INTEDG3 = !INTCON2bits.INTEDG3;
			ir_time = ir_time2;
    		if(first_edge)
    		{
        		first_edge = 0;
    		}
    		else
    		{
 				handle_ir();
    		}
		}





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
			if(repeating)
			{
				handle_repeat();
			}
			
			
			if(alarm && 
				!alarm_sounding &&
				hours == alarm_hours &&
				minutes == alarm_minutes &&
				seconds == 0 &&
				ampm == alarm_ampm)
			{
				start_alarm();
			}
			
        }



        
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








