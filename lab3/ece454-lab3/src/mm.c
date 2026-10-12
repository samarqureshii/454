/*
 * ECE454 Lab 3 - Malloc
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>

#include "mm.h"
#include "memlib.h"
#include "math.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "team summer",
    /* First member's first name and last name */
    "Samar:Qureshi",
    /* First member's student number */
    "1009149583",
    /* Second member's first name and last name (do not modify if working alone) */
    "",
    /* Second member's student number (do not modify if working alone) */
    ""
};

/*************************************************************************
 * Basic Constants and Macros
 * You are not required to use these macros but may find them helpful.
*************************************************************************/
#define WSIZE       sizeof(void *)            /* word size (bytes) */
#define DSIZE       (2 * WSIZE)            /* doubleword size (bytes) */
// #define CHUNKSIZE   (1<<7)      /* initial heap size (bytes) */
#define CHUNKSIZE (1<<12)
#define NUM_CLASSES 10

#define MAX(x,y) ((x) > (y)?(x) :(y))

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc) ((size) | (alloc))

/* Read and write a word at address p */
#define GET(p)          (*(uintptr_t *)(p))
#define PUT(p,val)      (*(uintptr_t *)(p) = (val))

/* Read the size and allocated fields from address p */
#define GET_SIZE(p)     (GET(p) & ~(DSIZE - 1))
#define GET_ALLOC(p)    (GET(p) & 0x1)

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp)        ((char *)(bp) - WSIZE)
#define FTRP(bp)        ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* Given block ptr bp, compute address of next and previous blocks */
//for walking through the heap block by block
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

//in segregated free lsits, we need explicit links inside each free block that point to other free blocks, not just adjacent physical blocks
#define NEXT_FREE(bp)  (*(void **)((char *)(bp) + sizeof(size_t)))
#define PREV_FREE(bp)  (*(void **)((char *)(bp) + sizeof(size_t) + sizeof(void *)))

// sizes
#define OVERHEAD       (2*WSIZE)              // 16
#define LINKS_SIZE     (2*sizeof(void*))      // 16
#define ALIGN16(x)     (((x)+15) & ~0xF)
#define MIN_BLOCK_SIZE ALIGN16(DSIZE + LINKS_SIZE)  // 32



// links INSIDE FREE payload
#define FL_NEXT(bp)  (*(void **)((char *)(bp) + 0))
#define FL_PREV(bp)  (*(void **)((char *)(bp) + sizeof(void *)))


void* heap_listp = NULL;
static void *freeLists[NUM_CLASSES]; //pointers to heads of each free list 
//2, 4, 8, 16, 32, 64, 128, 256, 512, 1024
static int get_class_index(size_t size);


static void *extend_heap_noinsert(size_t words) {
    char *bp;
    size_t size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
    if ((bp = mem_sbrk(size)) == (void *)-1) return NULL;

    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));
    return bp; // not inserted into free lists
}

static inline size_t adjust_asize(size_t size) {
    // payload + header/footer, rounded to 16, and at least MIN_BLOCK_SIZE
    size_t need = size + OVERHEAD;              // payload + 16B hdr/ftr
    size_t asz  = ALIGN16(need);                // up to multiple of 16
    if (asz < MIN_BLOCK_SIZE) asz = MIN_BLOCK_SIZE;
    return asz;
}



/**********************************************************
 * mm_check
 * Check the consistency of the memory heap
 * Return nonzero if the heap is consistant.
 *********************************************************/
