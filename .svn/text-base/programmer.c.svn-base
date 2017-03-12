#include "parapin.h"
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>


pthread_mutex_t parport_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_t charge_pump;



// LP_PIN01     hi - programming voltage on MCLR
//              lo - ground MCLR
// LP_PIN16     hi - enable ground on MCLR
//              lo - disable ground on MCLR
// LP_PIN14     charge pump oscillator
// LP_PIN02     programming data output
// LP_PIN15     programming data input
// LP_PIN17     programming clock

#define LOAD_PROGRAM_DATA 0x2
#define READ_PROGRAM_DATA 0x4
#define INCREMENT_ADDRESS 0x6
#define BEGIN_ERASE_PROGRAM 0x8
#define END_PROGRAMMING 0x17

#define DELAY 300
#define COMMAND_DELAY 0
#define ERASE_DELAY 10000
#define CHARGE_PUMP_DELAY 2

void lock_port()
{
	pthread_mutex_lock(&parport_lock);
}

void unlock_port()
{
	pthread_mutex_unlock(&parport_lock);
}

void* charge_pump_thread(void *ptr)
{
	while(1)
	{
		lock_port();
		set_pin(LP_PIN14);
		unlock_port();
		usleep(100000);
		lock_port();
		clear_pin(LP_PIN14);
		unlock_port();
		usleep(100000);
	}
}

void send_command(unsigned char command)
{
	int i;
	int debug = 0;

// Clock in command
	if(debug)
	{
		printf("send_command %02x ", command);
		fflush(stdout);
	}
	for(i = 0; i < 6; i++)
	{
		lock_port();
		set_pin(LP_PIN17);
		if((command & 0x1))
		{
			set_pin(LP_PIN02);
			if(debug) printf("1");
		}
		else
		{
			clear_pin(LP_PIN02);
			if(debug) printf("0");
		}
		unlock_port();
		if(debug) fflush(stdout);

		usleep(DELAY);


		lock_port();
		clear_pin(LP_PIN17);
		unlock_port();

		usleep(DELAY);
		command >>= 1;
	}
	if(debug) printf("\n");
	usleep(COMMAND_DELAY);
}

void send_data(uint16_t data)
{
	int i;
	int debug = 0;

	if(debug)
	{
		printf("send_data %04x ", data);
		fflush(stdout);
	}

	data <<= 1;
	for(i = 0; i < 16; i++)
	{
		int bit = data & 0x1;
		if(debug) printf("%d", bit);

		lock_port();
		set_pin(LP_PIN17);
		if(bit)
			set_pin(LP_PIN02);
		else
			clear_pin(LP_PIN02);
		unlock_port();

		if(debug) fflush(stdout);
		usleep(DELAY);
		
		lock_port();
		clear_pin(LP_PIN17);
		unlock_port();

		usleep(DELAY);




		data >>= 1;
	}

	lock_port();
	clear_pin(LP_PIN02);
	unlock_port();

	if(debug) printf("\n");
	usleep(COMMAND_DELAY);
}

uint16_t read_data()
{
	uint16_t result = 0;
	int i;
	int debug = 1;
	int bit;


	clear_pin(LP_PIN02);
	for(i = 0; i < 16; i++)
	{
		result >>= 1;

		lock_port();
		set_pin(LP_PIN17);
		unlock_port();

		usleep(DELAY);

		lock_port();
		clear_pin(LP_PIN17);
// Pin is connected through N inverting MOSFET
		bit = pin_is_set(LP_PIN15) ? 0x0 : 0x8000;
		unlock_port();


		result |= bit;
		if(debug)
		{
			printf("%d", bit ? 1 : 0);
			fflush(stdout);
		}
		usleep(DELAY);
	}
	if(debug) printf("\n");
	result >>= 1;

	return result;
}


void start_programming_mode()
{
	printf("Entering program mode\n");
// Enable grounding of MCLR
	lock_port();
	set_pin(LP_PIN16);
// Reset CPU
	clear_pin(LP_PIN01);
	unlock_port();

	usleep(100000);

// Set programming mode
	lock_port();
	clear_pin(LP_PIN02);
	clear_pin(LP_PIN17);
	set_pin(LP_PIN01);
	unlock_port();

	usleep(100000);
}

