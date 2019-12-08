#include <curses.h>
#include <fcntl.h>
#include <inttypes.h>
#include <sys/ioctl.h>                
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

int fd;
uint64_t plane1=0x8000011000000000;
uint64_t plane2=0x8000021000000000;

static double actual_voltage(uint64_t msr_response)
{                                                                  
	double a = (double)((msr_response & 0xFFFF00000000)>>32);
    	return a/8192.0;
}  

double get_voltage()
{
       	uint64_t msr_198;
	pread(fd,&msr_198,sizeof msr_198,0x198);
	return actual_voltage(msr_198);
}

int64_t unpack_offset(uint64_t msr_response)
{
        int64_t x=msr_response>>21;
        double y;
        if (x<1024)
                y=x/1.024;
        else
                y=(2048-x)/1.024;
        y=y+0.5; //rounding
        y=y*-1; //make it negative
        return (int)y;
}

int64_t get_undervolting()
{
        uint64_t read_register;
        pwrite(fd,&plane1,8,0x150);
        pwrite(fd,&plane2,8,0x150);
        pread(fd,&read_register,8,0x150);
        return unpack_offset(read_register);
}

int main(void) 
{
	/* check we can open the msr */
        fd = open("/dev/cpu/0/msr", O_RDWR);
        if(fd == -1)
        {
                printf("Could not open /dev/cpu/0/msr\n");
                return -1;
        }


    	/*  Initialize ncurses  */
    	WINDOW * mainwin;
	if ( (mainwin = initscr()) == NULL ) 
	{
		fprintf(stderr, "Error initialising ncurses.\n");
		exit(EXIT_FAILURE);
    	}

	start_color();       /*  Initialize colours  */
	init_pair(1, COLOR_YELLOW,  COLOR_BLACK);
	init_pair(6, COLOR_BLUE,    COLOR_YELLOW);
	color_set(1, NULL);
	
	char printme[100];
	curs_set(0);
	while (1) 
	{
		sprintf(printme,"     Voltage\n      %.3fV\n\n   Undervolting\n      %limV        ",get_voltage(),get_undervolting());
	    	mvaddstr(6, 0, printme);
		refresh();
		sleep(1);
	}

    /*  Clean up */
    delwin(mainwin);
    endwin();
    refresh();
    return EXIT_SUCCESS;
}