int mm_check(void) {
    void *bp;
    int ok = 1;

    // 1. check each block in heap
    for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) {
        //size_t size = GET_SIZE(HDRP(bp));
        int alloc = GET_ALLOC(HDRP(bp));

        // a) header/footer match
        if (GET_SIZE(HDRP(bp)) != GET_SIZE(FTRP(bp))) {
            fprintf(stderr, "mm_check error: header/footer size mismatch at %p\n", bp);
            //("CHK MISMATCH", bp);
            ok = 0;
        }

        // b) alignment
        if ((uintptr_t)bp % 16 != 0) {
            fprintf(stderr, "mm_check error: payload alignment wrong at %p\n", bp);
            ok = 0;
        }

        // c) if block is free and next block also free => coalesce missed
        if (!alloc && !GET_ALLOC(HDRP(NEXT_BLKP(bp)))) {
            fprintf(stderr, "mm_check error: two consecutive free blocks \"%p\" and \"%p\" should have been coalesced\n", bp, NEXT_BLKP(bp));
            ok = 0;
        }

    }

    // 2. check free list structure
    for (int cls = 0; cls < NUM_CLASSES; cls++) {
        void *bp2 = freeLists[cls];
        while (bp2 != NULL) {
            size_t size = GET_SIZE(HDRP(bp2));
            int alloc = GET_ALLOC(HDRP(bp2));

            // a) block must be free
            if (alloc) {
                fprintf(stderr, "mm_check error: block %p in free list[%d] but marked allocated\n", bp2, cls);
                ok = 0;
            }

            // b) size fits bucket class
            if (get_class_index(size) != cls) {
                fprintf(stderr, "mm_check error: block %p in wrong bucket %d (should be %d)\n", bp2, cls, get_class_index(size));
                ok = 0;
            }

            // c) pointers valid: next pointer should point to free block etc.
            void *next = FL_NEXT(bp2);
                if (next != NULL) {
                    if ((char *)next < (char *)mem_heap_lo() || (char *)next > (char *)mem_heap_hi()) {
                        fprintf(stderr, "mm_check error: free list node %p has NEXT=%p out of heap\n", bp2, next);
                        ok = 0;
                    } else {
                        // also ensure NEXT points to a free block
                        if (GET_ALLOC(HDRP(next))) {
                            fprintf(stderr, "mm_check error: NEXT=%p of %p is marked allocated\n", next, bp2);
                            ok = 0;
                        }
                        // and linkage symmetry if you want:
                        if (FL_PREV(next) != bp2) {
                            fprintf(stderr, "mm_check error: NEXT->PREV back-link broken at %p (prev should be %p)\n", next, bp2);
                            ok = 0;
                        }
                    }
                }


            // bp2 = NEXT_FREE(bp2);
            bp2 = FL_NEXT(bp2);

        }
    }

    //fprintf(stderr, "[chk] heap=%zu free_sum=%zu\n", mem_heapsize(), free_bytes_sum());

    return ok;
}



/**********************************************************
 * mm_init
 * Initialize the heap, including "allocation" of the
 * prologue and epilogue
 **********************************************************/
 int mm_init(void) 
 {
   if ((heap_listp = mem_sbrk(4*WSIZE)) == (void *)-1)
         return -1;
     PUT(heap_listp, 0);                         // alignment padding
     PUT(heap_listp + (1 * WSIZE), PACK(DSIZE, 1));   // prologue header
     PUT(heap_listp + (2 * WSIZE), PACK(DSIZE, 1));   // prologue footer
     PUT(heap_listp + (3 * WSIZE), PACK(0, 1));    // epilogue header
     heap_listp += DSIZE;

    for(int i = 0; i < NUM_CLASSES; i++){
        freeLists[i] = NULL;
    }

     return 0;
}

static int get_class_index(size_t size) {
    int class = 0;
    size_t threshold = MIN_BLOCK_SIZE;  // start at the minimum block size

    //fprintf(stderr, "get_class_index: size = %zu\n", size);

    while (class < NUM_CLASSES - 1 && size > threshold) {
        class++;
        threshold <<= 1;  // double the threshold
        //fprintf(stderr, " , checking class %d with threshold %zu\n", cls, threshold);
    }

    //fprintf(stderr, "  class chosen = %d\n", cls);
    return class;
}


void insert_free_block(void *bp) {
    size_t size = GET_SIZE(HDRP(bp));
    int cls = get_class_index(size);
    assert(!GET_ALLOC(HDRP(bp)));
    FL_PREV(bp) = NULL;                 // init both
    FL_NEXT(bp) = freeLists[cls];
    if (freeLists[cls]) FL_PREV(freeLists[cls]) = bp;
    freeLists[cls] = bp;
}


