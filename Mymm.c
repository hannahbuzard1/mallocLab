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

/* single word (4) or double word (8) alignment */
/* we will use 8. Don't change this */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))
#define FREESIZE (ALIGN(sizeof(freeStruct)))

typedef struct freeStruct {
        void *next;
        void *prev;
        size_t size;
} free_Struct;


/*
 * mm_init - initialize your  malloc package.
 */
int mm_init(void)
{
  freeStruct *freeptr;
  freeptr= mem_sbrk(mem_heap_lo + FREESIZE);
  if (freeptr == NULL) {  /*allocate space */
      return -1;
    }
  freeptr->size = 0; //should this be 0?
  freeptr->next = freeptr;
  freeList->prev = freeptr;
  return 0;
}


/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    size_t *findfit;
    int newsize = ALIGN(size + FREESIZE); //rounds up to nearest multiple of 8
    if (newsize <= 0) { //just in case this edge case occurs, no need to allocate any space
      return NULL;
    }
    findfit = firstfit(newsize);
    if (findfit == NULL) { //did not find a fit in freeList
      findfit = mem_sbrk(newsize); //extend the heap for the newsize (gets pointer to newly allocated space)
      return (void *)((char *)findfit + FREESIZE);
    } else {  //location was found using findfit
      //try to split block here ex: if( newsize < *findfit) but not sure how to do this...
      return (void *)((char *)findfit + FREESIZE);
    }
   return NULL; // if something went wrong (return NULL)
}

//find first spot in freeList that newsize (from malloc) can fit
void* firstfit (size_t requiredsize) {
    freeStruct *freeElem = mem_heap_lo() + FREESIZE;
    //start at first element in freeList and iterate up
    for (freeElem = freeElem -> next; freeElem != NULL; freeElem = freeElem -> next) { //iterate through freeList to find first fit (should NULL say mem_heap_hi() instead?)
        if(freeElem -> size >= requiredsize) { //spot found that has enough room
            freeElem -> next -> prev = freeElem -> prev; //remove from freeList (next element no longer points to this spot as its previous)
            freeElem -> prev -> next = freeElem -> next; //remove from freeList (prev element no longer points to this spot as its next)
            return freeElem ; //return this spot as found location (do I need to change this to void*?)
        }
    }
    return NULL; //return NULL if no available spot is found
}
/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
  //add freed block to free list after head ()
  /* freeStruct firstFreeBlock = mem_heap_lo() - FREESIZE;
  freeStruct *newblock = (char *)ptr - FREESIZE;
  newblock-> size = (size_t)newblock; //don't know what to put for block size here
  newblock->next = firstFreeBlock->next;
  newblock->prev = firstFreeBlock;
  firstFreeBlock->next = newblock;
  firstFreeBlock->prev = NULL; */
   freeStruct *header = (char *)ptr - FREESIZE;
   freeStruct *free_list_head = mem_heap_lo();
   // add freed block to free list after head
   header->size = *(size_t *)header;
   // add freed block to free list after head
   header->next = free_list_head->next;
   header->prior = free_list_head;
   free_list_head->next = free_list_head->next->prior = header;
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
  //case 1: ptr = NULL

  //case 2: ptr = 0

  //case 3: ptr is not NULL
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;e

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
