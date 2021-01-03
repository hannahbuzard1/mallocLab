/*

247 Textbook Citation:
Randal E. Bryant and David R. O'Hallaron. 2015. Computer Systems:
A Programmer's Perspective plus MasteringEngineering with Pearson eText --
Access Card Package (3rd. ed.). Pearson.
--Used Section 10.9 as well as 10.9 solutions in the back of the textbook
Any Function/idea taken from the textbook is cited in the code

High Level Code Description and Function descriptions:
My implementation of dynamic memroy allocation uses an explicit free list (versus an implicit free list - I can now
access both previous and next) by using a pointer to the head of the free list. I initially attempted to implement an explicit free list
using structs but ran into so many errors and issues that I decided to get rid of the struct (however some of the ideas from my
struct implementation can be seen in my comments). A lot of my code is based off of the implicit list implementation given in the 247 Textbook
that is cited above. However, when using an explicit free list, I found that I just needed to manually add and remove blocks from the list.
So, I wrote two functions which insert and remove blocks from my explicit free list and then anytime a block is "deallocated", I call
listInsert to add it to the free list and anytime a block is allocated (or reallocated) I remove it from the free list by calling
listRemove.

mm.init: Creates an initial empty heap and creates the first free block (in the extend_heap call). In the textbook, mem_sbrk is called with (4 * WSIZE)
however this had to be increased to account for the next and previous blocks for the free list.

mm.malloc: My mm.malloc implementation that I use is from my 247 textbook. Here is an explanation of what mm.malloc does:
mm.malloc calls find_fit (which is a first fit search implementation) which searches the free list to see if there is an unallocated
amount of space large enough to hold the requested size. If there is, then find_fit returns a pointer to this space. If space is found,
my split function is called which ensures that space in the free list is not wasted. For instance, if find_fit returns a block of size 30 bytes
but I only need 16, then the "free" block would be split into two blocks (unallocated and allocated). If a fit is not found, then the heap
must be extended, so extend_heap is called. A pointer to the extended block in the heap is returned (and split is called again).

mm.realloc: For my mm.realloc code, I followed the description that was given in the writeup and split my function into 3 cases.
Case 1: Pointer is NULL -> call is equivalent to mm_malloc
Case 2: Size given is 0 -> call is equivalent to mm_free
Case 3: Pointer is not NULL -> If the newsize is less than the current block's size, then just return the current block
(nothing needs to be resized). If the newsize is greater than the current block's size, then malloc size for the new block and copy
data from pointer that was passed in to the new pointer (using memcpy which means that bp will need to be freed when the function completes).
Return new pointer with new size. DONT FORGET to free the previous pointer (no longer needed because it was resized). Split also needs to
be called here again because a new block is being allocated, so we need to ensure that we are only taking the space that is needed.

mm.free: My mm.free implementation is essentially the same as the textbook. The pointer to the block passed in needs to be deallocated (flip
the bit so that it is marked as deallocated or "free") and add to the free list. Coalesce is called to see if the newly freed block can be
coalesced (or combined) with any adjacent free blocks. Because of the coalesce call, the insert to free list doesn't need to be called in
the free function because it is done in coalesce.

coalesce: Coalesce inserts a block into the free list and in doing so, checks to see if either of its adjacent blocks can be combined (coalesced)
and if so, the sizes of the unallocated blocks are combined and the blocks are combined (resulting in remove/insert calls to the free list).
These are the 4 cases:
Case 1: Neither the previous or next blocks are unallocated. In this case, the new free block cannot be combined with any other blocks so
the new block is inserted and returned
Case 2: The previous block is allocated but the next block is not. In this case, the next block needs to be combined with the block passed
to the function, so the next block can be removed from the free list since it will be combined with *bp. *bp is then set to have the
new size and is deallocted, added to the free list and returned.
Case 3: The previous block is unallocated but the next block is allocated. In this case, *bp can be removed from the free list because
the previous block is combined with the size of *bp (new area is from the header of the previous block to the footer of bp), set to
point to the new sized area, deallocated, added to the free list and returned.
Case 4: The last case is where both the previous and next blocks are unallocated. In this case, all 3 blocks can be combined, so the next
and previous blocks are removed from the free list. Size is updated to combine all three blocks and then the new area is set to be
from the header of the previous block to the footer of the next block. The blocks are deallocated and then a pointer to the previous block
(with the new size) is added to the free list and returned.

dealloc/alloc: The MACROS given in the 247 textbook use PUT and PACK to set the value of a pointer and set the allocation bit (the PACK
macro). Typically, but not in every case, the header and the footer just need to be updated. So, it saved me lines of code to just write
this as a function versus typing PUT and PACK all over my code. I chose to seperate dealloc from alloc instead of just passing in
1 or 0 to PACK with because it made it easier to see what was occurring in my code.

extend_heap: Requests additional memory and expands the heap by this amount. Extend heap returns a double-word aligned memory block
(meaning that it is a multiple of 8) and then updates the free block header and footer and the new epilogue block.
Coalesce is called before returning in case the heap ended with a free block (so now the free block and new block would need to be
merged). A pointer to the new block is returned.
immediately following the header of the epilogue block

insert/remove from free list: My insert and remove functions either insert a list at the head of my free list (using FIFO implementation) or
remove a block from the free list. The way I do this is standard for adding or removing from a doubly linked list. I use the two macros I wrote
to access the previous and next blocks in the list and then use these to implement a non-struct version of what my struct code did which is
as follows:
For remove: bp->prev->next = bp -> next and bp->next->prev = bp -> prev and I needed to account
 for the case where bp is the head of the free list, in which case the head of the list just needs to be updated instead of
bp -> prev -> next
For insert:  bp->next = start of free list and start of free list -> prev = bp
and bp -> prev = NULL. Then bp is the new start of the free list.
*/

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "memlib.h"
#include "mm.h"

 team_t team = {
     /* Team name */
     "buzardh@140.160.140.70",
     /* First member's full name */
     "Hannah Buzard",
     /* First member's email address */
     "buzardh@wwu.edu",
     /* Second member's full name (leave blank if none) */
     "",
     /* Second member's email address (leave blank if none) */
     ""
 };