void remove_free_block(void *bp) {
    size_t size = GET_SIZE(HDRP(bp));
    int cls = get_class_index(size);
    void *prev = FL_PREV(bp);
    void *next = FL_NEXT(bp);
    if (prev) FL_NEXT(prev) = next; else freeLists[cls] = next;
    if (next) FL_PREV(next) = prev;
    FL_PREV(bp) = FL_NEXT(bp) = NULL;
}


/**********************************************************
 * coalesce
 * Covers the 4 cases discussed in the text:
 * - both neighbours are allocated
 * - the next block is available for coalescing
 * - the previous block is available for coalescing
 * - both neighbours are available for coalescing
 **********************************************************/
// mm.c: coalesce(...)
void *coalesce(void *bp)
{
    size_t size = GET_SIZE(HDRP(bp));

    // check right
    void *nextp = NEXT_BLKP(bp);
    if (!GET_ALLOC(HDRP(nextp))) {
        remove_free_block(nextp);
        size += GET_SIZE(HDRP(nextp));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }

    // check left
    void *prevp = PREV_BLKP(bp);
    if (!GET_ALLOC(HDRP(prevp))) {
        remove_free_block(prevp);
        size += GET_SIZE(HDRP(prevp));
        PUT(HDRP(prevp), PACK(size, 0));
        PUT(FTRP(prevp), PACK(size, 0));
        bp = prevp;
    }

    return bp;
}



/**********************************************************
 * extend_heap
 * Extend the heap by "words" words, maintaining alignment
 * requirements of course. Free the former epilogue block
 * and reallocate its new header
 **********************************************************/
void *extend_heap(size_t words){
    //TODO when you create a big new free block, we insert a free block for it 

    char *bp;
    size_t size;

    //1. get size in bytes, ensure double word alignment  to double size 
    size = (words % 2) ? (words+1) * WSIZE : words * WSIZE;

    //2. get more heap memory 
    if ( (bp = mem_sbrk(size)) == (void *)-1 )
        return NULL;

    //3. init free block header/footer and epilogue header
    PUT(HDRP(bp), PACK(size, 0));                // free block header
    PUT(FTRP(bp), PACK(size, 0));                // free block footer
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));        // new epilogue header

    //4. coalesce if the previous block was free 
    bp = coalesce(bp);

    //5. insert into free list
    //if (GET_SIZE(HDRP(bp)) == 32 && !GET_ALLOC(HDRP(bp))) dump_block("INS 32", bp);

    insert_free_block(bp);

    //fprintf(stderr, "[extend] +%zu bytes, heap=%zu\n", size, mem_heapsize());

    return bp;
}


/**********************************************************
 * find_fit
 * Traverse the heap searching for a block to fit asize
 * Return NULL if no free blocks can handle that size
 * Assumed that asize is aligned
 **********************************************************/

void *find_fit(size_t asize) {
    int class = get_class_index(asize);
    for (int i = class; i < NUM_CLASSES; i++) {
        void *best = NULL;

        for (void *bp = freeLists[i]; bp; bp = FL_NEXT(bp)) {
            size_t sz = GET_SIZE(HDRP(bp));
            if (sz >= asize && (!best || sz < GET_SIZE(HDRP(best)))) {
                best = bp;
                
                if (sz == asize){
                    break; //
                }
            }
        }

        if (best!=NULL){
            return best;
        }
    }
    return NULL;
}



/**********************************************************
 * place
 * Mark the block as allocated
 **********************************************************/
static void place(void *bp, size_t asize) {
    size_t bsize = GET_SIZE(HDRP(bp));


    remove_free_block(bp);

    if (bsize - asize >= MIN_BLOCK_SIZE) {
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        void *rem = NEXT_BLKP(bp);
        size_t rsz = bsize - asize;
        PUT(HDRP(rem), PACK(rsz, 0));
        PUT(FTRP(rem), PACK(rsz, 0));
        insert_free_block(rem);
    } 
    
    else {
        PUT(HDRP(bp), PACK(bsize, 1));
        PUT(FTRP(bp), PACK(bsize, 1));
    }
}


