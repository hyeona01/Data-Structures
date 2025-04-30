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
#include <stdbool.h>
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

#define WSIZE sizeof(void *)
// #define WSIZE 4
#define DSIZE (2 * WSIZE)
// #define DSIZE 8
#define CHUNKSIZE (1 << 12) // 4KB

#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define PACK(size, alloc) ((size) | (alloc)) // 할당 여부를 마지막 비트에 저장

#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))

#define GET_SIZE(p) (GET(p) & ~0x7) // 사이즈
#define GET_ALLOC(p) (GET(p) & 0x1) // 마지막 비트의 할당 여부 반환

#define HDRP(bp) ((char *)(bp) - WSIZE)                      // 1 word 패딩 이후
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE) // 2 word header 이후

// 다음 블록의 페이로드 주소 - 현재 포인터 1 word 전 : 현재 block 헤더 / 헤더에서 해당 block 사이즈를 얻어옴
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
// 이전 블록의 페이로드 주소 - 현재 포인터 2 word 전 : 이전 block 푸터 / 이전 block 사이즈를 얻어옴
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

#define PRED(bp) (*(void **)(bp))
#define SUCC(bp) (*(void **)((char *)(bp) + WSIZE)) // bp에서 1 word(pointer size) 만큼 이동

#define MINBLOCKSIZE 32
#define LISTLIMIT 20
static char *seg_free_lists[LISTLIMIT];

static void *coalesce(void *bp);
static void *extend_heap(size_t words);
static void place(void *bp, size_t asize);
static void *find_fit(size_t asize);
static void insert_free_block(void *bp);
static void remove_free_block(void *bp);
static int get_list_index(size_t size);

/*
// 디버깅을 위한 함수: free list를 출력함
// void print_free_list()
{
    char *now_bp = free_listp;
    // printf("\n[free list] =========== \n");
    if (free_listp == NULL)
    {
        // printf("No list here\n");
    }
    while (now_bp != NULL)
    // while (now_bp != NULL && is_valid(now_bp))
    {
        // printf("prev bp position: %p | ", PRED(now_bp));
        // printf("bp: %p, ", now_bp);
        // printf("bp size: %d, ", GET_SIZE(HDRP(now_bp)));
        // printf("bp alloc: %d |", GET_ALLOC(HDRP(now_bp)));
        // printf("next bp position: %p |\n", SUCC(now_bp));
        now_bp = (char *)SUCC(now_bp);
    }
    // printf("============== \n\n");
}

// 디버깅을 위한 함수: 전체 힙 영역을 출력함
// void print_all_list()
{
    // initiate bp to epilogue bp
    char *now_bp = heap_listp;
    // printf("\nall list ===============\n");
    // if the header does not meet condition, update it
    while (GET_SIZE(HDRP(now_bp)) != 0)
    {
        // printf("address %p ", now_bp);
        // printf("size %d ", GET_SIZE(HDRP(now_bp)));
        // printf("allocated %d \n", GET_ALLOC(HDRP(now_bp)));
        now_bp = NEXT_BLKP(now_bp);
    }
    // printf("address %p ", (now_bp));
    // printf("size %d ", GET_SIZE(HDRP(now_bp)));
    // printf("allocated %d\n", GET_ALLOC(HDRP(now_bp)));
    // printf("=========== \n\n");
}
*/

/*
 * mm_init - initialize the malloc package.
 * 최초 가용 블록으로 힙 생성하기
 */
int mm_init(void)
{
    // seg_free_lists 초기화
    for (int i = 0; i < LISTLIMIT; ++i)
    {
        seg_free_lists[i] = NULL;
    }

    char *heap_listp;
    /* 힙 영역의 기본 구조를 위한 4 워드(4*WSIZE 바이트) 확보 - 정렬패딩, 프롤로그 헤더/푸터, 에필로그 헤더 */
    if ((heap_listp = mem_sbrk(4 * WSIZE)) == (void *)-1)
    {
        return -1;
    }

    PUT(heap_listp, 0);                            // 정렬 패딩
    PUT(heap_listp + (1 * WSIZE), PACK(DSIZE, 1)); // 프롤로그 헤더
    PUT(heap_listp + (2 * WSIZE), PACK(DSIZE, 1)); // 프롤로그 푸터
    PUT(heap_listp + (3 * WSIZE), PACK(0, 1));     // 에필로그 헤더

    /* CHUNKSIZE 바이트(보통 4096B 등)만큼 빈 블록 생성 */
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

    if (size <= DSIZE) // payload
    {
        asize = 2 * DSIZE; // 헤더+푸터(8B) payload(8B)
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
    if (ptr == NULL)
        return mm_malloc(size);
    if (size == 0)
    {
        mm_free(ptr);
        return NULL;
    }

    size_t old_size = GET_SIZE(HDRP(ptr));
    size_t new_size = ALIGN(size + DSIZE);

    // Shrinking
    if (new_size <= old_size)
    {
        if (old_size - new_size >= MINBLOCKSIZE)
        {
            PUT(HDRP(ptr), PACK(new_size, 1));
            PUT(FTRP(ptr), PACK(new_size, 1));

            void *next = NEXT_BLKP(ptr);
            PUT(HDRP(next), PACK(old_size - new_size, 0));
            PUT(FTRP(next), PACK(old_size - new_size, 0));
            coalesce(next);
        }
        return ptr;
    }

    // Try merging with next free block
    void *next = NEXT_BLKP(ptr);
    size_t next_size = GET_SIZE(HDRP(next));
    if (next_size > 0 && !GET_ALLOC(HDRP(next)) && (old_size + next_size >= new_size))
    {
        remove_free_block(next);
        size_t total_size = old_size + next_size;
        PUT(HDRP(ptr), PACK(total_size, 1));
        PUT(FTRP(ptr), PACK(total_size, 1));
        return ptr;
    }

    // Fallback: allocate new and copy
    void *newptr = mm_malloc(size);
    if (newptr == NULL)
        return NULL;

    size_t copySize = old_size - DSIZE;
    if (size < copySize)
        copySize = size;
    memcpy(newptr, ptr, copySize);
    mm_free(ptr);
    return newptr;
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
    char *bp;                                                        // 반환된 새 블록의 payload 시작 주소를 가리킬 포인터
    size_t size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE; // WSIZE를 곱하여 결국 8의 배수가 되도록 보장함

    if (size < MINBLOCKSIZE)
    {
        size = MINBLOCKSIZE;
    }

    if ((long)(bp = mem_sbrk(size)) == -1)
    {
        return NULL;
    }

    PUT(HDRP(bp), PACK(size, 0));         // 새 빈 블록의 헤더 초기화
    PUT(FTRP(bp), PACK(size, 0));         // 새 빈 블록의 푸터 초기화
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); // 새 에필로그 헤더 설정

    return coalesce(bp);
}

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
    size_t size = GET_SIZE(HDRP(bp));
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));

    if (prev_alloc && next_alloc)
    {
        /*----- 양쪽 블록 할당 중 -----*/
    }
    else if (prev_alloc && !next_alloc)
    {
        /*----- NEXT_BLKP 가용 -----*/
        remove_free_block(NEXT_BLKP(bp));
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }
    else if (!prev_alloc && next_alloc)
    {
        /*----- PREV_BLKP 가용 -----*/
        remove_free_block(PREV_BLKP(bp));
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        bp = PREV_BLKP(bp);
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }
    else
    {
        /*----- 양쪽 블록 가용 -----*/
        remove_free_block(PREV_BLKP(bp));
        remove_free_block(NEXT_BLKP(bp));
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(HDRP(NEXT_BLKP(bp)));
        bp = PREV_BLKP(bp);
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }

    insert_free_block(bp); // free block 추가
    return bp;
}