int main(int argc, char *argv[])
{
	int i, j;

	if(argc < 2)
	{
		printf("Need a program to upload.\n");
		exit(1);
	}

	FILE *in = fopen(argv[1], "r");
	if(!in)
	{
		perror("fopen");
		exit(1);
	}

	uint16_t *data = (uint16_t*)calloc(sizeof(uint16_t), 16384);
	char string[1024];
	int page = 0;
// Top address in bytes
	int max_address = 0;
	int min_address = 0xffff;

	while(fgets(string, 1024, in))
	{
// Convert string to binary
		unsigned char string2[512];
		unsigned char *in_ptr = string + 1;
		unsigned char *out_ptr = string2;
		for(i = 0; i < strlen(string); i += 2)
		{
			int character = *in_ptr++;
			if(character >= '0' && character <= '9')
				*out_ptr = (character - '0') << 4;
			else
				*out_ptr = (10 + character - 'A') << 4;

			character = *in_ptr++;
			if(character >= '0' && character <= '9')
				*out_ptr |= character - '0';
			else
				*out_ptr |= 10 + character - 'A';
			out_ptr++;
		}


		unsigned char *ptr = string2;

// Number of bytes of data in the line
		int data_bytes = *ptr++;
		if(!data_bytes) break;

// Starting address of data
		int address = (*ptr++) << 8;
		address |= *ptr++;

		int type = *ptr++;

// Data is the number of a page
		if(type == 4)
		{
			page = (*ptr++) << 8;
			page |= *ptr++;
			if(page != 0)
			{
				printf("page = %d  Don't know what to do.\n", page);
			}
		}
		else
// Data is program data
		{
			if(min_address > address)
				min_address = address;
			for(i = 0; i < data_bytes; i += 2)
			{
				uint16_t word = *ptr++;
				word |= (*ptr++) << 8;
				data[address >> 1] = word;
				address += 2;
			}
			if(max_address < address)
				max_address = address;
		}

// Checksum
		ptr++;
	}


// Dump memory
/*
 * for(i = 0; i < max_address / 2; i++)
 * {
 * 	uint16_t word = data[i];
 * 	printf("%04x: ", i);
 * 	printf("%04x ", word);
 * 	for(j = 0; j < 16; j++)
 * 	{
 * 		printf("%d", (word & 0x8000) ? 1 : 0);
 * 		word <<= 1;
 * 	}
 * 	printf("\n");
 * }
 */

	int total_bytes = max_address - min_address;
	int current_bytes = 0;


	struct sched_param params;
	params.sched_priority = 1;
	sched_setscheduler(0, SCHED_RR, &params);

	pin_init_user(LPT1);
	pin_output_mode(LP_DATA_PINS | LP_SWITCHABLE_PINS);


// Start charge pump
	pthread_create(&charge_pump, 0, charge_pump_thread, 0);
// Wait for charge buildup
	printf("Starting charge pump\n");
	sleep(CHARGE_PUMP_DELAY);

	start_programming_mode();
	printf("Uploading %d words\n", max_address / 2);

	for(j = 0; j < max_address / 2; j += 8)
	{
		for(i = 0; i < 8; i++)
		{
			send_command(LOAD_PROGRAM_DATA);
			send_data(data[j + i]);
			if(i < 7) send_command(INCREMENT_ADDRESS);
		}
		send_command(BEGIN_ERASE_PROGRAM);
		usleep(ERASE_DELAY - COMMAND_DELAY);
		send_command(INCREMENT_ADDRESS);

		current_bytes += 16;
		int dots = 79 * current_bytes / total_bytes;
		for(i = 0; i < dots; i++)
		{
			printf("*");
		}
		printf("\r");
		fflush(stdout);
	}

	printf("\n");

/*
 * 	printf("Downloading program\n");
 * 	for(i = 0; i < 8; i++)
 * 	{
 * start_programming_mode();
 * 		for(j = 0; j < i; j++)
 * 		{
 * 			send_command(INCREMENT_ADDRESS);
 * 		}
 * 		send_command(READ_PROGRAM_DATA);
 * 		uint16_t result = read_data();
 * 		printf("%04x\n", result);
 * 	}
 */



/*
 *  	start_programming_mode();
 * 	for(i = 0; i < 8; i++)
 * 	{
 * 		send_command(READ_PROGRAM_DATA);
 * 		uint16_t result = read_data();
 * 		printf("%04x\n", result);
 * 		usleep(DELAY);
 * 		send_command(INCREMENT_ADDRESS);
 * 	}
 */

 	pthread_cancel(charge_pump);
	pthread_join(charge_pump, 0);

// Shut down charge pump
	clear_pin(LP_PIN14);

// Reset CPU
	clear_pin(LP_PIN01);
	usleep(100000);
// Disable grounding of MCLR.  Puts 5V on MCLR.
	clear_pin(LP_PIN16);
	clear_pin(LP_PIN14);
}