/**********************************************************
 * mm_free
 * Free the block and coalesce with neighbouring blocks
 **********************************************************/
void mm_free(void *bp) {
    if (bp == NULL) return;
    size_t size = GET_SIZE(HDRP(bp));
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    bp = coalesce(bp);

    //if (GET_SIZE(HDRP(bp)) == 32 && !GET_ALLOC(HDRP(bp))) dump_block("INS 32", bp);

    insert_free_block(bp);
}



/**********************************************************
 * mm_malloc
 * Allocate a block of size bytes.
 * The type of search is determined by find_fit
 * The decision of splitting the block, or not is determined
 *   in place(..)
 * If no block satisfies the request, the heap is extended
 **********************************************************/
void *mm_malloc(size_t size) {
    //mm_check();
    if (size == 0) return NULL;

    size_t asize = adjust_asize(size);


    void *bp = find_fit(asize);
    if (bp != NULL) {
        place(bp, asize);
        return bp;
    }

    //extend the heap if no fit found 
    size_t extendsize = MAX(asize, CHUNKSIZE);
    void *new_bp = extend_heap(extendsize/WSIZE);
    if (new_bp == NULL) return NULL;
    place(new_bp, asize);

   //mm_check();
    return new_bp;
}


/**********************************************************
 * mm_realloc
 * Implemented simply in terms of mm_malloc and mm_free
 *********************************************************/
