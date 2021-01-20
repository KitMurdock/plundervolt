#include <immintrin.h>
#include <stdint.h>
#include <x86intrin.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <inttypes.h>

uint64_t msr_value(int64_t val,uint64_t plane)
{
	//0.5 is to deal with rounding
	val=(val*1.024)-0.5;
	val=0xFFE00000&((val&0xFFF)<<21);
	val=val|0x8000001100000000;
	val=val|(plane<<40);
	return (uint64_t)val;
}



int main (int argc, char* argv[])
{
	const char* program = argv[0];
	int fd0;

	if (argc != 2)
	{
		printf("Need 1 values: %s  offset\n",program);
		exit (-1);
	}
	int64_t offset_mV=strtol(argv[1],NULL,10);
	uint64_t val_plane0 = msr_value(offset_mV, 0);
	uint64_t val_plane2 = msr_value(offset_mV, 2);

	// voltage drop
	fd0 = open("/dev/cpu/0/msr", O_WRONLY);
	if(fd0 == -1)
	{
		printf("Could not open /dev/cpu/0/msr\n");
		return -1;
	}
	pwrite(fd0,&val_plane0,8,0x150);
	pwrite(fd0,&val_plane2,8,0x150);
	close(fd0);
	return 0;
}
