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

static void *heap_listp;        // Heap pointer for prologue block
static void *free_listp = NULL; // LIFO 방식 explicit free list root

static void *coalesce(void *bp);
static void *extend_heap(size_t words);
static void place(void *bp, size_t asize);
static void *find_fit(size_t asize);
static void insert_free_block(void *bp);
static void remove_free_block(void *bp);
static bool is_valid(void *bp);

/*
void print_free_list()
{
    char *now_bp = free_listp;
    printf("\n[free list] =========== \n");
    if (free_listp == NULL)
    {
        printf("No list here\n");
    }
    while (now_bp != NULL)
    // while (now_bp != NULL && is_valid(now_bp))
    {
        printf("prev bp position: %p | ", PRED(now_bp));
        printf("bp: %p, ", now_bp);
        printf("bp size: %d, ", GET_SIZE(HDRP(now_bp)));
        printf("bp alloc: %d |", GET_ALLOC(HDRP(now_bp)));
        printf("next bp position: %p |\n", SUCC(now_bp));
        now_bp = (char *)SUCC(now_bp);
    }
    printf("============== \n\n");
}
// void print_all_list()
{
    // initiate bp to epilogue bp
    char *now_bp = heap_listp;
    printf("\nall list ===============\n");
    // if the header does not meet condition, update it
    while (GET_SIZE(HDRP(now_bp)) != 0)
    {
        printf("address %p ", now_bp);
        printf("size %d ", GET_SIZE(HDRP(now_bp)));
        printf("allocated %d \n", GET_ALLOC(HDRP(now_bp)));
        now_bp = NEXT_BLKP(now_bp);
    }
    printf("address %p ", (now_bp));
    printf("size %d ", GET_SIZE(HDRP(now_bp)));
    printf("allocated %d\n", GET_ALLOC(HDRP(now_bp)));
    printf("=========== \n\n");
}
*/

/*
 * mm_init - initialize the malloc package.
 * 최초 가용 블록으로 힙 생성하기
 */