//* Basic constants and macros: Given in 247 Textbook (cited in description)*/
#define WSIZE      4 /* Word and header/footer size (bytes) */
#define DSIZE      8 /*Double word size */
#define CHUNKSIZE  (1 << 12)      /* Initial Heap Size */

#define ALIGNMENT 8  /* ALIGNMENT size provided by Meehan  : 8 */

/* all following MACROS given in 247 textbook (cited in description) */
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define PACK(size, alloc)  ((size) | (alloc))
#define GET(p)       (*(size_t *)(p))
#define PUT(p, val)  (*(size_t *)(p) = (val))
#define GET_SIZE(p)   (GET(p) & ~(0x7))
#define GET_ALLOC(p)  (GET(p) & 0x1)
#define HDRP(bp)  ((void *)(bp) - WSIZE)
#define FTRP(bp)  ((void *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)
#define NEXT_BLKP(bp) ((void *)(bp) + GET_SIZE(((void *)(bp) - WSIZE)))
#define PREV_BLKP(bp) ((void *)(bp) - GET_SIZE(((void *)(bp) - DSIZE)))


/* These two MACROS were not entirely written by me: Citation: UW CSE 351 Section 9 Lecture Slides (I used to go to UW
so I knew which course to look at for help with dynamic mem allocation. Here is the course material I referenced when looking
at macros for next and prev free pointers: https://courses.cs.washington.edu/courses/cse351/20su/labs/lab5.php
(the videos on MACROS & starter code) */
#define NEXTFREE(bp)  (*(void **)(bp + WSIZE))
#define PREVFREE(bp)  (*(void **)(bp))

//Pointer for free list Head
char *freeList = 0;
/* Function prototypes */
void *coalesce(void *bp);
void *extend_heap(size_t words);
void *find_fit(size_t asize);
void split(void *bp, size_t asize);
void deallocBit(void *bp, size_t size);
void allocBit(void *bp, size_t size);
void listInsert(void *bp);
void listRemove(void *bp);
/*
Initialize empty heap. Returns -1 if unsuccessful or 0 on success.
/* need to allocate space for 6 elements in the heap block (all of size WSIZE) : prologue header, prologue footer, epilogue header, payload,
//and a next and previous that is used for the free list (this is why instead of 4 * WSIZE that is given in the textbook, it must
  be 6 * WSIZE)*/
