/*
 * BASE STATION FOR TEMP SENSORS & DESK CONTROL
 * Copyright (C) 2020-2023 Adam Williams <broadcast at earthling dot net>
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

// record temperatures coming from the 2 sensors & transmit wunderground data to
// LED panel.

// scp it to the pi & compile it with gcc
// gcc -O2 -o temp_log temp_log.c -lpthread -lrt

// run it with a file that contains the wunderground commands
// The file contains separate lines with commands to download 
// different weather stations.  The lines should be in order of preference.
// This program runs each line until a download succeeds.

// nohup ./temp_log wunder.txt > /dev/null&

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <unistd.h>
#include <linux/serial.h>
#include <pthread.h>


// ignored for ttyACM
#define BAUD 8192

// number of seconds between logs
#define LOG_INTERVAL (60 * 5)

// key for packets going to panel
const uint8_t PANEL_KEY[] = 
{
    0xff, 0x8d, 0x4a, 0xe0, 0x84, 0x09, 0xd6, 0xb2,
    0xd6, 0x70, 0xb1, 0x7b, 0xbd, 0x06, 0x6b, 0x2c
};

// salt for radio data
const uint8_t salt[] = 
{
    0x80, 0x59, 0x4a, 0xb7, 0x39, 0xbe, 0x73, 0x51
};

#define KEY_SIZE sizeof(EXT_PACKET_KEY)



#define LOG_PATH "/var/climate.log"
#define TEXTLEN 1024
#define REPEATS 4
#define RESENDS 4
// temp only
#define INT_DATA_SIZE REPEATS
// temp & voltage
#define EXT_DATA_SIZE (REPEATS * 2)

unsigned char serial_in;
FILE *log_fd = 0;
struct timespec start_time = { 0 };
struct timespec current_time = { 0 };
struct timespec last_wunder_time = { 0 };
#define MAX_COMMANDS 255
char *wunder_commands[MAX_COMMANDS] = { 0 };
int total_commands = 0;
char wunder_json[TEXTLEN];
uint8_t wunder_packet[KEY_SIZE + REPEATS];
char string[TEXTLEN];
// number of seconds between wunder updates
#define WUNDER_INTERVAL 25

typedef struct
{
    int key_offset;
    const uint8_t *key;
    void (*function)(void *ptr);
    int data_size;
    uint8_t serial_data[EXT_DATA_SIZE];
    int data_offset;
    int min_temp;
    int max_temp;
    int temp_valid;
    const char *title;
    int is_ext;
    float voltage;
} serial_state_t;

serial_state_t ext_state;
serial_state_t int_state;
pthread_mutex_t temp_lock;
int serial_fd = -1;

// Returns the FD of the serial port
static int init_serial(char *path, int baud, int custom_baud)
{
	struct termios term;

	printf("init_serial %d: opening %s\n", __LINE__, path);

// Initialize serial port
	int fd = open(path, O_RDWR | O_NOCTTY | O_SYNC);
	if(fd < 0)
	{
		printf("init_serial %d: path=%s: %s\n", __LINE__, path, strerror(errno));
		return -1;
	}
	
	if (tcgetattr(fd, &term))
	{
		printf("init_serial %d: path=%s %s\n", __LINE__, path, strerror(errno));
		close(fd);
		return -1;
	}


#ifndef __clang__
// Try to set kernel to custom baud and low latency
	if(custom_baud)
	{
		struct serial_struct serial_struct;
		if(ioctl(fd, TIOCGSERIAL, &serial_struct) < 0)
		{
			printf("init_serial %d: path=%s %s\n", __LINE__, path, strerror(errno));
		}

		serial_struct.flags |= ASYNC_LOW_LATENCY;
		serial_struct.flags &= ~ASYNC_SPD_CUST;
		if(custom_baud)
		{
			serial_struct.flags |= ASYNC_SPD_CUST;
			serial_struct.custom_divisor = (int)((float)serial_struct.baud_base / 
				(float)custom_baud + 0.5);
			baud = B38400;
		}  
/*
 * printf("init_serial: %d serial_struct.baud_base=%d serial_struct.custom_divisor=%d\n", 
 * __LINE__,
 * serial_struct.baud_base,
 * serial_struct.custom_divisor);
 */


// Do setserial on the command line to ensure it actually worked.
		if(ioctl(fd, TIOCSSERIAL, &serial_struct) < 0)
		{
			printf("init_serial %d: path=%s %s\n", __LINE__, path, strerror(errno));
		}
	}
#endif // !__clang__
/*
 * printf("init_serial: %d path=%s iflag=0x%08x oflag=0x%08x cflag=0x%08x\n", 
 * __LINE__, 
 * path, 
 * term.c_iflag, 
 * term.c_oflag, 
 * term.c_cflag);
 */
 
	tcflush(fd, TCIOFLUSH);
	cfsetispeed(&term, baud);
	cfsetospeed(&term, baud);
