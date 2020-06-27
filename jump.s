.text
.globl jmp32k
.align 64
jmp32k:
	.macro jmpsect i
		jmp jmp32k_\i
		.align 64
		jmp32k_\i :
	.endm
	.macro jmp1k j
		jmpsect 10\j
		jmpsect 11\j
		jmpsect 12\j
		jmpsect 13\j
		jmpsect 14\j
		jmpsect 15\j
		jmpsect 16\j
		jmpsect 17\j
		jmpsect 18\j
		jmpsect 19\j
		jmpsect 20\j
		jmpsect 21\j
		jmpsect 22\j
		jmpsect 23\j
		jmpsect 24\j
		jmpsect 25\j
	.endm
	
	jmp1k 10
	jmp1k 11
	jmp1k 12
	jmp1k 13
	jmp1k 14
	jmp1k 15
	jmp1k 16
	jmp1k 17
	jmp1k 18
	jmp1k 19
	jmp1k 20
	jmp1k 21
	jmp1k 22
	jmp1k 23
	jmp1k 24
	jmp1k 25
	jmp1k 26
	jmp1k 27
	jmp1k 28
	jmp1k 29
	jmp1k 30
	jmp1k 31
	jmp1k 32
	jmp1k 33
	jmp1k 34
	jmp1k 35
	jmp1k 36
	jmp1k 37
	jmp1k 38
	jmp1k 39
	jmp1k 40
	jmp1k 41
	
	ret


# align to 64/128 minus 1 byte
.macro align128min1
	.align 64
	nop
	.align 32
	nop
	.align 16
	nop
	.align 8
	nop
	.align 4
	nop
	nop
	nop
.endm

.globl jmp32k_u
align128min1
jmp32k_u:
	.macro jmpsect_u i
		# jmp instruction is 2 bytes, so will straddle cachelines
		jmp jmp32k_u_\i
		align128min1
		jmp32k_u_\i :
	.endm
	.macro jmp2k_u j
		jmpsect_u 10\j
		jmpsect_u 11\j
		jmpsect_u 12\j
		jmpsect_u 13\j
		jmpsect_u 14\j
		jmpsect_u 15\j
		jmpsect_u 16\j
		jmpsect_u 17\j
		jmpsect_u 18\j
		jmpsect_u 19\j
		jmpsect_u 20\j
		jmpsect_u 21\j
		jmpsect_u 22\j
		jmpsect_u 23\j
		jmpsect_u 24\j
		jmpsect_u 25\j
	.endm
	
	jmp2k_u 10
	jmp2k_u 11
	jmp2k_u 12
	jmp2k_u 13
	jmp2k_u 14
	jmp2k_u 15
	jmp2k_u 16
	jmp2k_u 17
	jmp2k_u 18
	jmp2k_u 19
	jmp2k_u 20
	jmp2k_u 21
	jmp2k_u 22
	jmp2k_u 23
	jmp2k_u 24
	jmp2k_u 25
	
	ret

