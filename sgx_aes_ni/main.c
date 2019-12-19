#include <sgx_urts.h>
#include "Enclave/encl_u.h"
#include <sys/mman.h>
#include <signal.h>
#include <curses.h>
#include <stdint.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <inttypes.h>

#define BUFLEN 2048
#define SGX_AESGCM_MAC_SIZE 16
#define SGX_AESGCM_IV_SIZE 12

#define ENCLAVE_FILE "CryptoEnclave.signed.so"

int fd;

/* OCall functions */
void ocall_print_string(const char *str)
{
	/* Proxy/Bridge will check the length and null-terminate 
	 * the input string to prevent buffer overflow. 
	 */
	printf("\t[Enclave] %s", str);
	refresh();
}


void ocall_voltage_change(uint64_t plane)
{
	pwrite(fd, &plane, 8, 0x150);
}

void ocall_phex(const uint8_t *print_me, int len)
{
	char *output = malloc(len * 2 + 1);
	char *ptr = output;
	for (int i = 0; i < len; i++)
	{
		ptr += sprintf(ptr, "%02x", (uint8_t)print_me[i]);
	}
	printf("\t%s\n", output);
	free(output);
}
int main(int argc, char **argv)
{
	sgx_launch_token_t token = {0};
	int retval = 0, updated = 0;
	char old = 0x00, new = 0xbb;
	sgx_enclave_id_t eid = 0;
	uint64_t ret;
	const char *program = argv[0];

	if (argc != 3)
	{
		printf("Need 2 args: %s iterations undervolting \n", program);
		exit(-1);
	}

	//SGX_ASSERT(sgx_create_enclave("./Enclave/encl.so", /*debug=*/1, &token, &updated, &eid, NULL));
	sgx_create_enclave("./Enclave/encl.so", /*debug=*/1, &token, &updated, &eid, NULL);

	int j = 0;

	int iterations = atoi(argv[1]);
	int64_t offset = strtol(argv[2], NULL, 10);

	// open the register
	fd = open("/dev/cpu/0/msr", O_WRONLY);
	if (fd == -1)
	{
		printf("Could not open /dev/cpu/0/msr\n");
		return -1;
	}
	enclave_calculation(eid, iterations, offset);
	close(fd);
	//SGX_ASSERT(sgx_destroy_enclave(eid));
	sgx_destroy_enclave(eid);
	return 0;
}