//	term.c_iflag = IGNBRK;
	term.c_iflag = 0;
	term.c_oflag = 0;
	term.c_lflag = 0;
//	term.c_cflag &= ~(PARENB | PARODD | CRTSCTS | CSTOPB | CSIZE);
//	term.c_cflag |= CS8;
//    term.c_cflag |= CRTSCTS; // flow control
	term.c_cc[VTIME] = 1;
	term.c_cc[VMIN] = 1;
/*
 * printf("init_serial: %d path=%s iflag=0x%08x oflag=0x%08x cflag=0x%08x\n", 
 * __LINE__, 
 * path, 
 * term.c_iflag, 
 * term.c_oflag, 
 * term.c_cflag);
 */
	if(tcsetattr(fd, TCSANOW, &term))
	{
		printf("init_serial %d: path=%s %s\n", __LINE__, path, strerror(errno));
		close(fd);
		return -1;
	}

	printf("init_serial %d: opened %s\n", __LINE__, path);
	return fd;
}


void get_key(void *ptr);
void get_temp(void *ptr)
{
    serial_state_t *state = (serial_state_t*)ptr;
    state->serial_data[state->data_offset++] = serial_in;
    if(state->data_offset >= state->data_size)
    {
        state->key_offset = 0;
        state->function = get_key;

// test temperature integrity
        int i, j;
        int failed = 0;
        for(i = 1; i < REPEATS; i++)
        {
            if(state->serial_data[0] != state->serial_data[i])
            {
// reject packet if any value is different
                failed = 1;
                break;
            }
        }

// test voltage integrity
    	if(state->is_ext && !failed)
        {
            for(i = 1; i < REPEATS; i++)
            {
                if(state->serial_data[REPEATS] != state->serial_data[REPEATS + i])
                {
// reject packet if any value is different
                    failed = 1;
                    break;
                }
            }
        }

// new temperature reading
        if(!failed)
        {
            int temp = state->serial_data[0];

            if(temp != 0xff)
            {            
                pthread_mutex_lock(&temp_lock);
                if(!state->temp_valid)
                {
                    state->min_temp = temp;
                    state->max_temp = temp;
                }
                if(state->min_temp > temp)
                {
                    state->min_temp = temp;
                }
                if(state->max_temp < temp)
                {
                    state->max_temp = temp;
                }
                state->temp_valid = 1;
                pthread_mutex_unlock(&temp_lock);
            }

// get voltage
            if(state->is_ext)
            {
                state->voltage = (float)state->serial_data[4] / 10;
            }

            printf("get_temp %d new %s temp=%d voltage=%.1f\n", 
                __LINE__, 
                state->title, 
                temp,
                state->voltage);


// get wunderground temperature when indoor temperature is received
            if(!state->is_ext)
            {
                clock_gettime(CLOCK_MONOTONIC, &current_time);
                if(current_time.tv_sec / WUNDER_INTERVAL != last_wunder_time.tv_sec / WUNDER_INTERVAL)
                {
                    last_wunder_time = current_time;

// Try every wunderground station until one works
                    int got_it = 0;
                    for(j = 0; j < total_commands && !got_it; j++)
                    {
                        char *wunder_command = wunder_commands[j];
                        FILE *fd = popen(wunder_command, "r");
                        if(fd)
                        {
                            int len = 0;
                            while(!feof(fd))
                            {
                                int result = fread(wunder_json + len,
                                    1, 
                                    TEXTLEN - len - 1,
                                    fd);
                                if(result <= 0)
                                {
                                    break;
                                }
                                len += result;
                            }
                            fclose(fd);
                            wunder_json[len] = 0;
    //                        printf("get_temp %d:\n%s\n\n",
    //                            __LINE__,
    //                            wunder_json);
    // find the temperature value
                            char *ptr = strstr(wunder_json, "\"temp\":");
                            if(ptr)
                            {
                                ptr += 7;
    // truncate string
                                char *ptr2 = strchr(ptr, ',');
                                if(ptr2)
                                {
                                    *ptr2 = 0;
                                }
                                float value = atof(ptr);
                                int rounded = (int)(value + 0.5);
                                printf("get_temp %d: %s temp=%f rounded=%d\n", 
                                    __LINE__,
                                    ptr, 
                                    value,
                                    rounded);
    // send the packet
                                for(i = 0; i < KEY_SIZE; i++)
                                {
                                    wunder_packet[i] = PANEL_KEY[i];
                                }

                                for(i = 0; i < REPEATS; i++)
                                {
                                    wunder_packet[KEY_SIZE + i] = rounded ^ salt[i];
                                }
    // retransmit
                                for(i = 0; i < RESENDS; i++)
                                {
                                    int result = write(serial_fd, wunder_packet, sizeof(wunder_packet));
    //                                 printf("get_temp %d: result=%d\n",
    //                                     __LINE__,
    //                                     result);
                                }
                                got_it = 1;
                            }
                        }
                    }






                }
            }
        }
    }    
}