int mm_init(void)
{
    // printf("\ninit start\n");
    /* 힙 영역의 기본 구조를 위한 4 워드(4*WSIZE 바이트) 확보 - 정렬패딩, 프롤로그 헤더/푸터, 에필로그 헤더 */

    if ((heap_listp = mem_sbrk(4 * WSIZE)) == (void *)-1)
    {
        return -1;
    }

    PUT(heap_listp, 0);                            // 정렬 패딩
    PUT(heap_listp + (1 * WSIZE), PACK(DSIZE, 1)); // 프롤로그 헤더
    PUT(heap_listp + (2 * WSIZE), PACK(DSIZE, 1)); // 프롤로그 푸터
    PUT(heap_listp + (3 * WSIZE), PACK(0, 1));     // 에필로그 헤더

    heap_listp += (2 * WSIZE); // payload 위치
    free_listp = NULL;         // 초기화

    /* CHUNKSIZE 바이트(보통 4096B 등)만큼 빈 블록 생성 */
    if (extend_heap(CHUNKSIZE / WSIZE) == NULL)
    {
        return -1;
    }
    // printf("init success\n");
    return 0;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    // printf("\nmalloc 요청 %ld\n", size);

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
        // printf("malloc 요청 성공 %ld %p\n", size, bp);
        return bp;
    }

    extendsize = MAX(asize, CHUNKSIZE);
    if ((bp = extend_heap(extendsize / WSIZE)) == NULL)
    {
        return NULL;
    }

    place(bp, asize);
    // printf("malloc 확장 후 요청 성공 %ld %p\n", size, bp);
    return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    // printf("----- free start ----- \n");
    size_t size = GET_SIZE(HDRP(ptr));

    PUT(HDRP(ptr), PACK(size, 0));
    PUT(FTRP(ptr), PACK(size, 0));

    PRED(ptr) = NULL;
    SUCC(ptr) = NULL;

    coalesce(ptr);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    // printf("----- mm_realloc start ----- \n");
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
    // printf("----- extend_heap start ----- \n");
    char *bp;                                                        // 반환된 새 블록의 payload 시작 주소를 가리킬 포인터
    size_t size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE; // WSIZE를 곱하여 결국 8의 배수가 되도록 보장함

    if ((long)(bp = mem_sbrk(size)) == -1)
    {
        return NULL;
    }

    // printf("extend heap size: %ld\n", size);
    PUT(HDRP(bp), PACK(size, 0));         // 새 빈 블록의 헤더 초기화
    PUT(FTRP(bp), PACK(size, 0));         // 새 빈 블록의 푸터 초기화
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); // 새 에필로그 헤더 설정
    // printf("extend heap setting: bp - %p / Header - %p / Footer - %p / next - %p \n", bp, HDRP(bp), FTRP(bp), NEXT_BLKP(bp));
    // printf("현재 주소 : %p\n", bp);
    // printf("(1) 이전 블록 페이로드 주소 : %p\n", PREV_BLKP(bp));
    // printf("(2) 이전 블록 - footer: %p\n", FTRP(PREV_BLKP(bp)));
    // printf("(3) 이전 블록 - size: %d\n", GET_SIZE(FTRP(PREV_BLKP(bp))));

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
    // printf("----- coalesce start %p ----- \n", bp);
    // print_all_list();
    // printf("bp=%p, prev=%p, next=%p\n", bp, PREV_BLKP(bp), NEXT_BLKP(bp));

    size_t size = GET_SIZE(HDRP(bp));
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));

    if (prev_alloc && next_alloc)
    {
        // printf("----- 양쪽 블록 할당 중 ----- \n");
    }
    else if (prev_alloc && !next_alloc)
    {
        // printf("----- NEXT_BLKP 가용 ----- \n");
        remove_free_block(NEXT_BLKP(bp));
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }
    else if (!prev_alloc && next_alloc)
    {
        // printf("----- PREV_BLKP 가용 ----- \n");
        // printf("%d\n", GET_SIZE(FTRP(PREV_BLKP(bp))));
        remove_free_block(PREV_BLKP(bp));
        // 수정: 이전 블록의 크기 정확히 계산
        size_t prev_size = GET_SIZE(HDRP(PREV_BLKP(bp)));
        size += prev_size;
        bp = PREV_BLKP(bp);
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
        // print_free_list();
    }
    else
    {
        // printf("----- 양쪽 블록 가용 상태 ----- \n");
        remove_free_block(PREV_BLKP(bp));
        remove_free_block(NEXT_BLKP(bp));
        // 수정: 이전 블록과 다음 블록의 크기 정확히 계산
        size_t prev_size = GET_SIZE(HDRP(PREV_BLKP(bp)));
        size_t next_size = GET_SIZE(HDRP(NEXT_BLKP(bp)));
        size += prev_size + next_size;
        bp = PREV_BLKP(bp);
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }

    insert_free_block(bp); // free block 추가
    // print_all_list();
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
    // printf("----- find_fit start ----- \n");
    // print_free_list();

    void *bp = free_listp;
    void *start = bp;

    while (bp != NULL && is_valid(bp))
    {
        if (asize <= GET_SIZE(HDRP(bp)))
        {
            return bp;
        }
        bp = SUCC(bp);
        if (bp == start)
        {
            break;
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
    // printf("----- place start ----- \n");
    // print_all_list();

    size_t csize = GET_SIZE(HDRP(bp)); // csize: free block 크기
    remove_free_block(bp);

    // printf("place: csize=%zu, asize=%zu, rem=%zu\n", csize, asize, csize - asize);

    if ((csize - asize) >= (2 * DSIZE)) // 남는 블록이 free block 최소 크기(24B->정렬 32B)보다 크다면
    {
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));

        // 나머지는 free block으로 분할
        void *remainder = NEXT_BLKP(bp); // 나중에 free block 합침(지연 연결)

        // 블록 정보 업데이트
        PUT(HDRP(remainder), PACK(csize - asize, 0));
        PUT(FTRP(remainder), PACK(csize - asize, 0));

        // free list 업데이트
        insert_free_block(remainder);

        // printf("%p 할당 블록 분할함\n", bp);
        // print_all_list();
    }
    else
    {
        // 분할 불가 - 잔여 block이 넉넉치 않고 fit함
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));

        // printf("%p %s\n", bp, "할당 블록 분할 없음\n");
        // print_all_list();
    }
}

// free block 노드 삽입(LIFO)
void insert_free_block(void *bp)
{
    // printf("----- insert_free_block start ----- \n");
    // print_free_list();
    PRED(bp) = NULL;
    SUCC(bp) = free_listp;
    if (free_listp)
        PRED(free_listp) = bp;
    free_listp = bp;
    // printf("%p insert!\n", bp);
    // print_free_list();
}

// free block 노드 삭제
void remove_free_block(void *bp)
{
    // printf("----- remove_free_block start payload: %p ----- \n", bp);
    // printf("\n[삭제 전 free list]\n");
    // print_free_list();

    // printf(" bp=%p, pred=%p, succ=%p\n", bp, PRED(bp), SUCC(bp));

    if (is_valid(bp))
    {
        if (PRED(bp))
            SUCC(PRED(bp)) = SUCC(bp);
        else
            free_listp = SUCC(bp);
        if (SUCC(bp))
            PRED(SUCC(bp)) = PRED(bp);

        PRED(bp) = NULL;
        SUCC(bp) = NULL;
        // printf("%p remove!\n", bp);
        // printf("\n[삭제 후 free list]\n");
        // print_free_list();
    }
    else
    {
        // printf("힙 영역을 벗어났다!");
    }
}

static bool is_valid(void *bp)
{
    char *lo = (char *)mem_heap_lo();
    char *hi = (char *)mem_heap_hi();
    char *p = (char *)bp;

    if (p >= lo && p <= hi)
        return true;
    else
        return false;
}
