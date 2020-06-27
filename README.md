JIT/Self-Modifying Code Performance on x86
======

Suppose you have the following code:

```
page = allocate_memory_page(permissions = read+write+execute)
begin loop
	write_1kb_function_code(page)
	page() ; execute written code
end loop
```

This is essentially a JIT where the code is only used once.

On many processors, this seems to cause it to detect self-modifying code behaviour, and act accordingly. This usually involves invalidating cache and flushing pipelines, which causes severe performance penalties.

I came across this issue whilst investigating an algorithm for [performing GF(2<sup>16</sup>) multiplication](https://github.com/animetosho/ParPar/blob/master/xor_depends/info.md), and noticed that some extra work can be done to mitigate the performance penalty, on some processors, such as clearing the memory before writing to it.

However, I’ve been unable to find much information on this. Hence, this repository hosts some test code which intends to investigate ways in which to reduce the overhead of such code.

### Compiling/Running

The code currently does not implement dynamic dispatch, so should be compiled using GCC/Clang with the `-march=native` flag.
The following command can be used to compile this test:

```
cc -g -std=gnu99 -O3 -march=native -o test test.c jump.s
```

I’ve noticed significant variability in results when running the test. The code does try to cater for this, by running multiple trials and taking the fastest run, but it may be beneficial to set the CPU governor/power profile to Performance before running the test.

### Test Summary

It’s best to look at the code to see what is done, but a summary of ideas tried is listed here:

* obvious approach - that is, just write the code an execute it. Also tests writing the code in reverse to see if that has any effect.
* write code to non-executable memory, then copy it across (using `memcpy` as well as aligned vector copies with/without non-temporal hinting). The memory copy means that fewer write operations hit the executable region, which seems to work better on some processors, despite the extra work
* clear executable memory before writing code, tested using `memset` as well as writing once to each cacheline. Presumably this helps invalidate the cache quickly before the JIT process does it slowly?
* flushing cachelines before/after writing code
* prefetching code to L2 cache, with/without write hinting
* leaving a `UD2` instruction at the beginning of the executable code, until the JIT process is complete. Maybe this helps with preventing the processor from prefetching the code as it’s being written to
* alternating between a number of pre-allocated pages. This might help by reducing the likelihood that the JIT process touches memory that is cached for execution, however, this is fairly expensive in terms of cache usage
* trying to clear the instruction cache by jumping across a large number of cachelines. Probably thrases the cache however
* seeing whether applying a fence or serializing operation between writing and executing the code, makes any difference

Note that the JIT routine itself writes out instructions one-by-one, and doesn’t attempt to batch multiple instructions into a single write.