/*
 * find_fit 함수
 * : 요청한 크기의 free block이 있는 지 확인하는 함수
 *
 * size에 맞는 free block이 있다면 포인터를 반환한다.
 */
void *find_fit(size_t asize)
{
    int idx = get_list_index(asize);
    void *bp;
    void *best_fit = NULL;
    size_t best_size = (size_t)-1;

    for (int i = idx; i < LISTLIMIT; ++i) // 요청한 크기 이상의 블록 리스트 찾기(first fit)
    {
        bp = seg_free_lists[i];

        while (bp != NULL)
        {
            // if (asize <= GET_SIZE(HDRP(bp)))
            size_t bsize = GET_SIZE(HDRP(bp));
            if (asize <= bsize && bsize < best_size)
            {
                // return bp;
                best_fit = bp;
                best_size = bsize;
                if (bsize == asize) // 크기가 정확히 일치함
                    return bp;
            }
            bp = SUCC(bp);
        }
        if (best_fit != NULL)
            return best_fit;
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
    remove_free_block(bp);

    if ((csize - asize) >= MINBLOCKSIZE)
    {
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));

        // 나머지는 free block으로 분할
        bp = NEXT_BLKP(bp); // 나중에 free block 합침(지연 연결)

        // 블록 정보 업데이트
        PUT(HDRP(bp), PACK(csize - asize, 0));
        PUT(FTRP(bp), PACK(csize - asize, 0));

        // free list 업데이트
        insert_free_block(bp);
    }
    else
    {
        // 분할 불가 - 잔여 block이 넉넉치 않고 fit함
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
    }
}

// free block 노드 삽입(주소순)
void insert_free_block(void *bp)
{
    int idx = get_list_index(GET_SIZE(HDRP(bp))); // 사이즈에 맞는 인덱스 찾기
    char *head = seg_free_lists[idx];             // 해당 인덱스의 포인터

    if (head == NULL)
    {
        PRED(bp) = NULL;
        SUCC(bp) = NULL;
        seg_free_lists[idx] = bp;
        return;
    }

    void *curr = head;
    void *prev = NULL;

    while (curr != NULL && bp > curr)
    {
        prev = curr;
        curr = SUCC(curr);
    }

    PRED(bp) = prev;
    SUCC(bp) = curr;

    // 이전 노드가 있다면
    if (prev != NULL)
    {
        SUCC(prev) = bp;
    }
    else
    {
        seg_free_lists[idx] = bp;
    }

    // 다음 노드가 있다면
    if (curr != NULL)
    {
        PRED(curr) = bp;
    }
}

// free block 노드 삭제
void remove_free_block(void *bp)
{
    int idx = get_list_index(GET_SIZE(HDRP(bp)));

    if (bp == seg_free_lists[idx]) // 이 경우가 더 이해가 쉬움
    {
        seg_free_lists[idx] = SUCC(bp);
        if (seg_free_lists[idx] != NULL)
        {
            PRED(seg_free_lists[idx]) = NULL;
        }
    }
    else
    {
        if (PRED(bp))
        {
            SUCC(PRED(bp)) = SUCC(bp);
        }
        if (SUCC(bp))
        {
            PRED(SUCC(bp)) = PRED(bp);
        }
    }
}

static int get_list_index(size_t size)
{
    int idx = 0;
    size >>= 4; // 16으로 나눔(최소 단위 블록 크기)

    while (size > 1 && idx < LISTLIMIT - 1)
    {
        size >>= 1; // 2씩 나눔
        idx++;
    }
    return idx;
}