void get_key(void *ptr)
{
    serial_state_t *state = (serial_state_t*)ptr;

// get packet key from radio
    if(serial_in == state->key[state->key_offset])
    {
        state->key_offset++;
        if(state->key_offset >= KEY_SIZE)
        {
            state->function = get_temp;
            state->data_offset = 0;
        }
    }
    else
    if(serial_in == state->key[0])
    {
        state->key_offset = 1;
    }
    else
    if(state->key_offset > 0)
    {
        state->key_offset = 0;
    }
}

void* log_writer(void *ptr)
{
    while(1)
    {
        sleep(1);
        clock_gettime(CLOCK_MONOTONIC, &current_time);
        if(current_time.tv_sec / LOG_INTERVAL != start_time.tv_sec / LOG_INTERVAL)
        {
            start_time = current_time;
            FILE *utc_fd = popen("date -u", "r");
            char string[TEXTLEN];
            fread(string, 1, TEXTLEN, utc_fd);
            fclose(utc_fd);
            
// strip trailing newline from date
            char *ptr = string + strlen(string);
            while(ptr > string)
            {
                ptr--;
                if(*ptr == '\n' || *ptr == '\r')
                {
                    *ptr = 0;
                }
            }
            pthread_mutex_lock(&temp_lock);
            printf("log_writer %d: new line\n", __LINE__);
            fprintf(log_fd, 
                "%s\t%d\t%d\t%d\t%d\t%d\t%d\t%.1f\n",
                string,
                int_state.temp_valid,
                int_state.min_temp,
                int_state.max_temp,
                ext_state.temp_valid,
                ext_state.min_temp,
                ext_state.max_temp,
                ext_state.voltage);
            fflush(log_fd);
            
            int_state.temp_valid = 0;
            ext_state.temp_valid = 0;
            pthread_mutex_unlock(&temp_lock);
        }
    }
}

int main(int argc, char *argv[])
{
    int i;
    for(i = 1; i < argc; i++)
    {
        if(!strcmp(argv[i], "-h"))
        {
            printf("Record temperatures from remote thermometers\n");
            exit(0);
        }
        else
        {
            FILE *fd = fopen(argv[i], "r");
            if(fd)
            {
                while(!feof(fd))
                {
                    char *result = fgets(string, TEXTLEN, fd);
                    if(!result)
                    {
                        break;
                    }
                    if(strlen(string) > 0)
                    {
                        wunder_commands[total_commands] = strdup(string);
                        printf("wunderground command: %s\n", string);
                        total_commands++;
                    }
                }

                fclose(fd);
            }
            else
            {
                printf("Couldn't open wunderground commands %s\n", argv[i]);
                exit(1);
            }
        }
    }
    
    if(total_commands == 0)
    {
        printf("No wunderground commands provided.\n");
        exit(1);
    }
    
    
    printf("main %d recording temperatures\n", __LINE__);
    
    bzero(&ext_state, sizeof(ext_state));
    bzero(&int_state, sizeof(int_state));
    ext_state.function = get_key;
    int_state.function = get_key;
    ext_state.key = EXT_PACKET_KEY;
    int_state.key = INT_PACKET_KEY;
    ext_state.title = "EXT";
    int_state.title = "INT";
    ext_state.is_ext = 1;
    ext_state.data_size = EXT_DATA_SIZE;
    int_state.data_size = INT_DATA_SIZE;
    ext_state.min_temp = 0xff;
    int_state.min_temp = 0xff;
    ext_state.max_temp = -0xff;
    int_state.max_temp = -0xff;

	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutex_init(&temp_lock, &attr);
    
    log_fd = fopen(LOG_PATH, "a");
    if(!log_fd)
    {
        printf("main %d: failed to open log file\n", __LINE__);
        exit(1);
    }
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    
	pthread_attr_t tid_attr;
  	pthread_t tid;
	pthread_attr_init(&tid_attr);
    pthread_create(&tid, &tid_attr, log_writer, 0);

// 
    while(1)
    {
        if(serial_fd < 0)
        {
            serial_fd = init_serial("/dev/ttyACM0", B38400, BAUD);
	        if(serial_fd < 0) serial_fd = init_serial("/dev/ttyACM1", B38400, BAUD);
	        if(serial_fd < 0) serial_fd = init_serial("/dev/ttyACM2", B38400, BAUD);
            if(serial_fd < 0)
            {
                printf("main %d failed to open serial port\n", __LINE__);
                sleep(1);
            }
        }

        if(serial_fd >= 0)
        {
            int result = read(serial_fd, &serial_in, 1);
            if(result <= 0)
		    {
			    printf("main %d Unplugged\n", __LINE__);

// try to reopen it
                serial_fd = -1;
		    }
            else
            {
                ext_state.function(&ext_state);
                int_state.function(&int_state);
            }
        }
    }
}



