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
#define ARR_LEN     MAGIC_OFFSET + 1001
#define SECRET_LEN  8
struct_foo_t secret_array[ARR_LEN];
uint32_t enclave_secret = 0xdeadbeef;

#if 1
    void do_init_secret(struct_foo_t *arr, uint64_t offset)
    {
        struct_foo_t *foo = &arr[offset];
        foo->foo[0] = enclave_secret;
    }
#else
    __attribute__((optimize("O0")))
    void do_init_secret(struct_foo_t *arr, uint64_t offset)
    {
        for (int j = 0; j < SECRET_LEN; j++)
        {
            asm volatile("" ::: "memory"); 
            volatile struct_foo_t *foo = &arr[offset];
            foo->foo[0] = enclave_secret;
            asm volatile("" ::: "memory"); 
        }
    }
#endif

__attribute__((optimize("O0"))) 
void my_ecall(void)
{
    for (int i = 0; i < 1000; i++)
	{
		//asm volatile("" ::: "memory"); 
        do_init_secret(secret_array, MAGIC_OFFSET+i);
		//asm volatile("" ::: "memory"); 
	}
}

/* ---------------------------------------------------------------------- */

#define REF_LE_WL_SIZE      (MAGIC_OFFSET + 1001)

/* From: https://github.com/intel/linux-sgx/blob/master/psw/ae/ref_le/ref_le.cpp#L47 */
// staticly allocate the white list cache - all the values in the cache are in little endian
ref_le_white_list_entry_t g_ref_le_white_list_cache[REF_LE_WL_SIZE] = { 0 };

void init_wl(void)
{
    memset(g_ref_le_white_list_cache, 0x00, sizeof(ref_le_white_list_entry_t) * REF_LE_WL_SIZE);
}

int check_wl_entry(size_t idx, sgx_measurement_t *mrsigner, int provision)
{
    /* From: https://github.com/intel/linux-sgx/blob/master/psw/ae/ref_le/ref_le.cpp#L220 */
    ref_le_white_list_entry_t *current_entry = &g_ref_le_white_list_cache[idx];

    if (memcmp(&(current_entry->mr_signer), mrsigner, sizeof(sgx_measurement_t)) == 0)
        return (provision ? current_entry->provision_key : 1);

    return 0;
}

__attribute__((optimize("O0"))) 
int get_launch_token(size_t *it, sgx_measurement_t mrsigner, int provision)
{
    for (size_t i = 0; i < REF_LE_WL_SIZE; i++)
    {
        if (check_wl_entry(i, &mrsigner, provision))
        {
            return 1;
        }

        /* NOTE: we explicitly leak the loop iteration to simplify things;
         * real-world adversaries could simply use a #PF side-channel or count
         * instructions w precise single-stepping
         */
        *it = i;
    } 

    return 0;
}
