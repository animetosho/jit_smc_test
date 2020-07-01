.text
.globl jmp32k
.align 64
jmp32k:
	.macro jmp32sect i
		jmp jmp32k_\i
		.align 64
		jmp32k_\i :
	.endm
	.macro jmp32_1k j
		jmp32sect 10\j
		jmp32sect 11\j
		jmp32sect 12\j
		jmp32sect 13\j
		jmp32sect 14\j
		jmp32sect 15\j
		jmp32sect 16\j
		jmp32sect 17\j
		jmp32sect 18\j
		jmp32sect 19\j
		jmp32sect 20\j
		jmp32sect 21\j
		jmp32sect 22\j
		jmp32sect 23\j
		jmp32sect 24\j
		jmp32sect 25\j
	.endm
	
	jmp32_1k 10
	jmp32_1k 11
	jmp32_1k 12
	jmp32_1k 13
	jmp32_1k 14
	jmp32_1k 15
	jmp32_1k 16
	jmp32_1k 17
	jmp32_1k 18
	jmp32_1k 19
	jmp32_1k 20
	jmp32_1k 21
	jmp32_1k 22
	jmp32_1k 23
	jmp32_1k 24
	jmp32_1k 25
	jmp32_1k 26
	jmp32_1k 27
	jmp32_1k 28
	jmp32_1k 29
	jmp32_1k 30
	jmp32_1k 31
	jmp32_1k 32
	jmp32_1k 33
	jmp32_1k 34
	jmp32_1k 35
	jmp32_1k 36
	jmp32_1k 37
	jmp32_1k 38
	jmp32_1k 39
	jmp32_1k 40
	jmp32_1k 41
	
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
	.macro jmp32sect_u i
		# jmp instruction is 2 bytes, so will straddle cachelines
		jmp jmp32k_u_\i
		align128min1
		jmp32k_u_\i :
	.endm
	.macro jmp32_2k_u j
		jmp32sect_u 10\j
		jmp32sect_u 11\j
		jmp32sect_u 12\j
		jmp32sect_u 13\j
		jmp32sect_u 14\j
		jmp32sect_u 15\j
		jmp32sect_u 16\j
		jmp32sect_u 17\j
		jmp32sect_u 18\j
		jmp32sect_u 19\j
		jmp32sect_u 20\j
		jmp32sect_u 21\j
		jmp32sect_u 22\j
		jmp32sect_u 23\j
		jmp32sect_u 24\j
		jmp32sect_u 25\j
	.endm
	
	jmp32_2k_u 10
	jmp32_2k_u 11
	jmp32_2k_u 12
	jmp32_2k_u 13
	jmp32_2k_u 14
	jmp32_2k_u 15
	jmp32_2k_u 16
	jmp32_2k_u 17
	jmp32_2k_u 18
	jmp32_2k_u 19
	jmp32_2k_u 20
	jmp32_2k_u 21
	jmp32_2k_u 22
	jmp32_2k_u 23
	jmp32_2k_u 24
	jmp32_2k_u 25
	
	ret


.globl jmp64k
.align 64
jmp64k:
	.macro jmp64sect i
		jmp jmp64k_\i
		.align 64
		jmp64k_\i :
	.endm
	.macro jmp64_1k j
		jmp64sect 10\j
		jmp64sect 11\j
		jmp64sect 12\j
		jmp64sect 13\j
		jmp64sect 14\j
		jmp64sect 15\j
		jmp64sect 16\j
		jmp64sect 17\j
		jmp64sect 18\j
		jmp64sect 19\j
		jmp64sect 20\j
		jmp64sect 21\j
		jmp64sect 22\j
		jmp64sect 23\j
		jmp64sect 24\j
		jmp64sect 25\j
	.endm
	
	jmp64_1k 10
	jmp64_1k 11
	jmp64_1k 12
	jmp64_1k 13
	jmp64_1k 14
	jmp64_1k 15
	jmp64_1k 16
	jmp64_1k 17
	jmp64_1k 18
	jmp64_1k 19
	jmp64_1k 20
	jmp64_1k 21
	jmp64_1k 22
	jmp64_1k 23
	jmp64_1k 24
	jmp64_1k 25
	jmp64_1k 26
	jmp64_1k 27
	jmp64_1k 28
	jmp64_1k 29
	jmp64_1k 30
	jmp64_1k 31
	jmp64_1k 32
	jmp64_1k 33
	jmp64_1k 34
	jmp64_1k 35
	jmp64_1k 36
	jmp64_1k 37
	jmp64_1k 38
	jmp64_1k 39
	jmp64_1k 40
	jmp64_1k 41
	jmp64_1k 42
	jmp64_1k 43
	jmp64_1k 44
	jmp64_1k 45
	jmp64_1k 46
	jmp64_1k 47
	jmp64_1k 48
	jmp64_1k 49
	jmp64_1k 50
	jmp64_1k 51
	jmp64_1k 52
	jmp64_1k 53
	jmp64_1k 54
	jmp64_1k 55
	jmp64_1k 56
	jmp64_1k 57
	jmp64_1k 58
	jmp64_1k 59
	jmp64_1k 60
	jmp64_1k 61
	jmp64_1k 62
	jmp64_1k 63
	jmp64_1k 64
	jmp64_1k 65
	jmp64_1k 66
	jmp64_1k 67
	jmp64_1k 68
	jmp64_1k 69
	jmp64_1k 70
	jmp64_1k 71
	jmp64_1k 72
	jmp64_1k 73
	
	ret

.globl jmp64k_u
align128min1
jmp64k_u:
	.macro jmp64sect_u i
		# jmp instruction is 2 bytes, so will straddle cachelines
		jmp jmp64k_u_\i
		align128min1
		jmp64k_u_\i :
	.endm
	.macro jmp64_2k_u j
		jmp64sect_u 10\j
		jmp64sect_u 11\j
		jmp64sect_u 12\j
		jmp64sect_u 13\j
		jmp64sect_u 14\j
		jmp64sect_u 15\j
		jmp64sect_u 16\j
		jmp64sect_u 17\j
		jmp64sect_u 18\j
		jmp64sect_u 19\j
		jmp64sect_u 20\j
		jmp64sect_u 21\j
		jmp64sect_u 22\j
		jmp64sect_u 23\j
		jmp64sect_u 24\j
		jmp64sect_u 25\j
	.endm
	
	jmp64_2k_u 10
	jmp64_2k_u 11
	jmp64_2k_u 12
	jmp64_2k_u 13
	jmp64_2k_u 14
	jmp64_2k_u 15
	jmp64_2k_u 16
	jmp64_2k_u 17
	jmp64_2k_u 18
	jmp64_2k_u 19
	jmp64_2k_u 20
	jmp64_2k_u 21
	jmp64_2k_u 22
	jmp64_2k_u 23
	jmp64_2k_u 24
	jmp64_2k_u 25
	jmp64_2k_u 26
	jmp64_2k_u 27
	jmp64_2k_u 28
	jmp64_2k_u 29
	jmp64_2k_u 30
	jmp64_2k_u 31
	jmp64_2k_u 32
	jmp64_2k_u 33
	jmp64_2k_u 34
	jmp64_2k_u 35
	jmp64_2k_u 36
	jmp64_2k_u 37
	jmp64_2k_u 38
	jmp64_2k_u 39
	jmp64_2k_u 40
	jmp64_2k_u 41
	
	ret