int mm_init(void) {
  if ((freeList = mem_sbrk(6*WSIZE)) == NULL)
    return -1;

  PUT(freeList, 0);                            /* Alignment padding */
  PUT(freeList + (WSIZE), PACK(ALIGNMENT, 1)); /* Prologue header */
  PUT(freeList + (DSIZE), PACK(ALIGNMENT, 1)); /* Prologue footer */
  PUT(freeList + (WSIZE + DSIZE), PACK(0, 1));     /* Epilogue header */
  freeList = freeList + DSIZE;

  /* Extend the empty heap with a free block of minimum possible block size  */
  if (extend_heap(CHUNKSIZE/WSIZE) == NULL){
    return -1;
  }
  return 0;
}

/*
 Entirely  from the 247 textbook (cited in description above)
 */
void *mm_malloc(size_t size)
{
  size_t asize;      /* Adjusted block size */
  size_t extendsize; /* Amount to extend heap if no fit is found*/
  void *bp;

  /* Ignore spurious requests. */
  if (size <= 0) {
    return (NULL);
  }

  if (size <= DSIZE)
    asize = DSIZE + ALIGNMENT;
  else
    asize = DSIZE * ((size + ALIGNMENT + (DSIZE - 1)) / DSIZE);

  /* Search the free list for a fit. */
  if ((bp = find_fit(asize)) != NULL) {
    split(bp, asize);
    return (bp);
  }
  /* No fit found.  Get more memory and split the block. */
  extendsize = MAX(asize, CHUNKSIZE);
  if ((bp = extend_heap(extendsize / WSIZE)) == NULL) {
    return (NULL);
  }
  split(bp, asize);
  return (bp);
}

/*
 Frees/Deallocates a block of memory and adds the new "FREE" block to the free list.
 Call coalesce after block is added to see if any previous or next blocks are also free & can be combined
 Note: coalesce is where the block is actually added to the free list (insert call is in coalesce)
 */
void mm_free(void *bp){
    size_t size = GET_SIZE(HDRP(bp));
    deallocBit(bp, size);
    coalesce(bp);
  }

/*
 Returns a pointer to an allocated amount of space of at least "size" bytes
 */
  void *mm_realloc(void *bp, size_t size)
  {
    if(bp == NULL) {
      mm_malloc(size);
    }  else if(size == 0) {
      mm_free(bp);
    }  else if(bp != NULL) {
        size_t newsize = size + ALIGNMENT;
        //do not need to allocate more space
        if(newsize <= GET_SIZE(HDRP(bp))) { //compare requested size to current size
            return bp;
        //need to allocate more space (then copy data into new larger block)
        }  else {
              void *newblock = mm_malloc(newsize);
              if(newblock == NULL) {
                return (void *)-1;
              }
              split(newblock, newsize);
              memcpy(newblock, bp, newsize);
              mm_free(bp);
              return newblock;
      }
    }
    return NULL;
  }


/*
 Checks to see if freed block can be "combined" with any adjacent free blocks (checks if previous and next blocks are also
unallocated). If previous or next block are also unallocated, these blocks are combined with the pointer to the free block that is
passed in. Inserts new block into free list and removes any blocks that would now overlap. Returns pointer to freed space.
 */
void *coalesce(void *bp){

    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t prev_alloc = 0;
    if(GET_ALLOC(FTRP(PREV_BLKP(bp))) || PREV_BLKP(bp) == bp){
      prev_alloc = 1;
    }
    size_t size = GET_SIZE(HDRP(bp));

    /* Next block is only free */
    if(prev_alloc && next_alloc) {
      listInsert(bp); //cannot combine either prev or next block so just insert bp
      return(bp);
    }
    else if (prev_alloc && !next_alloc) {
      size += GET_SIZE( HDRP(NEXT_BLKP(bp)) );
      listRemove(NEXT_BLKP(bp)); //combining blocks, so bp -> next can be removed from the list
      deallocBit(bp, size); //set bp w/ new size to free and then insert into free list
      listInsert(bp); //insert bp with new updated (combined) size to free list
      return(bp);
    }
    /* Prev block is only free */
    else if (!prev_alloc && next_alloc) {
      size += GET_SIZE( HDRP(PREV_BLKP(bp))  );
      listRemove(PREV_BLKP(bp)); //need to remove this from the free list in order to add the new PREV_BLKP w/ the combined new size
      PUT(FTRP(bp), PACK(size, 0)); //unallocate from PREV header to bp footer (new block header/footer)
      PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
      listInsert(PREV_BLKP(bp)); //coalesced w/ previous block, so add previous block with updated size to free list
      return(PREV_BLKP(bp));
    }
    /* Both blocks are free */
    else  {
      size += GET_SIZE( HDRP(PREV_BLKP(bp))  ) + GET_SIZE( HDRP(NEXT_BLKP(bp))  );
      listRemove(PREV_BLKP(bp)); //need to remove previous and next blocks from free list so that I can add previous block w/ new combined size
      listRemove(NEXT_BLKP(bp));
      PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0)); //unallocate from header of PREV to footer of NEXT (new block header/footer)
      PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
      listInsert(PREV_BLKP(bp)); //coealesced with previous so insert previous block into free list updated with new size
      return(PREV_BLKP(bp));
    }
    return NULL; //if all of these cases fail, just return NULL
}

