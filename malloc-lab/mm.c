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
    "hyeona",
    /* First member's full name */
    "Seol Hyeona",
    /* First member's email address */
    "shappy0209@naver.com",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8 // 블록 크기로 8바이트의 배수로 맞추기 위함

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7) // 임의의 사이즈를 8의 배수로 만들어주는 함수

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

// 들어가기
#define WSIZE 4
#define DSIZE 8
#define CHUNKSIZE (1 << 12)

#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define PACK(size, alloc) ((size) | (alloc)) // 할당 여부를 마지막 비트에 저장

#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))

#define GET_SIZE(p) (GET(p) & ~0x7) // 사이즈
#define GET_ALLOC(p) (GET(p) & 0x1) // 마지막 비트의 할당 여부 반환

#define HDRP(bp) ((char *)(bp) - WSIZE)                      // 1 word 패딩 이후
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE) // 2 word header 이후

// 현재 payload의 포인터에서 1 word 전 : 현재 block 헤더 / 헤더에서 해당 block 사이즈를 얻어옴
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
// 현재 payload의 포인터에서 2 word 전 : 이전 block 푸터 / 이전 block 사이즈를 얻어옴
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

// heap_listp 정의
static void *heap_listp;

/*
 * coalesce
 * : 반환된 free block을 앞, 뒤 free block과 병합하는 함수
 *
 * 블록 bp가 해제(free)된 뒤에,
 * 이전 블록과 다음 블록의 상태를 보고 인접한 빈 블록들과 병합(merge)한다.
 *
 * 경계 태그 연결을 사용해서 상수 시간에 인접 가용 블록들과 통합한다.
 */
static void *coalesce(void *bp)
{
    if (bp == NULL)
    {
        return NULL;
    }

    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp))); // bp 바로 이전 블록의 payload 시작 포인터 의 푸터 의 할당 비트
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp))); // bp 바로 다음 블록의 payload 시작 포인터 의 푸터 의 할당 비트
    size_t size = GET_SIZE(HDRP(bp));

    // 이전, 다음 블록 모두 할당
    if (prev_alloc && next_alloc)
    {
        return bp;
    }
    // 다음 블록 가용 상태
    else if (prev_alloc && !next_alloc)
    {
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0)); // 사이즈, 할당 여부
        PUT(FTRP(bp), PACK(size, 0)); // 이미 업데이트된 header를 참조하여 새로운 footer주소에 도착
    }
    // 이전 블록 가용 상태
    else if (!prev_alloc && next_alloc)
    {
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 0));            // size 변경
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0)); // 이전 블록의 header 주소를 현재 블록의 header 주소로 변경
        bp = PREV_BLKP(bp);
    }
    // 이전 블록, 다음 블록 가용 상태
    else
    {
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0)); // 이전 블록의 header 주소로 업데이트
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0)); // 다음 블록의 footer 주소로 업데이트
        bp = PREV_BLKP(bp);
    }
    return bp;
}

/*
 * extend_heap
 * : mm_malloc에서 적당한 free block을 찾지 못한 경우 heap을 확장하는 함수
 *
 * 힙을 지정된 크기만큼 늘리고,
 * 그 새 영역을 하나의 빈(free) 블록으로 초기화한 뒤,
 * 인접한 빈 블록들과 병합(coalesce)해준다.
 */
static void *extend_heap(size_t words)
{
    char *bp; // 반환된 새 블록의 payload 시작 주소를 가리킬 포인터
    size_t size;

    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE; // WSIZE를 곱하여 결국 8의 배수가 되도록 보장함
    if ((long)(bp = mem_sbrk(size)) == -1)                    // 힙 확장 + 확장 전 포인터 bp에 저장
    {
        return NULL;
    }

    // 새 빈 블록의 헤더와 푸터 초기화
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    // 새 에필로그 헤더 설정
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));

    // 인접 블록 병합
    return coalesce(bp);
}

/*
 * find_fit 함수
 * : 요청한 크기의 free block이 있는 지 확인하는 함수
 *
 * size에 맞는 free block이 있다면 포인터를 반환한다.
 */
void *find_fit(size_t asize)
{
    void *bp;

    // first fit
    for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp))
    {
        if (!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp))))
        {
            return bp;
        }
    }
    return NULL;
}

/*
 * place 함수
 * : free block을 할당하여 초기화하는 함수
 *
 * 헤더와 푸터를 세팅한다.
 */
void place(void *bp, size_t asize)
{
    size_t csize = GET_SIZE(HDRP(bp)); // csize: free block 크기

    if ((csize - asize) >= (2 * DSIZE)) // 남는 블록이 재사용 가능한 최소 크기보다 크다면
    {
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));

        // 나머지는 free block으로 분할
        bp = NEXT_BLKP(bp); // 나중에 free block 합침(지연 연결)
        PUT(HDRP(bp), PACK(csize - asize, 0));
        PUT(FTRP(bp), PACK(csize - asize, 0));
    }
    else
    {
        // 잔여 block이 넉넉치 않고 fit함
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
    }
}

/*
 * mm_init - initialize the malloc package.
 * 최초 가용 블록으로 힙 생성하기
 */
int mm_init(void)
{
    /* 1) 힙 영역의 기본 구조를 위한 4 워드(4*WSIZE 바이트) 확보 */

    if ((heap_listp = mem_sbrk(4 * WSIZE)) == (void *)-1)
    {
        return -1;
    }

    /* 2) Alignment padding (정렬 패딩) */
    PUT(heap_listp, 0);
    /* 3) Prologue header: 크기 DSIZE(=8바이트), 할당(1) */
    PUT(heap_listp + (1 * WSIZE), PACK(DSIZE, 1));
    /* 4) Prologue footer: 크기 DSIZE(=8바이트), 할당(1) */
    PUT(heap_listp + (2 * WSIZE), PACK(DSIZE, 1));
    /* 5) Epilogue header: 크기 0, 할당(1) → 힙 끝 표시 */
    PUT(heap_listp + (3 * WSIZE), PACK(0, 1));
    /* 6) heap_listp를 첫 번째 유효 블록의 payload 시작점으로 조정 */
    heap_listp += (2 * WSIZE);

    /* 7) CHUNKSIZE 바이트(보통 4096B 등)만큼 빈 블록 생성 */
    if (extend_heap(CHUNKSIZE / WSIZE) == NULL)
    {
        return -1;
    }
    return 0;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    char *bp;
    size_t asize;      // 실제 할당할 블록 크기
    size_t extendsize; // free 블록이 없을 때 확장 요청할 크기

    if (size == 0)
    {
        return NULL;
    }

    if (size <= DSIZE)
    {
        asize = 2 * DSIZE; // 헤더+푸터(8B) 페이로드(8B)
    }
    else
    {
        asize = DSIZE * ((size + (DSIZE) + (DSIZE - 1)) / DSIZE);
    }

    if ((bp = find_fit(asize)) != NULL)
    {
        place(bp, asize);
        return bp;
    }

    extendsize = MAX(asize, CHUNKSIZE);
    if ((bp = extend_heap(extendsize / WSIZE)) == NULL)
    {
        return NULL;
    }
    place(bp, asize);
    return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    size_t size = GET_SIZE(HDRP(ptr));

    PUT(HDRP(ptr), PACK(size, 0));
    PUT(FTRP(ptr), PACK(size, 0));
    coalesce(ptr);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;

    newptr = mm_malloc(size);
    if (newptr == NULL)
        return NULL;
    copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
    if (size < copySize)
        copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}