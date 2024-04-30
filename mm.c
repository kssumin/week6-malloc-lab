/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */

// #include <cstddef>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "team1",
    /* First member's full name */
    "kimsumin",
    /* First member's email address */
    "201566@jnu.ac.kr",
    /* Second member's full name (leave blank if none) */
    "mando",
    /* Second member's email address (leave blank if none) */
    "ksoomin25@gmail.com"
};

// 정렬의 기준
#define WSIZE 4
#define DSIZE 8
#define CHUNKSIZE (1<<12)

#define ALLOCATED 1
#define FREE 0

#define PACK(size, alloc) ((size) | (alloc))
#define GET(p) (*(unsigned int*)(p))
#define PUT(p, val) ((*(unsigned int*)(p)) = (val))

#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

#define HDRP(bp)  ((char *)bp - WSIZE)
#define FTRP(bp) ((char*)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

#define PREV_BLKP(bp) ((char*)(bp) - GET_SIZE((char*)bp - DSIZE))
#define NEXT_BLKP(bp) ((char*)(bp) + GET_SIZE((char*)bp - WSIZE))

#define MAX(x,y) (x>y?x:y)

#define ALIGNMENT 8
#define MIN_BLOCK_SIZE 16

// 정렬을 위한 size 재조정 8의 배수로 재조정한다
// #define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)
#define ALIGN(size) (DSIZE * ((size + (DSIZE)+(DSIZE - 1)) / DSIZE))

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

// #define FIRST_FIT
// #define BEST_FIT
#define NEXT_FIT

static void* heap_list;
static void* next_fit_search_start_bp;


/**                                                                                                                                                                                          escing)한다.
현 block(bp)의 인접 free block들을 연결(calescing)한다.
이때 연결 이후 free block의 header p
*/
void * coalesce(void *bp)
{
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));

    size_t size = GET_SIZE(HDRP(bp));

    if (prev_alloc==ALLOCATED && next_alloc==ALLOCATED) {
        return bp;
    }

    else if (prev_alloc==FREE && next_alloc==ALLOCATED) {
        size +=GET_SIZE(HDRP(PREV_BLKP(bp)));

        PUT(HDRP(PREV_BLKP(bp)), PACK(size, FREE));
        PUT(FTRP(bp), PACK(size, FREE));

        bp = PREV_BLKP(bp);
    }

    else if (prev_alloc==ALLOCATED && next_alloc==FREE) {
        size +=GET_SIZE(HDRP(NEXT_BLKP(bp)));

        PUT(HDRP(bp), PACK(size, FREE));
        PUT(FTRP(bp), PACK(size, FREE));
    }
    else {
        size= size + GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(HDRP(NEXT_BLKP(bp)));

        PUT(HDRP(PREV_BLKP(bp)), PACK(size, FREE));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, FREE));

        bp = PREV_BLKP(bp);
    }

    next_fit_search_start_bp = bp;
    return bp; 
}

// 추가적인 heap 메모리를 할당한다
void * extend_heap(size_t words)
{
    char * bp;
    size_t size;

    // 정렬을 맞추기 위해
    size = (words % 2) ? (words+1) * WSIZE : words * WSIZE;

    // 사이즈를 할당한다
    if ((long)(bp = mem_sbrk(size))==-1) {
        return NULL;
    }

    PUT(HDRP(bp), PACK(size, FREE));
    PUT(FTRP(bp), PACK(size, FREE));
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, ALLOCATED));

    return coalesce(bp);
}

/* 
 * mm_init - initialize the malloc package.

 정렬을 위한 패딩, prologue(header, footer), epilogue header가 필요하다.
 즉, 처음에는 word 사이즈의 block이 4개 필요하다.
 */
int mm_init(void)
{
    if((heap_list = mem_sbrk(4 * WSIZE)) == (void*)-1)
    {
        return -1;
    }

    // 정렬을 위한 padding block
    PUT(heap_list, 0);
    // prologue block
    PUT(heap_list + (WSIZE*1), PACK(DSIZE, ALLOCATED));
    PUT(heap_list+ (WSIZE*2), PACK(DSIZE, ALLOCATED));
    //epilogue block
    PUT(heap_list + (WSIZE*3), PACK(0, ALLOCATED));

    heap_list +=(WSIZE*2);
    next_fit_search_start_bp = heap_list;

    if (extend_heap(CHUNKSIZE / WSIZE) == NULL) {
        return -1;
    }
    return 0;
}

/* 
 * block list에서 free block들을 찾는다.
 * 요청한 size에 맞는 free block을 찾으면 바로 해당 block에 할당한다.(first fit)
 */
