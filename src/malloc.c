#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#define ALIGN4(s)         (((((s) - 1) >> 2) << 2) + 4)
#define BLOCK_DATA(b)     ((b) + 1)
#define BLOCK_HEADER(ptr) ((struct _block *)(ptr) - 1)

static int atexit_registered = 0;
static int num_mallocs       = 0;
static int num_frees         = 0;
static int num_reuses        = 0;
static int num_grows         = 0;
static int num_splits        = 0;
static int num_coalesces     = 0;
static int num_blocks        = 0;
static int num_requested     = 0;
static int max_heap          = 0;

struct _block 
{
   size_t  size;         /* Size of the allocated _block of memory in bytes     */
   struct _block *next;  /* Pointer to the next _block of allocated memory      */
   bool   free;          /* Is this _block free?                                */
   char   padding[3];    /* Padding                                             */
};


struct _block *heapList = NULL; /* Free list to track the _blocks available */
struct _block *nf = NULL;

/*
 *  \brief printStatistics
 *
 *  \param none
 *
 *  Prints the heap statistics upon process exit.  Registered
 *  via atexit()
 *
 *  \return none
 */
void printStatistics( void )
{
   // List of blocks
   // Traverse through list and increment num_blocks
   struct _block *block = heapList;
   while(block)
   {
      num_blocks++;
      block = block->next;
   }

  printf("\nheap management statistics\n");
  printf("mallocs:\t%d\n", num_mallocs );
  printf("frees:\t\t%d\n", num_frees );
  printf("reuses:\t\t%d\n", num_reuses );
  printf("grows:\t\t%d\n", num_grows );
  printf("splits:\t\t%d\n", num_splits );
  printf("coalesces:\t%d\n", num_coalesces );
  printf("blocks:\t\t%d\n", num_blocks );
  printf("requested:\t%d\n", num_requested );
  printf("max heap:\t%d\n", max_heap );
}



/*
 * \brief findFreeBlock
 *
 * \param last pointer to the linked list of free _blocks
 * \param size size of the _block needed in bytes 
 *
 * \return a _block that fits the request or NULL if no free _block matches
 *
 *
 */
struct _block *findFreeBlock(struct _block **last, size_t size) 
{
   struct _block *curr = heapList;

#if defined FIT && FIT == 0
   /* First fit */
   //
   // While we haven't run off the end of the linked list and
   // while the current node we point to isn't free or isn't big enough
   // then continue to iterate over the list.  This loop ends either
   // with curr pointing to NULL, meaning we've run to the end of the list
   // without finding a node or it ends pointing to a free node that has enough
   // space for the request.
   // 
   while (curr && !(curr->free && curr->size >= size)) 
   {
      *last = curr;
      curr  = curr->next;
   }
#endif

#if defined BEST && BEST == 0
   // While we haven't run off the end of the linked list
   // If current node is free and its size is big enough
   //    if tracked block is NULL, assign curr to tracked block
   //    else if curr size is less than current tracked block, assign curr to tracked block


   // To track block
   struct _block *smallest = NULL;

   while(curr)
   {
      if(curr->free && curr->size >= size)
      {
        if(smallest == NULL)
        {
            smallest = curr;
        }
        else if(curr->size < smallest->size)
        {
            smallest = curr;
        }
      }
      
      *last = curr;
      curr = curr->next;
   }

   return smallest;
#endif

#if defined WORST && WORST == 0

   // While we haven't run off the end of the linked list
   // If current node is free and its size is big enough
   //    if tracked block is NULL, assign curr to tracked block
   //    else if curr size is greater than current tracked block, assign curr to tracked block

   struct _block *biggest = NULL;

   while(curr)
   {
      if(curr->free && curr->size >= size)
      {
         if(biggest == NULL)
         {
            biggest = curr;
         }
         else if(curr->size > biggest->size)
         {
            biggest = curr;
         }
      }
      
      *last = curr;
      curr = curr->next;
   }

   return biggest;
#endif


#if defined NEXT && NEXT == 0

   // start from where last memory allocated 
   // If curr reaches NULL, go back to heapList


   struct _block *start = NULL;
   int flag = 0;

   if(nf != NULL)
   {
      curr = nf;
      start = nf;
   }
   else{
      curr = heapList;
      start = heapList;
   }

   while(curr)
   {
      if(curr->free && curr->size >= size)
      {
         break;
      }

      *last = curr;
      curr = curr->next;

      if(curr == NULL && flag == 0)
      {
         curr = heapList;
         flag++;
      }

      if(curr == start && flag == 1)
      {
         curr = NULL;
         break;
      }
   }

   if(curr != NULL)
   {
      if(curr->next)
      {
         nf = curr->next;
      }
      else
      {
         nf = heapList;
      }
   }
   else
   {
      struct _block *ptr = heapList;

      while(ptr && ptr->next)
      {
         ptr = ptr->next;
      }

      *last = ptr;
      nf = NULL;
   }
   
#endif

