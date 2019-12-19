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

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "sgx_trts.h"
#include "sgx_tcrypto.h"
#include "encl_t.h"	// for ocall
#include <ipp/ippcp.h>

#define RSA_LEN 256

#define MAX(a,b) (((a)>(b))?(a):(b))

static void eprintf(const char *fmt, ...)
{
    char buf[1000] = {'\0'};
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, 1000, fmt, ap);
    va_end(ap);
    ocall_print_string(buf);
}

static uint8_t rsa_inited = 0;

IppsBigNumState* create_BN_state(int len, const Ipp32u* pData)  
{
    int size;
    
    ippsBigNumGetSize(len, &size);
    IppsBigNumState* pBN = (IppsBigNumState*) malloc(size);

    if(pBN == 0)
    {
        eprintf("create_BN_state(): malloc(%d) failed\n", size);
        return 0;
    }

    ippsBigNumInit(len, pBN);

    if (pData != NULL) 
    {
        ippsSet_BN(IppsBigNumPOS, len, pData, pBN);
    }

    return pBN;
}

const static char HexDigitList[] = "0123456789ABCDEF";

IppsBigNumState* create_BN_state_from_string(const char* s)
{
    if(!( '0' == s[0]) && (('x' == s[1]) || ('X' == s[1])))
    {
        return 0;
    }

    s += 2;

    const size_t len_in_nibbles = strlen(s);

    // Must be dword aligned, add leading zeroes if not
    if((len_in_nibbles % 8) != 0)
    {
        return 0;
    }

    const size_t len_in_dword = (int)(len_in_nibbles + 7)/8;

    Ipp32u* num = calloc(len_in_dword, sizeof(Ipp32u));

    if(num == 0)
    {
        return 0;
    } 

    int nibble_in_dword = 7;
    Ipp32u current_dword = 0;
    int dword_idx = len_in_dword - 1;

    for(unsigned int i = 0; i < len_in_nibbles; i++)
    {
        // Convert digit to hex
        const char tmp[2] = {s[i], 0};
        Ipp32u digit = (Ipp32u)strcspn(HexDigitList, tmp);

        current_dword |= digit << 4*nibble_in_dword; 

        if(nibble_in_dword == 0)
        {
            nibble_in_dword = 7;
            num[dword_idx] = current_dword;
            eprintf("DW: %08x\n", current_dword);

            current_dword = 0;
            dword_idx--;
        }
        else
        {
            nibble_in_dword--;
        }
    }

    IppsBigNumState* result = create_BN_state(len_in_dword, num);

    eprintf("create_BN_state() = %p\n", result);
    
    free(num);

    return result;
}

static IppsBigNumState *P = 0, *Q = 0, *dP = 0, *dQ = 0, *invQ = 0, *D = 0, *N = 0, *E = 0;
static IppsBigNumState *pt = 0, *ct = 0;

static IppsRSAPrivateKeyState* pPrv = 0;
static IppsRSAPublicKeyState* pPub = 0;
static Ipp8u* scratchBuffer = 0;
static IppsBigNumState* dec = 0;

#define INIT_CHECK(n, s) (n) = create_BN_state_from_string((s)); \
    if((n) == 0)                                                 \
    {                                                            \
        eprintf("Could not init big num\n");                     \
        return 0;                                                \
    }