// -----------------------------------------------------------------------------
// Optimized mm_realloc
// - Keeps asize rounding identical to mm_malloc
// - Shrink-in-place with tail split
// - Right-grow into next if free
// - Left-grow into prev if free (move payload backward with memmove)
// - Epilogue extend without free-list churn (extend_heap_noinsert)
// - Fallback: alloc+copy
// -----------------------------------------------------------------------------
void *mm_realloc(void *ptr, size_t size)
{
    if (ptr == NULL) return mm_malloc(size);
    if (size == 0)   { mm_free(ptr); return NULL; }

    size_t asize = adjust_asize(size);
    size_t csize = GET_SIZE(HDRP(ptr));

    // case A: shrink in place
    if (asize <= csize) {
        size_t rem = csize - asize;
        if (rem >= MIN_BLOCK_SIZE) {
            PUT(HDRP(ptr), PACK(asize, 1));
            PUT(FTRP(ptr), PACK(asize, 1));

            void *nbp = NEXT_BLKP(ptr);
            PUT(HDRP(nbp), PACK(rem, 0));
            PUT(FTRP(nbp), PACK(rem, 0));
            nbp = coalesce(nbp);
            insert_free_block(nbp);
        } else {
            PUT(HDRP(ptr), PACK(csize, 1));
            PUT(FTRP(ptr), PACK(csize, 1));
        }
        return ptr;
    }

    // case B: grow right if next is free
    void *nextp = NEXT_BLKP(ptr);
    size_t next_alloc = GET_ALLOC(HDRP(nextp));
    size_t next_size  = GET_SIZE(HDRP(nextp));

    if (!next_alloc && (csize + next_size) >= asize) {
        remove_free_block(nextp);
        size_t newsize = csize + next_size;

        if (newsize - asize >= MIN_BLOCK_SIZE) {
            PUT(HDRP(ptr), PACK(asize, 1));
            PUT(FTRP(ptr), PACK(asize, 1));

            void *rem = NEXT_BLKP(ptr);
            size_t rsz = newsize - asize;
            PUT(HDRP(rem), PACK(rsz, 0));
            PUT(FTRP(rem), PACK(rsz, 0));
            insert_free_block(rem);
        } else {
            PUT(HDRP(ptr), PACK(newsize, 1));
            PUT(FTRP(ptr), PACK(newsize, 1));
        }
        return ptr;
    }

    // case C: grow left if prev is free (copy BEFORE writing headers/footers)
    void *prevp = PREV_BLKP(ptr);
    if (!GET_ALLOC(HDRP(prevp))) {
        size_t prevsz = GET_SIZE(HDRP(prevp));
        if (prevsz + csize >= asize) {
            remove_free_block(prevp);
            size_t newsize = prevsz + csize;

            // move payload backward first to avoid overlap corruption
            size_t old_payload = csize - OVERHEAD;
            size_t copySize = (old_payload < size) ? old_payload : size;
            memmove(prevp, ptr, copySize);

            if (newsize - asize >= MIN_BLOCK_SIZE) {
                PUT(HDRP(prevp), PACK(asize, 1));
                PUT(FTRP(prevp), PACK(asize, 1));

                void *rem = NEXT_BLKP(prevp);
                size_t rsz = newsize - asize;
                PUT(HDRP(rem), PACK(rsz, 0));
                PUT(FTRP(rem), PACK(rsz, 0));
                insert_free_block(rem);
            } else {
                PUT(HDRP(prevp), PACK(newsize, 1));
                PUT(FTRP(prevp), PACK(newsize, 1));
            }
            return prevp; // pointer changes
        }
    }

    // case C2: grow both sides if prev and next are free and combined fit

    int prev_free = !GET_ALLOC(HDRP(prevp));
    int next_free = !GET_ALLOC(HDRP(nextp));

    if (prev_free && next_free) {
        size_t prevsz = GET_SIZE(HDRP(prevp));
        size_t nextsz = GET_SIZE(HDRP(nextp));
        size_t combined = prevsz + csize + nextsz;

        if (combined >= asize) {
            remove_free_block(prevp);
            remove_free_block(nextp);

            // move payload back into prevp BEFORE writing headers
            size_t old_payload = csize - OVERHEAD;
            size_t copySize = (old_payload < size) ? old_payload : size;
            memmove(prevp, ptr, copySize);

            if (combined - asize >= MIN_BLOCK_SIZE) {
                // allocated part
                PUT(HDRP(prevp), PACK(asize, 1));
                PUT(FTRP(prevp), PACK(asize, 1));

                // remainder becomes one free block
                void *rem = NEXT_BLKP(prevp);
                size_t rsz = combined - asize;
                PUT(HDRP(rem), PACK(rsz, 0));
                PUT(FTRP(rem), PACK(rsz, 0));
                insert_free_block(rem);
            } else {
                PUT(HDRP(prevp), PACK(combined, 1));
                PUT(FTRP(prevp), PACK(combined, 1));
            }
            return prevp; // pointer changes
        }
    }

    // case D: at epilogue: extend without list + grow in place
    if (GET_SIZE(HDRP(nextp)) == 0 && GET_ALLOC(HDRP(nextp)) == 1) {
        size_t need  = asize - csize;
        size_t words = (need + (WSIZE - 1)) / WSIZE;

        if (extend_heap_noinsert(words) != NULL) {
            size_t nextsz  = GET_SIZE(HDRP(NEXT_BLKP(ptr)));
            size_t newsize = csize + nextsz;

            if (newsize - asize >= MIN_BLOCK_SIZE) {
                PUT(HDRP(ptr), PACK(asize, 1));
                PUT(FTRP(ptr), PACK(asize, 1));

                void *rem = NEXT_BLKP(ptr);
                size_t rsz = newsize - asize;
                PUT(HDRP(rem), PACK(rsz, 0));
                PUT(FTRP(rem), PACK(rsz, 0));
                insert_free_block(rem);
            } else {
                PUT(HDRP(ptr), PACK(newsize, 1));
                PUT(FTRP(ptr), PACK(newsize, 1));
            }
            return ptr;
        }
        // fall through
    }

    // last resoirt: alloc+copy+free
    void *newptr = mm_malloc(size);
    if (!newptr) return NULL;

    size_t old_payload = csize - OVERHEAD;
    size_t copySize = (old_payload < size) ? old_payload : size;
    memcpy(newptr, ptr, copySize);
    mm_free(ptr);
    return newptr;
}