void *find_fit(size_t asize)
{   
    #ifdef FIRST_FIT
        void *bp;

        for (bp = heap_list; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp))
        {
            if (GET_ALLOC(HDRP(bp))==FREE && (asize <= GET_SIZE(HDRP(bp))))
            {
                return bp;
            }
        }

        return NULL;

    #elif defined(BEST_FIT)
        void *bp;
        void *best_bp = NULL;

        for (bp = heap_list; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp))
        {   
            if (GET_SIZE(HDRP(bp)) >= asize && GET_ALLOC(HDRP(bp))==FREE) {
                if (best_bp == NULL) {
                    best_bp = bp;
                    continue;
                }
                else
                {
                    if (GET_SIZE(HDRP(best_bp)) > GET_SIZE(HDRP(bp))) {
                        best_bp = bp;
                    }
                }
            }
        }

        return best_bp;

    #else
        void *bp;

        for (bp = next_fit_search_start_bp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp))
        {   
            if (GET_SIZE(HDRP(bp)) >= asize && GET_ALLOC(HDRP(bp))==FREE) {
                return bp;
            }
        }

        // free block을 찾지 못 했을 경우
        for (bp = heap_list; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp))
        {   
            if (GET_SIZE(HDRP(bp)) >= asize && GET_ALLOC(HDRP(bp))==FREE) {
                return bp;
            }
        }
        return NULL;
    
    #endif
}

/**
free block의 pointer를 전달받아
size만큼을 할당한다.
이때 해당 free block에 할당했을 때 padding으로 block의 최소 크기 이상이 남는다면 free block을 분할하여 할당한다
*/
void place(void* bp, size_t want_allocate_size)
{
    size_t have_size= GET_SIZE(HDRP(bp));

    if ((have_size - want_allocate_size) >= 2*DSIZE) {
        PUT(HDRP(bp), PACK(want_allocate_size, ALLOCATED));
        PUT(FTRP(bp), PACK(want_allocate_size, ALLOCATED));

        bp = NEXT_BLKP(bp);
        next_fit_search_start_bp = bp;

        size_t rest_size = have_size - want_allocate_size;
        PUT(HDRP(bp), PACK(rest_size, FREE));
        PUT(FTRP(bp), PACK(rest_size, FREE));
    }
    // 분할하면 나머지 block은 최소 block의 크기를 만족하지 못 한다.
    else {
        PUT(HDRP(bp), PACK(have_size, ALLOCATED));
        PUT(FTRP(bp), PACK(have_size, ALLOCATED));

        next_fit_search_start_bp = NEXT_BLKP(bp);
    }
}


/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    void *bp;
    size_t adjust_size;
    size_t extend_heap_size;

    if (size <= 0) {
        return NULL;
    }

    // 요청한 size가 footer+header를 포함한 2word보다 작다면
    // 최소 블록의 크기로 size를 재조정한다.
    if (size <= DSIZE) {
        adjust_size = MIN_BLOCK_SIZE;
    }

    // 요청한 block의 크기를 정렬을 하기 위해 재조정한다.
    else
    {
        adjust_size = ALIGN(size);
    }
   
    if ((bp = find_fit(adjust_size))!=NULL) {
        place(bp, adjust_size);
        return bp;
    }

    extend_heap_size = MAX(adjust_size, CHUNKSIZE);
    if ((bp = extend_heap(extend_heap_size/WSIZE))== NULL) {
        return NULL;
    }
    place(bp, adjust_size);
    return bp;    
}

/*
 * alloated된 ptr block을 free시킨다.
 * 즉시 연결 : block을 해제시키면서 인접의 free block과 연결한다.
 */
void mm_free(void *bp)
{
    size_t size = GET_SIZE(HDRP(bp));

    PUT(HDRP(bp), PACK(size, FREE));
    PUT(FTRP(bp), PACK(size, FREE));

    coalesce(bp);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free

ptr이 가리키는 메모리의 주소 크기를 size 바이트로 확장한다.
이때, 연속된 메모리를 할당할 수 없을 경우 새로운 영역을 할당한 후 기존 요소들을 복사하여 새 메모리 주소를 반환
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *newptr;
    size_t copySize = GET_SIZE(HDRP(ptr));
    size = ALIGN(size);

    if (ptr== NULL) {
        return mm_malloc(size);
    }

    if (size <=0) {
        mm_free(ptr);
        return NULL;
    }

    if (size + DSIZE <= copySize) {
        return ptr;
    }

    // 다음 block이 free일 떄
    if (GET_ALLOC(HDRP(NEXT_BLKP(ptr)))==FREE) {
        if (copySize + GET_SIZE(HDRP(NEXT_BLKP(ptr))) - DSIZE >=size) {
            PUT(HDRP(ptr), PACK(copySize + GET_SIZE(HDRP(NEXT_BLKP(ptr))), ALLOCATED));
            PUT(FTRP(ptr), PACK(copySize + GET_SIZE(HDRP(NEXT_BLKP(ptr))), ALLOCATED));

            place(ptr, size);

            return ptr;
        }
    }
    
    newptr = mm_malloc(size);
    if (newptr == NULL){
        return NULL;
    }
    
    if (size < copySize) {
        copySize = size;
    }

    //bp메모리에 있는 값을 num copysize만큼 복사해서 newptr에 넣는다.
    memcpy(newptr, ptr, copySize);
    mm_free(ptr);
    return newptr;
}













