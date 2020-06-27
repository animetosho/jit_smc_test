#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <x86intrin.h>

// compile with `cc -g -std=gnu99 -O3 -march=native -o test test.c jump.s`

/**************************************/
// boiler plate stuff

typedef void (*stratfunc_t)(void* dst);
typedef int (*jitfunc_t)();
const int PRE_ITERS = 50;
const int ITERS = 1000;
const int TRIALS = 10;
#define TEST_TRIALS 1
const int CODE_SIZE = 1024;

#if defined(_WINDOWS) || defined(__WINDOWS__) || defined(_WIN32) || defined(_WIN64)
# include <windows.h>
static __inline__ void* jit_alloc(size_t len) {
	return VirtualAlloc(NULL, len, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
}
static __inline__ void jit_free(void* mem, size_t len) {
	VirtualFree(mem, 0, MEM_RELEASE);
}
#else
# include <sys/mman.h>
static __inline__ void* jit_alloc(size_t len) {
	return mmap(NULL, len, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANON, -1, 0);
}
static __inline__ void jit_free(void* mem, size_t len) {
	munmap(mem, len);
}
#endif


#ifdef _MSC_VER
# define ALIGN_TO(a, v) __declspec(align(a)) v
# define ALIGN_ALLOC(buf, len, align) *(void**)&(buf) = _aligned_malloc((len), align)
# define ALIGN_FREE _aligned_free
# include <intrin.h>
#else
# define ALIGN_TO(a, v) v __attribute__((aligned(a)))
# include <stdlib.h>
# define ALIGN_ALLOC(buf, len, align) if(posix_memalign((void**)&(buf), align, (len))) (buf) = NULL
# define ALIGN_FREE free
#endif


static __inline__ uint64_t rdtsc() {
#ifdef _MSC_VER
	return __rdtsc();
#else
	uint32_t low, high;
	__asm__ __volatile__ ("rdtsc" : "=a" (low), "=d" (high));
	return (uint64_t)high << 32 | low;
#endif
}

static uint64_t time_jit(stratfunc_t fn, void* dst) {
	// to try to reduce variability, run multiple trials, and find lowest value
	uint64_t result = ~0ULL;
	
	for(int trial=0; trial<TEST_TRIALS; trial++) {
		uint64_t starttime, stoptime;
		// warmup (try to exclude variability present in initial rounds)
		for(int i=0; i<PRE_ITERS; i++)
			fn(dst);
		starttime = rdtsc();
		for(int i=0; i<ITERS; i++)
			fn(dst);
		stoptime = rdtsc();
		stoptime -= starttime;
		if(stoptime < result) result = stoptime;
	}
	return result;
}


/**************************************/
// the JITting function
// this is just a simple pointless sequence of ADD instructions, written one at a time, similar to how a simple JIT might do it
static void write_code(void* dst, size_t offset) {
	static uint32_t base = 0;
	uint8_t* code = (uint8_t*)dst;
	while(offset<CODE_SIZE-6) {
		code[offset++] = 5; // ADD eax, imm
		memcpy(code+offset, &base, 4); // immediate value
		offset += 4;
		base = base*2 + 1; // "random" transformation
	}
	code[offset] = 0xc3; // RET
}
// JIT code in reverse order
static void write_code_reverse(void* dst) {
	static uint32_t base = 0;
	uint8_t* code = (uint8_t*)dst;
	size_t p = CODE_SIZE-6;
	p -= p%5;
	code[p] = 0xc3; // RET
	while(p) {
		p -= 5;
		code[p] = 5; // ADD eax, imm
		memcpy(code+p+1, &base, 4); // immediate value
		base = base*2 + 1; // "random" transformation
	}
}


/**************************************/
// strategies for apply the JIT function

// do nothing special - base case
static void jit_plain(void* dst) {
	write_code(dst, 0);
	((jitfunc_t)dst)();
}

// only write, don't execute; this is just to show the overhead of the CPU handling JIT condition
static void* static_code = NULL;
static void jit_only_init() {
	static_code = jit_alloc(CODE_SIZE);
	write_code(static_code, 0);
}
static void jit_only(void* dst) {
	write_code(dst, 0);
	((jitfunc_t)static_code)();
}

// write JIT code in reverse
static void jit_reverse(void* dst) {
	write_code_reverse(dst);
	((jitfunc_t)dst)();
}

// JIT to temporary location on stack, then copy across to destination
static void jit_memcpy(void* dst) {
	ALIGN_TO(64, char* tmp[CODE_SIZE]);
	write_code(tmp, 0);
	memcpy(dst, tmp, CODE_SIZE);
	((jitfunc_t)dst)();
}
static void jit_memcpy_sse2(void* dst) {
	ALIGN_TO(16, char* tmp[CODE_SIZE]);
	write_code(tmp, 0);
	for(int i=0; i<((CODE_SIZE+15)&~15); i+=16)
		_mm_store_si128((__m128i*)(dst + i), _mm_load_si128((__m128i*)((char*)tmp + i)));
	((jitfunc_t)dst)();
}
static void jit_memcpy_sse2_nt(void* dst) {
	ALIGN_TO(16, char* tmp[CODE_SIZE]);
	write_code(tmp, 0);
	for(int i=0; i<((CODE_SIZE+15)&~15); i+=16)
		_mm_stream_si128((__m128i*)(dst + i), _mm_load_si128((__m128i*)((char*)tmp + i)));
	((jitfunc_t)dst)();
}
#ifdef __AVX__
static void jit_memcpy_avx(void* dst) {
	ALIGN_TO(32, char* tmp[CODE_SIZE]);
	write_code(tmp, 0);
	for(int i=0; i<((CODE_SIZE+31)&~31); i+=32)
		_mm256_store_si256((__m256i*)(dst + i), _mm256_load_si256((__m256i*)((char*)tmp + i)));
	((jitfunc_t)dst)();
}
static void jit_memcpy_avx_nt(void* dst) {
	ALIGN_TO(32, char* tmp[CODE_SIZE]);
	write_code(tmp, 0);
	for(int i=0; i<((CODE_SIZE+31)&~31); i+=32)
		_mm256_stream_si256((__m256i*)(dst + i), _mm256_load_si256((__m256i*)((char*)tmp + i)));
	((jitfunc_t)dst)();
}
#endif
#ifdef __AVX512F__
static void jit_memcpy_avx3(void* dst) {
	ALIGN_TO(64, char* tmp[CODE_SIZE]);
	write_code(tmp, 0);
	for(int i=0; i<((CODE_SIZE+63)&~63); i+=64)
		_mm512_store_si512(dst + i, _mm512_load_si512((char*)tmp + i));
	((jitfunc_t)dst)();
}
static void jit_memcpy_avx3_nt(void* dst) {
	ALIGN_TO(64, char* tmp[CODE_SIZE]);
	write_code(tmp, 0);
	for(int i=0; i<((CODE_SIZE+63)&~63); i+=64)
		_mm512_stream_si512(dst + i, _mm512_load_si512((char*)tmp + i));
	((jitfunc_t)dst)();
}
#endif

static void jit_memcpy_sse2_rev(void* dst) {
	ALIGN_TO(16, char* tmp[CODE_SIZE]);
	write_code(tmp, 0);
	for(int i=((CODE_SIZE+15)&~15)-16; i>=0; i-=16)
		_mm_store_si128((__m128i*)(dst + i), _mm_load_si128((__m128i*)((char*)tmp + i)));
	((jitfunc_t)dst)();
}
#ifdef __AVX__
static void jit_memcpy_avx_rev(void* dst) {
	ALIGN_TO(32, char* tmp[CODE_SIZE]);
	write_code(tmp, 0);
	for(int i=((CODE_SIZE+31)&~31)-32; i>=0; i-=32)
		_mm256_store_si256((__m256i*)(dst + i), _mm256_load_si256((__m256i*)((char*)tmp + i)));
	((jitfunc_t)dst)();
}
#endif
#ifdef __AVX512F__
static void jit_memcpy_avx3_rev(void* dst) {
	ALIGN_TO(64, char* tmp[CODE_SIZE]);
	write_code(tmp, 0);
	for(int i=((CODE_SIZE+63)&~63)-64; i>=0; i-=64)
		_mm512_store_si512(dst + i, _mm512_load_si512((char*)tmp + i));
	((jitfunc_t)dst)();
}
#endif


// clear JIT memory before writing
static void jit_clr(void* dst) {
	memset(dst, 0, CODE_SIZE);
	write_code(dst, 0);
	((jitfunc_t)dst)();
}

// fill memory with RET instruction before writing
static void jit_clr_ret(void* dst) {
	memset(dst, 0xC3, CODE_SIZE);
	write_code(dst, 0);
	((jitfunc_t)dst)();
}

// clear first byte per cacheline before writing
static void jit_clr_1byte(void* dst) {
	for(int i=0; i<CODE_SIZE; i+=64)
		((char*)dst)[i] = 0;
	//	memset(dst + i, 0, 1);
	
	write_code(dst, 0);
	((jitfunc_t)dst)();
}

// clear two cachelines with a straddled 2-byte write
static void jit_clr_2byte(void* dst) {
	uint16_t* code = (uint16_t*)((uint8_t*)dst + 63); // straddle cacheline boundary
	for(int i=0; i<CODE_SIZE/2-33; i+=64)
		code[i] = 0;
	
	//for(int i=0; i<CODE_SIZE-65; i+=128)
	//	memset(dst + i + 63, 0, 2);
	
	write_code(dst, 0);
	((jitfunc_t)dst)();
}

#ifdef __AVX512F__
// clear via scatter instruction, writing 4 bytes per cacheline
static void jit_clr_scatter(void* dst) {
	for(int i=0; i<CODE_SIZE; i+=64*16)
		_mm512_i32scatter_epi32(dst + i, _mm512_set_epi32(
			0x3c0, 0x380, 0x340, 0x300,
			0x2c0, 0x280, 0x240, 0x200,
			0x1c0, 0x180, 0x140, 0x100,
			0x0c0, 0x080, 0x040, 0x000
		), _mm512_setzero_si512(), 1);
	
	write_code(dst, 0);
	((jitfunc_t)dst)();
}

#endif

// clear JIT memory then write in reverse
static void jit_clr_reverse(void* dst) {
	memset(dst, 0, CODE_SIZE);
	write_code_reverse(dst);
	((jitfunc_t)dst)();
}
// other reverse variants of above
static void jit_clr_1byte_rev(void* dst) {
	for(int i=0; i<CODE_SIZE; i+=64)
		((char*)dst)[i] = 0;
	write_code_reverse(dst);
	((jitfunc_t)dst)();
}
static void jit_clr_2byte_rev(void* dst) {
	uint16_t* code = (uint16_t*)((uint8_t*)dst + 63); // straddle cacheline boundary
	for(int i=0; i<CODE_SIZE/2-33; i+=64)
		code[i] = 0;
	write_code_reverse(dst);
	((jitfunc_t)dst)();
}


#ifdef __CLZERO__
// clear via CLZERO
static void jit_clzero(void* dst) {
	uint8_t* code = (uint8_t*)dst;
	for(int i=0; i<CODE_SIZE; i+=64)
		_mm_clzero(code + i);
	
	write_code(dst, 0);
	((jitfunc_t)dst)();
}
#endif

#ifdef __CLDEMOTE__
// apply CLDEMOTE before writing JIT
static void jit_cldemote(void* dst) {
	uint8_t* code = (uint8_t*)dst;
	for(int i=0; i<CODE_SIZE; i+=64)
		_mm_cldemote(code + i);
	
	write_code(dst, 0);
	((jitfunc_t)dst)();
}
// apply it before execution
static void jit_cldemote_after(void* dst) {
	write_code(dst, 0);
	uint8_t* code = (uint8_t*)dst;
	for(int i=0; i<CODE_SIZE; i+=64)
		_mm_cldemote(code + i);
	((jitfunc_t)dst)();
}
#endif


// CLFLUSH region before JIT/execution
static void jit_clflush(void* dst) {
	uint8_t* code = (uint8_t*)dst;
	for(int i=0; i<CODE_SIZE; i+=64)
		_mm_clflush(code + i);
	
	write_code(dst, 0);
	((jitfunc_t)dst)();
}
static void jit_clflush_after(void* dst) {
	write_code(dst, 0);
	uint8_t* code = (uint8_t*)dst;
	for(int i=0; i<CODE_SIZE; i+=64)
		_mm_clflush(code + i);
	((jitfunc_t)dst)();
}

#ifdef __CLFLUSHOPT__
// CLFLUSHOPT region before JIT/execution
static void jit_clflushopt(void* dst) {
	uint8_t* code = (uint8_t*)dst;
	for(int i=0; i<CODE_SIZE; i+=64)
		_mm_clflushopt(code + i);
	
	write_code(dst, 0);
	((jitfunc_t)dst)();
}
static void jit_clflushopt_after(void* dst) {
	write_code(dst, 0);
	uint8_t* code = (uint8_t*)dst;
	for(int i=0; i<CODE_SIZE; i+=64)
		_mm_clflushopt(code + i);
	((jitfunc_t)dst)();
}
#endif

// PREFETCHW (write hint?) region before JIT
// seems to generally run, even if PREFETCHW not supported by the processor
static void jit_prefetchw(void* dst) {
	uint8_t* code = (uint8_t*)dst;
	for(int i=0; i<CODE_SIZE; i+=64)
		_mm_prefetch(code + i, _MM_HINT_ET1);
	
	write_code(dst, 0);
	((jitfunc_t)dst)();
}

// PREFETCHT1 (L2 cache?) region before JIT
static void jit_prefetcht1(void* dst) {
	uint8_t* code = (uint8_t*)dst;
	for(int i=0; i<CODE_SIZE; i+=64)
		_mm_prefetch(code + i, _MM_HINT_T1);
	
	write_code(dst, 0);
	((jitfunc_t)dst)();
}
static void jit_prefetcht1_after(void* dst) {
	write_code(dst, 0);
	uint8_t* code = (uint8_t*)dst;
	for(int i=0; i<CODE_SIZE; i+=64)
		_mm_prefetch(code + i, _MM_HINT_T1);
	((jitfunc_t)dst)();
}

// write a single UD2 instruction at beginning, JIT, then write first instruction last
static void jit_ud2(void* dst) {
	*(uint16_t*)dst = 0xb0f; // UD2
	write_code(dst, 5);
	// write ADD eax, imm
	*(uint8_t*)dst = 5;
	*(uint32_t*)((char*)dst+1) = 0x55555555;
	((jitfunc_t)dst)();
}
// as above, but also clear remaining
static void jit_ud2_clr(void* dst) {
	*(uint16_t*)dst = 0xb0f; // UD2
	memset(dst + 2, 0, CODE_SIZE-2);
	write_code(dst, 5);
	// write ADD eax, imm
	*(uint8_t*)dst = 5;
	*(uint32_t*)((char*)dst+1) = 0x55555555;
	((jitfunc_t)dst)();
}
static void jit_ud2_clr_1byte(void* dst) {
	*(uint16_t*)dst = 0xb0f; // UD2
	for(int i=64; i<CODE_SIZE; i+=64)
		((char*)dst)[i] = 0;
	write_code(dst, 5);
	// write ADD eax, imm
	*(uint8_t*)dst = 5;
	*(uint32_t*)((char*)dst+1) = 0x55555555;
	((jitfunc_t)dst)();
}

// realloc a whole new region per JIT invocation (to demonstrate the cost of W^X)
static void jit_realloc(void* dst) {
	void* tmp = jit_alloc(CODE_SIZE);
	write_code(tmp, 0);
	((jitfunc_t)tmp)();
	jit_free(tmp, CODE_SIZE);
}


#define NUM_REGIONS 64
// alternate between multiple destinations; does trash the cache somewhat
static void jit_2region(void* regions) {
	static unsigned cnt = 0;
	void* dst = ((void**)regions)[cnt];
	write_code(dst, 0);
	((jitfunc_t)dst)();
	cnt = (cnt+1) % 2;
}
static void jit_4region(void* regions) {
	static unsigned cnt = 0;
	void* dst = ((void**)regions)[cnt];
	write_code(dst, 0);
	((jitfunc_t)dst)();
	cnt = (cnt+1) % 4;
}
static void jit_8region(void* regions) {
	static unsigned cnt = 0;
	void* dst = ((void**)regions)[cnt];
	write_code(dst, 0);
	((jitfunc_t)dst)();
	cnt = (cnt+1) % 8;
}
static void jit_16region(void* regions) {
	static unsigned cnt = 0;
	void* dst = ((void**)regions)[cnt];
	write_code(dst, 0);
	((jitfunc_t)dst)();
	cnt = (cnt+1) % 16;
}
static void jit_32region(void* regions) {
	static unsigned cnt = 0;
	void* dst = ((void**)regions)[cnt];
	write_code(dst, 0);
	((jitfunc_t)dst)();
	cnt = (cnt+1) % 32;
}
static void jit_64region(void* regions) {
	static unsigned cnt = 0;
	void* dst = ((void**)regions)[cnt];
	write_code(dst, 0);
	((jitfunc_t)dst)();
	cnt = (cnt+1) % 64;
}

// alternate between regions, but flush after use
static void jit_2region_flush(void* regions) {
	static unsigned cnt = 0;
	void* dst = ((void**)regions)[cnt];
	write_code(dst, 0);
	((jitfunc_t)dst)();
	
	uint8_t* code = (uint8_t*)dst;
	for(int i=0; i<CODE_SIZE; i+=64)
		_mm_clflush(code + i);
	
	cnt = (cnt+1) % 2;
}
#ifdef __CLFLUSHOPT__
static void jit_2region_flushopt(void* regions) {
	static unsigned cnt = 0;
	void* dst = ((void**)regions)[cnt];
	write_code(dst, 0);
	((jitfunc_t)dst)();
	
	uint8_t* code = (uint8_t*)dst;
	for(int i=0; i<CODE_SIZE; i+=64)
		_mm_clflushopt(code + i);
	
	cnt = (cnt+1) % 2;
}
#endif
// like above, but clear region afterwards instead
static void jit_2region_clr(void* regions) {
	static unsigned cnt = 0;
	void* dst = ((void**)regions)[cnt];
	write_code(dst, 0);
	((jitfunc_t)dst)();
	
	memset(dst, 0, CODE_SIZE);
	
	cnt = (cnt+1) % 2;
}

// run 32KB of instructions by jumping across cachelines
extern void jmp32k(void);
static void jit_jmp32k(void* dst) {
	jmp32k();
	write_code(dst, 0);
	((jitfunc_t)dst)();
}
// as above, but align jump instructions to straddle cachelines, requiring half the number of jumps
extern void jmp32k_u(void);
static void jit_jmp32k_unalign(void* dst) {
	jmp32k_u();
	write_code(dst, 0);
	((jitfunc_t)dst)();
}

// does fencing do anything?
static void jit_mfence(void* dst) {
	write_code(dst, 0);
	_mm_mfence();
	((jitfunc_t)dst)();
}

// does serializing do anything?
#ifdef _MSC_VER
# include <intrin.h>
# define _cpuid __cpuid
#else
# include <cpuid.h>
# define _cpuid(ar, eax) __cpuid(eax, ar[0], ar[1], ar[2], ar[3])
#endif
static void jit_serialize(void* dst) {
	write_code(dst, 0);
	int id[4];
	_cpuid(id, 1);
	volatile int unused = id[0];
	((jitfunc_t)dst)();
}

// TODO: test multiple virtual mappings to same physical address?


int main(void) {
	void* dst[NUM_REGIONS];
	for(int i=0; i<NUM_REGIONS; i++) {
		void* region = jit_alloc(CODE_SIZE);
		if(!region) {
			printf("Failed to allocate write+execute page\n");
			return 1;
		}
		if((uintptr_t)region & 63) {
			printf("Allocated page isn't cacheline aligned?!\n");
			return 1;
		}
		dst[i] = region;
	}
	
	jit_only_init();
	
	uint64_t times[100];
	memset(times, 0xff, sizeof(times));
	
	int trial = TRIALS;
	while(trial--) {
		int test = 0;
		// to reduce variability, try to sample the fastest time
		#define DO_TIME_TEST(fn, dst) { \
			uint64_t time = time_jit(fn, dst); \
			if(times[test] > time) times[test] = time; \
			if(!trial) \
				printf("%20s  %9" PRIu64 " rdtsc counts\n", #fn, times[test]); \
			test++; \
		}
		
		DO_TIME_TEST(jit_plain, dst[0]);
		DO_TIME_TEST(jit_only, dst[0]);
		DO_TIME_TEST(jit_reverse, dst[0]);
		DO_TIME_TEST(jit_memcpy, dst[0]);
		DO_TIME_TEST(jit_memcpy_sse2, dst[0]);
		DO_TIME_TEST(jit_memcpy_sse2_nt, dst[0]);
		#ifdef __AVX__
		DO_TIME_TEST(jit_memcpy_avx, dst[0]);
		DO_TIME_TEST(jit_memcpy_avx_nt, dst[0]);
		#endif
		#ifdef __AVX512F__
		DO_TIME_TEST(jit_memcpy_avx3, dst[0]);
		DO_TIME_TEST(jit_memcpy_avx3_nt, dst[0]);
		#endif
		DO_TIME_TEST(jit_memcpy_sse2_rev, dst[0]);
		#ifdef __AVX__
		DO_TIME_TEST(jit_memcpy_avx_rev, dst[0]);
		#endif
		#ifdef __AVX512F__
		DO_TIME_TEST(jit_memcpy_avx3_rev, dst[0]);
		#endif
		DO_TIME_TEST(jit_clr, dst[0]);
		DO_TIME_TEST(jit_clr_ret, dst[0]);
		DO_TIME_TEST(jit_clr_1byte, dst[0]);
		DO_TIME_TEST(jit_clr_2byte, dst[0]);
		#ifdef __AVX512F__
		DO_TIME_TEST(jit_clr_scatter, dst[0]);
		#endif
		DO_TIME_TEST(jit_clr_reverse, dst[0]);
		DO_TIME_TEST(jit_clr_1byte_rev, dst[0]);
		DO_TIME_TEST(jit_clr_2byte_rev, dst[0]);
		#ifdef __CLZERO__
		DO_TIME_TEST(jit_clzero, dst[0]);
		#endif
		#ifdef __CLDEMOTE__
		DO_TIME_TEST(jit_cldemote, dst[0]);
		DO_TIME_TEST(jit_cldemote_after, dst[0]);
		#endif
		DO_TIME_TEST(jit_clflush, dst[0]);
		DO_TIME_TEST(jit_clflush_after, dst[0]);
		#ifdef __CLFLUSHOPT__
		DO_TIME_TEST(jit_clflushopt, dst[0]);
		DO_TIME_TEST(jit_clflushopt_after, dst[0]);
		#endif
		//#ifdef __PREFETCHWT1__
		DO_TIME_TEST(jit_prefetchw, dst[0]);
		//#endif
		DO_TIME_TEST(jit_prefetcht1, dst[0]);
		DO_TIME_TEST(jit_prefetcht1_after, dst[0]);
		DO_TIME_TEST(jit_ud2, dst[0]);
		DO_TIME_TEST(jit_ud2_clr, dst[0]);
		DO_TIME_TEST(jit_ud2_clr_1byte, dst[0]);
		DO_TIME_TEST(jit_2region, dst);
		DO_TIME_TEST(jit_4region, dst);
		DO_TIME_TEST(jit_8region, dst);
		DO_TIME_TEST(jit_16region, dst);
		DO_TIME_TEST(jit_32region, dst);
		DO_TIME_TEST(jit_64region, dst);
		DO_TIME_TEST(jit_2region_flush, dst);
		#ifdef __CLFLUSHOPT__
		DO_TIME_TEST(jit_2region_flushopt, dst);
		#endif
		DO_TIME_TEST(jit_2region_clr, dst);
		DO_TIME_TEST(jit_jmp32k, dst[0]);
		DO_TIME_TEST(jit_jmp32k_unalign, dst[0]);
		DO_TIME_TEST(jit_mfence, dst[0]);
		DO_TIME_TEST(jit_serialize, dst[0]);
		DO_TIME_TEST(jit_realloc, dst[0]);
	}
	
	for(int i=0; i<NUM_REGIONS; i++)
		jit_free(dst[i], CODE_SIZE);
	
	return 0;
}
