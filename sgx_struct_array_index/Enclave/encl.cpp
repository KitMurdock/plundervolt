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
#include <stdio.h>
#include <string.h>
#include "sgx_tcrypto.h"
#include "sgx_trts.h"
#include "encl_t.h"	// for ocall

static void eprintf(const char *fmt, ...)
{
	char buf[1000] = {'\0'};
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(buf, 1000, fmt, ap);
	va_end(ap);
	ocall_print_string(buf);
}

#define MAGIC_OFFSET 0x8ce05
#define ARR_LEN     MAGIC_OFFSET + 50
#define SECRET_LEN  8
struct_foo_t secret_array[ARR_LEN];
uint32_t enclave_secret = 0xdeadbeef;

__attribute__((optimize("O0"))) void do_init_secret(struct_foo_t *arr, uint64_t offset)
{
    for (int j = 0; j < SECRET_LEN; j++)
	{
		asm volatile("" ::: "memory"); 
		volatile struct_foo_t *foo = &arr[offset];
        foo->foo[j] = enclave_secret + j;
		asm volatile("" ::: "memory"); 
	}
}

__attribute__((optimize("O0"))) void my_ecall(void)
{
    for (int i = 0; i < 1000; i++)
	{
		asm volatile("" ::: "memory"); 
        do_init_secret(secret_array, MAGIC_OFFSET);
		asm volatile("" ::: "memory"); 
	}
}