uint8_t rsa_init_ecall()
{
    if(rsa_inited != 0)
    {
        eprintf("RSA already initialised\n");
        return 0;
    }

    eprintf("Init P...\n");
    INIT_CHECK(P,    "0xEECFAE81B1B9B3C908810B10A1B5600199EB9F44AEF4FDA493B81A9E3D84F632124EF0236E5D1E3B7E28FAE7AA040A2D5B252176459D1F397541BA2A58FB6599");

    eprintf("Init Q...");
    INIT_CHECK(Q,    "0xC97FB1F027F453F6341233EAAAD1D9353F6C42D08866B1D05A0F2035028B9D869840B41666B42E92EA0DA3B43204B5CFCE3352524D0416A5A441E700AF461503");

    INIT_CHECK(dP,   "0x54494CA63EBA0337E4E24023FCD69A5AEB07DDDC0183A4D0AC9B54B051F2B13ED9490975EAB77414FF59C1F7692E9A2E202B38FC910A474174ADC93C1F67C981");
    INIT_CHECK(dQ,   "0x471E0290FF0AF0750351B7F878864CA961ADBD3A8A7E991C5C0556A94C3146A7F9803F8F6F8AE342E931FD8AE47A220D1B99A495849807FE39F9245A9836DA3D");
    INIT_CHECK(invQ, "0xB06C4FDABB6301198D265BDBAE9423B380F271F73453885093077FCD39E2119FC98632154F5883B167A967BF402B4E9E2E0F9656E698EA3666EDFB25798039F7");
    INIT_CHECK(N,    "0xBBF82F090682CE9C2338AC2B9DA871F7368D07EED41043A440D6B6F07454F51FB8DFBAAF035C02AB61EA48CEEB6FCD4876ED520D60E1EC4619719D8A5B8B807FAFB8E0A3DFC737723EE6B4B7D93A2584EE6A649D060953748834B2454598394EE0AAB12D7B61A51F527A9A41F6C1687FE2537298CA2A8F5946F8E5FD091DBDCB");
    INIT_CHECK(D,    "0xA5DAFC5341FAF289C4B988DB30C1CDF83F31251E0668B42784813801579641B29410B3C7998D6BC465745E5C392669D6870DA2C082A939E37FDCB82EC93EDAC97FF3AD5950ACCFBC111C76F1A9529444E56AAF68C56C092CD38DC3BEF5D20A939926ED4F74A13EDDFBE1A1CECC4894AF9428C2B7B8883FE4463A4BC85B1CB3C1");
    INIT_CHECK(E,    "0x00000011");
    INIT_CHECK(pt,   "0x00EB7A19ACE9E3006350E329504B45E2CA82310B26DCD87D5C68F1EEA8F55267C31B2E8BB4251F84D7E0B2C04626F5AFF93EDCFB25C9C2B3FF8AE10E839A2DDB4CDCFE4FF47728B4A1B7C1362BAAD29AB48D2869D5024121435811591BE392F982FB3E87D095AEB40448DB972F3AC14F7BC275195281CE32D2F1B76D4D353E2D");
    INIT_CHECK(ct,   "0x1253E04DC0A5397BB44A7AB87E9BF2A039A33D1E996FC82A94CCD30074C95DF763722017069E5268DA5D1C0B4F872CF653C11DF82314A67968DFEAE28DEF04BB6D84B1C31D654A1970E5783BD6EB96A024C2CA2F4A90FE9F2EF5C9C140E5BB48DA9536AD8700C84FC9130ADEA74E558D51A74DDF85D8B50DE96838D6063E0955");

     // Do RSA dec
    int keyCtxSize;

    // (bit) size of key components
    int bitsN = RSA_LEN * 8;
    int bitsE = 8;
    int bitsP = RSA_LEN * 4;
    int bitsQ = RSA_LEN * 4;

    // define and setup public key
    ippsRSA_GetSizePublicKey(bitsN, bitsE, &keyCtxSize);
    pPub = (IppsRSAPublicKeyState*)( calloc(keyCtxSize, sizeof(Ipp8u)) );

    if(pPub == 0)
    {
        eprintf("Failed to alloc pPub\n");
        return 0;
    }

    ippsRSA_InitPublicKey(bitsN, bitsE, pPub, keyCtxSize);
    ippsRSA_SetPublicKey(N, E, pPub);

    // define and setup (type2) private key
    ippsRSA_GetSizePrivateKeyType2(bitsP, bitsQ, &keyCtxSize);
    pPrv = (IppsRSAPrivateKeyState*)( calloc(keyCtxSize, sizeof(Ipp8u)) );

    if(pPrv == 0)
    {
        eprintf("Failed to alloc pPrv\n");
        return 0;
    }

    ippsRSA_InitPrivateKeyType2(bitsP, bitsQ, pPrv, keyCtxSize);
    ippsRSA_SetPrivateKeyType2(P, Q, dP, dQ, invQ, pPrv);

    // allocate scratch buffer
    int buffSizePublic;
    ippsRSA_GetBufferSizePublicKey(&buffSizePublic, pPub);
    int buffSizePrivate;
    ippsRSA_GetBufferSizePrivateKey(&buffSizePrivate, pPrv);
    int buffSize = MAX(buffSizePublic, buffSizePrivate);
    scratchBuffer = calloc(buffSize, sizeof(Ipp8u) );

    if(scratchBuffer == 0)
    {
        eprintf("Failed to alloc scratchBuffer\n");
        return 0;
    }

    dec = create_BN_state(RSA_LEN / 4, 0);

    if(dec == 0)
    {
        eprintf("Failed to alloc decryption result dec\n");
        return 0;
    }


    rsa_inited = 1;
    return 1;
}

uint8_t rsa_clean_ecall()
{
    if(rsa_inited != 1)
    {
        eprintf("RSA not initialised\n");
        return 0;
    }

    // Cleanup
    free(dec);
    free(scratchBuffer);
    free(pPub);
    free(pPrv);
    free(P);
    free(Q);
    free(dP);
    free(dQ);
    free(invQ);
    free(N);
    free(D);
    free(E);
    free(ct);
    free(pt);

    return 1;
}

void trigger_fault(int iterations)
{
    volatile uint64_t var = 0xdeadbeef;
    volatile uint32_t not_used = 0xdead;

    for (int j = 0; j < 50000 - iterations;j++)
    {
        not_used = not_used * 0x12345;
    }

    // Trigger wait
    uint64_t random_value = 0x1122334455667788;
    var = 0xdeadbeef * random_value;
    int i = 0;
    while (var == 0xdeadbeef * random_value && i < 10000000)
    {
        i++;
        var = 0xdeadbeef;
        var *= random_value;
    }
}

uint8_t rsa_dec_ecall(uint8_t result[RSA_LEN], int iterations)
{
    IppsBigNumSGN sgn;

    if(rsa_inited != 1)
    {
        eprintf("RSA not initialised\n");
        return 0;
    }

    // Wait for first fault
    trigger_fault(iterations);

    // Actual decryption
    ippsRSA_Decrypt(ct, dec, pPrv, scratchBuffer);

    int result_len = RSA_LEN / 4;
    ippsGet_BN(&sgn, &result_len, result, dec);
  

    return 1;
}

uint64_t multiply_ecall(int iterations)
{
    volatile uint64_t var = 0xdeadbeef;
    volatile uint32_t not_used = 0xdead;

    for (int j = 0; j < 50000 - iterations;j++)
    {
        not_used = not_used * 0x12345;
    }

    uint64_t random_value = 0x1122334455667788;
    var = 0xdeadbeef * random_value;
    int i = 0;
    while (var == 0xdeadbeef * random_value && i < 10000000)
    {
        i++;
        var = 0xdeadbeef;
        var *= random_value;
    }

    eprintf("Computed: %016llx\n", var);
    eprintf("Expected: %016llx\n", 0xdeadbeef * random_value);

    var ^= 0xdeadbeef * random_value;

    eprintf("Diff:     %016llx\n", var);

    return var;
}