/*
 Requests "word" bytes to extend the heap by and aligns this with the nearest multiple of 8. Returns a pointer to the
 additional heap space created.
 */

 void *extend_heap(size_t words) {
  char *bp;
  size_t size;

  /* Allocate an even number of words to maintain alignment */
  size = (words % 2) ? (words+1) * WSIZE : words * WSIZE;
  if ((int)(bp = mem_sbrk(size)) == 0){
    return NULL;
  }
  /* Initialize free block header/footer and the epilogue header */
  deallocBit(bp, size);
  PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); /* new epilogue header */
  /* coalesce bp with next and previous blocks */
  return coalesce(bp);
}

/*
 Implementation of first fit search through free list
 Entirely taken from 247 textbook (cited in description above)
 */

void *find_fit(size_t asize) {
  void *bp;
 /* first fit search */
 for (bp = freeList; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) {
   if (!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp)))) {
     return bp;
   }
 }
 return NULL; /* no fit */
}

/*
 Split function is based highly off of the PLACE function given in the 247 textbook (cited above)...
 Needed to change so that list item was removed and coalesce was called where needed
 */
void split(void *bp, size_t asize)
 {
     size_t csize = GET_SIZE(HDRP(bp));
     int coalesceFlag = 0; //no need to coalesce unless dealloc is called (now this space is free)
     if ((csize - asize) >= (DSIZE + ALIGNMENT)) { //block can be split
         allocBit(bp, asize);
         deallocBit(NEXT_BLKP(bp), csize - asize); //NOTE: don't need to call listInsert here because NEXTBLK will be inserted w/ coalesce call!
         coalesceFlag = 1;
     }
     else { //block does not need to be split
       allocBit(bp, csize);
     }
     listRemove(bp); //remove bp from free list (it is now allocated again)
     if(coalesceFlag == 1) {
       bp = NEXT_BLKP(bp); //unallocated NEXT_BLKP(bp), so this needs to be coalesced
       coalesce(bp);
     }
 }
//adds specified block to free list after the head (FIFO implementation )
void listInsert(void *bp) {
    NEXTFREE(bp) = freeList;
    PREVFREE(freeList) = bp;
    PREVFREE(bp) =  NULL;
    freeList = bp;
  }
//Removes specified block from free list
void listRemove(void *bp){
  void* next = NEXTFREE(bp);
  void* prev = PREVFREE(bp);

  if (!PREVFREE(bp)) {
    freeList = NEXTFREE(bp);
  }else{
    NEXTFREE(PREVFREE(bp)) = next;
  }
  PREVFREE(NEXTFREE(bp)) = prev;
}

/* wrote a function for the (PUT, PACK) call (used
   in the 247 textbook) for header and footer where space is deallocated. Did this to make code look cleaner */
void deallocBit (void* ptr, size_t size) {
    PUT(HDRP(ptr), PACK(size, 0));
    PUT(FTRP(ptr), PACK(size ,0));
}

/* wrote a function for the (PUT, PACK) call (used
   in the 247 textbook) for header and footer where space is allocated (ore reallocated). Did this to make code look cleaner */
void allocBit (void *ptr, size_t size) {
    PUT(HDRP(ptr), PACK(size,1));
    PUT(FTRP(ptr), PACK(size, 1));
}
