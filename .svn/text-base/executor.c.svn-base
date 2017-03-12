#include "lib68hc11e1.h"



int main(int argc, char *argv[])
{
	unsigned char *program = (unsigned char*)malloc(RAM_SIZE);
	int program_size;
	int result = read_s19(program, &program_size, "clock.s19");
	FILE *serial_fd = run_68hc11e1("/dev/ttyS0", program, program_size, 1);
	printf("Done.\n");
//	do_serial(serial_fd);
sleep(1000000);
}