   return curr;
}

/*
 *
 * Given a requested size of memory, use sbrk() to dynamically 
 * increase the data segment of the calling process.  Updates
 * the free list with the newly allocated memory.
 *
 * \param last tail of the free _block list
 * \param size size in bytes to request from the OS
 *
 * \return returns the newly allocated _block of NULL if failed
 */
struct _block *growHeap(struct _block *last, size_t size) 
{
   /* Request more space from OS */
   struct _block *curr = (struct _block *)sbrk(0);
   struct _block *prev = (struct _block *)sbrk(sizeof(struct _block) + size);

   assert(curr == prev);

   /* OS allocation failed */
   if (curr == (struct _block *)-1) 
   {
      return NULL;
   }

   /* Update heapList if not set */
   if (heapList == NULL) 
   {
      heapList = curr;
   }

   /* Attach new _block to previous _block */
   if (last) 
   {
      last->next = curr;
   }

   /* Update _block metadata:
      Set the size of the new block and initialize the new block to "free".
      Set its next pointer to NULL since it's now the tail of the linked list.
   */
   curr->size = size;
   curr->next = NULL;
   curr->free = false;

   max_heap += size + sizeof(struct _block);
   num_grows++;
   
   return curr;
}

/*
 *
 * finds a free _block of heap memory for the calling process.
 * if there is no free _block that satisfies the request then grows the 
 * heap and returns a new _block
 *
 */
void *malloc(size_t size) 
{

   if( atexit_registered == 0 )
   {
      atexit_registered = 1;
      atexit( printStatistics );
   }

   /* Align to multiple of 4 */
   size = ALIGN4(size);

   /* Handle 0 size */
   if (size == 0) 
   {
      return NULL;
   }

   /* Look for free _block.  If a free block isn't found then we need to grow our heap. */
   struct _block *last = heapList;
   struct _block *next = findFreeBlock(&last, size);

   /* If the block found by findFreeBlock is larger than we need then:
      If the leftover space in the new block is greater than the sizeof(_block)+4 then
      split the block.
      If the leftover space in the new block is less than the sizeof(_block)+4 then
      don't split the block.
   */

   if(next && (next->size - size) > (sizeof(struct _block) + 4))
   {
      int oldSize = next->size;
      struct _block *oldNext = next->next;

      next->next = (struct _block*)(uint8_t*)((char*)next + size + sizeof(struct _block));
      next->size = size;

      next->next->next = oldNext;
      next->next->size = oldSize - size - sizeof(struct _block);
      next->next->free = true;
   
      num_splits++;
      
   }
   

   /* Could not find free _block, so grow heap */
   if (next == NULL) 
   {
      next = growHeap(last, size);
   }
   else
   {
      num_reuses++;
   }

   /* Could not find free _block or grow heap, so just return NULL */
   if (next == NULL) 
   {
      return NULL;
   }
   
   /* Mark _block as in use */
   next->free = false;

   num_requested += size;
   num_mallocs++;

   /* Return data address associated with _block to the user */
   return BLOCK_DATA(next);
}

/*
 * frees the memory _block pointed to by pointer. if the _block is adjacent
 * to another _block then coalesces (combines) them
 *
 */
void free(void *ptr) 
{
   if (ptr == NULL) 
   {
      return;
   }

   /* Make _block as free */
   struct _block *curr = BLOCK_HEADER(ptr);
   assert(curr->free == 0);
   curr->free = true;

   /* Coalesce free _blocks.  If the next block or previous block 
      are free then combine them with this block being freed.
   */

   curr = heapList;

   while(curr)
   {
      if(curr && curr->next && curr->next->free && curr->free)
      {
         curr->size += curr->next->size + sizeof(struct _block);
         curr->next = curr->next->next;
         num_coalesces++;
         // don't advance — check curr again for further merging
      }
      else
      {
         curr = curr->next;
      }
   }

   num_frees++;
}

void *calloc( size_t nmemb, size_t size )
{
   size_t sizeNeeded = nmemb * size;
   void *ptr = malloc(sizeNeeded);

   if(ptr == NULL)
   {
      return NULL;
   }

   memset(ptr, 0, sizeNeeded);

   return ptr;
}


void *realloc( void *ptr, size_t size )
{
   if(ptr == NULL)
   {
      return malloc(size);
   }
   else if(size == 0)
   {
      free(ptr);
      return NULL;
   }
   else
   {
      // allocate new
      void *newPtr = malloc(size);
      if(newPtr == NULL)
      {
         return NULL;
      }
      struct _block *oldBlock = BLOCK_HEADER(ptr);
      size_t copySize = oldBlock->size < size ? oldBlock->size : size;
      memcpy(newPtr, ptr, copySize);
      free(ptr);
      return newPtr;
   }
   
   return NULL;
}
