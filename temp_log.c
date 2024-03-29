/*
 * BASE STATION FOR TEMP SENSORS, DESKS & filament drier
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

// record temperatures coming from the 2 sensors
// transmit wunderground data & desk commands to
// LED panel & desks.

// scp it to the pi & compile it with gcc
// gcc -O2 -o /usr/bin/temp_log temp_log.c -lpthread -lrt
// gcc -O2 -o temp_log temp_log.c -lpthread -lrt

// run it with a file that contains the wunderground commands
// The file contains separate lines with commands to download 
// different weather stations.  The lines should be in order of preference.
// This program runs each line until a download succeeds.

// nohup temp_log wunder.txt > /dev/null&

// print UDP debug statements with netcat -u -l 1234

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
#include <netinet/in.h>
#include <sys/socket.h>
#include <semaphore.h>
#include <stdarg.h>



// send debug text to UDP
uint8_t debug_address[] = 
{
    10,0,10,25
};
#define DEBUG_PORT 1234
int debug_fd = -1;

uint8_t desk_addresses[] = 
{
    10,0,2,108, // ec:fa:bc:05:63:32
    10,0,2,109, // 
    10,0,2,110, // 
    10,0,2,111, // 
};
#define DESKS 4
#define DESK_PORT 1234
int desk_fd[DESKS];

// number of seconds between logs
#define LOG_INTERVAL (60 * 5)
// minimum number of seconds between drier logs
#define DRIER_INTERVAL (15)

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

#define KEY_SIZE sizeof(PANEL_KEY)



#define LOG_PATH "/var/climate.log"
#define DRIER_PATH "/var/drier.log"
#define TEXTLEN 1024
#define REPEATS 4
#define RESENDS 4
// temp only
//#define INT_DATA_SIZE REPEATS
#define INT_DATA_SIZE 1
// temp & voltage
//#define EXT_DATA_SIZE (REPEATS * 2)
#define EXT_DATA_SIZE 2

// start codes for USB input
#define INT_START 0x01
#define EXT_START 0x02
#define DESK_START 0x03
#define DRIER_START 0x04


unsigned char serial_in;
FILE *log_fd = 0;
FILE *drier_fd = 0;
struct timespec start_time = { 0 };
struct timespec last_wunder_time = { 0 };
struct timespec last_drier_time = { 0 };
#define MAX_COMMANDS 255
char *wunder_commands[MAX_COMMANDS] = { 0 };
int total_commands = 0;
char wunder_json[TEXTLEN];
uint8_t wunder_packet[KEY_SIZE + REPEATS];
char string[TEXTLEN];
// number of seconds between wunder updates
#define WUNDER_INTERVAL 25
sem_t wunder_lock;

typedef struct
{
    int data_size;
    uint8_t serial_data[EXT_DATA_SIZE];
    int data_offset;
    int min_temp;
    int max_temp;
    int temp_valid;
    const char *title;
    int is_ext;
    float voltage;
} weather_state_t;

typedef struct
{
    uint8_t data[4];
    int data_offset;
    int desk;
    uint8_t id;
    uint8_t adc;
    uint8_t button;
} desk_state_t;

desk_state_t desk_state;

uint8_t drier_data[6];
float drier_t;
float drier_h;
float drier_dp;
int drier_offset = 0;

weather_state_t ext_state;
weather_state_t int_state;
weather_state_t *current_state;
pthread_mutex_t temp_lock;
int serial_fd = -1;
void (*parser)();

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

void print_debug(char *text, ...)
{
    if(debug_fd < 0)
    {
        struct sockaddr_in peer_addr;
        socklen_t peer_addr_len = sizeof(struct sockaddr_in);
        peer_addr.sin_addr.s_addr = (debug_address[0]) |
            (debug_address[1] << 8) |
            (debug_address[2] << 16) |
            (debug_address[3] << 24);
        debug_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        peer_addr.sin_port = htons((unsigned short)DEBUG_PORT);
        peer_addr.sin_family = AF_INET;
        int result = connect(debug_fd, 
            (struct sockaddr*)&peer_addr, 
		    peer_addr_len);
        if(result != 0)
        {
            printf("print_debug %d: connect %s\n", __LINE__, strerror(errno));
            close(debug_fd);
            debug_fd = -1;
        }
    }
    
    if(debug_fd >= 0)
    {
        va_list(args);
        va_start(args, text);
        char string[TEXTLEN];
        vsprintf(string, text, args);
        va_end (args);
        
        int _ = write(debug_fd, string, strlen(string) + 1);
    }
}

float read_float(uint8_t *data)
{
    int8_t a = data[0];
    int8_t b = data[1];
    return (float)a + (float)b / 256;
}

void get_date_text(char *string)
{
    FILE *utc_fd = popen("date -u", "r");
    int _ = fread(string, 1, TEXTLEN, utc_fd);
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
}

void get_start();
void get_temp()
{
    current_state->serial_data[current_state->data_offset++] = serial_in;
//print_debug("GET_TEMP %d %d\n", current_state->data_offset, current_state->data_size);

    if(current_state->data_offset >= current_state->data_size)
    {
        parser = 0;


// new temperature reading
        int temp = current_state->serial_data[0];

        if(temp != 0xff)
        {
            pthread_mutex_lock(&temp_lock);
            if(!current_state->temp_valid)
            {
                current_state->min_temp = temp;
                current_state->max_temp = temp;
            }
            if(current_state->min_temp > temp)
            {
                current_state->min_temp = temp;
            }
            if(current_state->max_temp < temp)
            {
                current_state->max_temp = temp;
            }
            current_state->temp_valid = 1;
            pthread_mutex_unlock(&temp_lock);
        }

// get voltage
        if(current_state->is_ext)
        {
            current_state->voltage = (float)current_state->serial_data[1] / 10;
        }

        printf("get_temp %d: new %s temp=%d voltage=%.1f\n", 
            __LINE__, 
            current_state->title, 
            temp,
            current_state->voltage);
        print_debug("get_temp %d: new %s temp=%d voltage=%.1f\n", 
            __LINE__, 
            current_state->title, 
            temp,
            current_state->voltage);


// wake up thread to get the wunderground temperature
        sem_post(&wunder_lock);
    }
}

void get_drier_data()
{
    drier_data[drier_offset++] = serial_in;
//printf("%02x ", serial_in);
    if(drier_offset >= 6)
    {
//printf("\n");
        parser = 0;
        drier_t = read_float(drier_data);
        drier_h = read_float(drier_data + 2);
        drier_dp = read_float(drier_data + 4);
        printf("get_drier_data %d: TEMP: %.2f HUMID: %.2f DEW: %.2f\n",
            __LINE__,
            drier_t,
            drier_h,
            drier_dp);

        struct timespec current_time = { 0 };
        clock_gettime(CLOCK_MONOTONIC, &current_time);
        if(current_time.tv_sec / DRIER_INTERVAL != 
            last_drier_time.tv_sec / DRIER_INTERVAL)
        {
            last_drier_time = current_time;

            char string[TEXTLEN];
            get_date_text(string);
            drier_fd = fopen(DRIER_PATH, "a");
            if(!drier_fd)
            {
                printf("get_drier_data %d: failed to open drier file\n", __LINE__);
                exit(1);
            }
            fprintf(drier_fd, 
                "%s\t%.2f\t%.2f\t%.2f\n",
                string,
                drier_t,
                drier_h,
                drier_dp);
            fclose(drier_fd);
            drier_fd = 0;
        }
    }
}


void desk_data()
{
    desk_state.data[desk_state.data_offset++] = serial_in;
    if(desk_state.data_offset >= 3)
    {
        parser = 0;
        desk_state.id = desk_state.data[0];
        desk_state.adc = desk_state.data[1];
        desk_state.button = desk_state.data[2];

    // convert ADC to a desk number
        if(desk_state.adc >= 200)
            desk_state.desk = 0;
        else
        if(desk_state.adc >= 120)
            desk_state.desk = 1;
        else
        if(desk_state.adc >= 70)
            desk_state.desk = 2;
        else
            desk_state.desk = 3;


        printf("desk_data %d: id=%d adc=%d desk=%d button=0x%02x\n",
            __LINE__,
            desk_state.id,
            desk_state.adc,
            desk_state.desk,
            desk_state.button);
        if(desk_state.desk >= DESKS)
        {
            printf("desk_data %d: invalid desk\n",
                __LINE__);
        }
        else
        {
// send button over UDP
#define PACKET_SIZE 2
            uint8_t buffer[PACKET_SIZE];
            buffer[0] = desk_state.id;
            buffer[1] = desk_state.button;

            print_debug("GOT DESK %d 0x%02x 0x%02x\n", 
                desk_state.desk, 
                desk_state.id, 
                desk_state.button);
// open the socket
            if(desk_fd[desk_state.desk] < 0)
            {
                struct sockaddr_in peer_addr;
                socklen_t peer_addr_len = sizeof(struct sockaddr_in);
                uint8_t *address = &desk_addresses[desk_state.desk * 4];
                peer_addr.sin_addr.s_addr = (address[0]) |
                    (address[1] << 8) |
                    (address[2] << 16) |
                    (address[3] << 24);
                desk_fd[desk_state.desk] = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
                peer_addr.sin_port = htons((unsigned short)DESK_PORT);
                peer_addr.sin_family = AF_INET;
                int result = connect(desk_fd[desk_state.desk], 
                    (struct sockaddr*)&peer_addr, 
		            peer_addr_len);
                if(result != 0)
                {
                    printf("main %d: connect %s\n", __LINE__, strerror(errno));
                    close(desk_fd[desk_state.desk]);
                    desk_fd[desk_state.desk] = -1;
                }
            }

            if(desk_fd[desk_state.desk] >= 0)
            {
                int _ = write(desk_fd[desk_state.desk], buffer, PACKET_SIZE);
//                print_debug("SENT DESK\n");
            }
        }
    }
}

void get_start2()
{
//    print_debug("GOT START2\n");
    if(serial_in == DRIER_START)
    {
        parser = get_drier_data;
        drier_offset = 0;
    }
    else
    if(serial_in == DESK_START)
    {
        parser = desk_data;
        desk_state.data_offset = 0;
    }
    else
    if(serial_in == EXT_START)
    {
        parser = get_temp;
        current_state = &ext_state;
        current_state->data_offset = 0;
    }
    else
    if(serial_in == INT_START)
    {
        parser = get_temp;
        current_state = &int_state;
        current_state->data_offset = 0;
    }
    else
        parser = 0;
}

// void get_start(void *ptr)
// {
// // get packet type from radio
//     print_debug("GOT START\n");
//     if(serial_in == 0xff)
//     {
//         parser = get_start2;
//     }
// }


void* wunder_reader(void *ptr)
{
    while(1)
    {
        sem_wait(&wunder_lock);


        struct timespec current_time = { 0 };
        clock_gettime(CLOCK_MONOTONIC, &current_time);
        if(current_time.tv_sec / WUNDER_INTERVAL != 
            last_wunder_time.tv_sec / WUNDER_INTERVAL)
        {
            last_wunder_time = current_time;

    // Try every wunderground station until one works
            int got_it = 0;
            int i, j;
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


void* log_writer(void *ptr)
{
    while(1)
    {
        sleep(1);
        struct timespec current_time = { 0 };
        clock_gettime(CLOCK_MONOTONIC, &current_time);
        if(current_time.tv_sec / LOG_INTERVAL != start_time.tv_sec / LOG_INTERVAL)
        {
            start_time = current_time;
            char string[TEXTLEN];
            get_date_text(string);
            pthread_mutex_lock(&temp_lock);
            printf("log_writer %d: new line\n", __LINE__);
    
            log_fd = fopen(LOG_PATH, "a");
            if(!log_fd)
            {
                printf("log_writer %d: failed to open log file\n", __LINE__);
                exit(1);
            }
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
            fclose(log_fd);
            log_fd = 0;

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
//        exit(1);
    }
    else
    {
        printf("%d wunderground commands:\n", total_commands);
        for(i = 0; i < total_commands; i++)
        {
            printf("%s", wunder_commands[i]);
        }
    }
    
    
    
    printf("main %d recording temperatures in %s\n", __LINE__, LOG_PATH);
    printf("main %d recording drier in %s\n", __LINE__, DRIER_PATH);

    bzero(&ext_state, sizeof(ext_state));
    bzero(&int_state, sizeof(int_state));
    ext_state.title = "EXT";
    int_state.title = "INT";
    ext_state.is_ext = 1;
    ext_state.data_size = EXT_DATA_SIZE;
    int_state.data_size = INT_DATA_SIZE;
    ext_state.min_temp = 0xff;
    int_state.min_temp = 0xff;
    ext_state.max_temp = -0xff;
    int_state.max_temp = -0xff;

    bzero(&desk_state, sizeof(desk_state));

    parser = 0;

	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutex_init(&temp_lock, &attr);

    clock_gettime(CLOCK_MONOTONIC, &start_time);
    clock_gettime(CLOCK_MONOTONIC, &last_wunder_time);
    clock_gettime(CLOCK_MONOTONIC, &last_drier_time);

    
	pthread_attr_t tid_attr;
  	pthread_t tid;
	pthread_attr_init(&tid_attr);
    pthread_create(&tid, &tid_attr, log_writer, 0);
    sem_init(&wunder_lock, 0, 0);
    pthread_create(&tid, &tid_attr, wunder_reader, 0);


    for(i = 0; i < DESKS; i++)
        desk_fd[i] = -1;

#define START_CODE 0xff
#define ESC_CODE 0xfe
    int got_esc = 0;
    while(1)
    {
        if(serial_fd < 0)
        {
            for(i = 0; i < 99 && serial_fd < 0; i++)
            {
                sprintf(string, "/dev/ttyACM%d", i);
                serial_fd = init_serial(string, B38400, 0);
            }
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
//printf("serial_in=%02x got_esc=%d\n", serial_in, got_esc);
// decode byte stuffing
                if(!got_esc)
                {
                    if(serial_in == START_CODE)
                    {
//                        print_debug("GOT START\n");
                        parser = get_start2;
                    }
                    else
                    if(serial_in == ESC_CODE)
                    {
                        got_esc = 1;
                    }
                    else
                    {
                        print_debug("%02x ", serial_in);
                        if(parser) parser();
                    }
                }
                else
                {
// pass through START or ESC code
                    got_esc = 0;
                    serial_in ^= 0xff;
                    print_debug("%02x ", serial_in);
                    if(parser) parser();
                }
            }
        }
    }
}



