/*
 * memlib.c - a module that simulates the memory system.  Needed because it 
 *            allows us to interleave calls from the student's malloc package 
 *            with the system's malloc package in libc.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>
#include <errno.h>

#include "memlib.h"
#include "config.h"

/* private variables */  

// heap의 첫 byte를 가리키는 포인터
static char *mem_start_brk;

// 힙의 마지막 byte에서 plus 1의 byte를 가리키는 포인터
static char *mem_brk;

// 힙의 최대 크기에 plus 1의 byte를 가리키는 포인터
static char *mem_max_addr;

/* 
 * mem_init - initialize the memory system model
 */
void mem_init(void)
{
    /* allocate the storage we will use to model the available VM */
    if ((mem_start_brk = (char *)malloc(MAX_HEAP)) == NULL) {
	fprintf(stderr, "mem_init_vm: malloc error\n");
	exit(1);
    }

    // 할당받은 heap의 첫 byte의포인터에 heap의 최대 크기를 더한다.
    mem_max_addr = mem_start_brk + MAX_HEAP; 

    // mem_brk는 사용중인 heap의 마지막 byte에 +1을 한 byte를 가리킨다.
    // init 상태에서는 heap이 비어있으므로 start_brk와 동일하다
    mem_brk = mem_start_brk;
}

/* 
 * mem_deinit - free the storage used by the memory system model
 */
void mem_deinit(void)
{
    free(mem_start_brk);
}

/*
 * mem_reset_brk - reset the simulated brk pointer to make an empty heap
 */
void mem_reset_brk()
{
    mem_brk = mem_start_brk;
}

/* 
 * mem_sbrk - simple model of the sbrk function. Extends the heap 
 *    by incr bytes and returns the start address of the new area. In
 *    this model, the heap cannot be shrunk.

인자로 받은 incr만큼 heap을 확장한다.
이때 반환하는 값은 확장한 heap의 첫 주소를 가리키는 포인터이다
 */
void *mem_sbrk(int incr) 
{
    char *old_brk = mem_brk;

    if ( (incr < 0) || ((mem_brk + incr) > mem_max_addr)) {
	errno = ENOMEM;
	fprintf(stderr, "ERROR: mem_sbrk failed. Ran out of memory...\n");
	return (void *)-1;
    }
    mem_brk += incr;
    return (void *)old_brk;
}

/*
 * mem_heap_lo - return address of the first heap byte
 */
void *mem_heap_lo()
{
    return (void *)mem_start_brk;
}

/* 
 * mem_heap_hi - return address of last heap byte

 mem_brk는 사용중인 heap의 마지막 byte에 +1을 한 값이다.
 해당 함수는 heap의 마지막 byte를 가리키는 포인터를 원하므로 mem_brk에 -1을 해준다.
 */
void *mem_heap_hi()
{
    return (void *)(mem_brk - 1);
}

/*
 * mem_heapsize() - returns the heap size in bytes
 */
size_t mem_heapsize() 
{
    return (size_t)(mem_brk - mem_start_brk);
}

/*
 * mem_pagesize() - returns the page size of the system
 */
size_t mem_pagesize()
{
    return (size_t)getpagesize();
}
