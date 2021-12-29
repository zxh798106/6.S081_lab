// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.
                   
int page_ref[PAGE_NUM] = {0};
struct spinlock page_ref_lock;

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  initlock(&page_ref_lock, "page_ref");
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  int num = 0;
  int test = 0;
  p = (char*)PGROUNDUP((uint64)pa_start);
  printf("pa_start : %p\n", (uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE) {
  	kfree(p);
  	++num;
  	test += page_ref[(uint64)(p - KERNBASE) / PGSIZE];
  }
  printf("page num : %d, test : %d\n", num, test);
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
	/*if ((uint64)pa >= KERNBASE) {
		int idx = (uint64)((uint64)pa - KERNBASE) / PGSIZE;
		if (page_ref[idx] > 1) {
			//printf("kfree: page_ref[%d] = %d\n", idx, page_ref[idx]);
			return;
		}
		else if (page_ref[(uint64)((uint64)pa - KERNBASE) / PGSIZE] < 0)
			panic("kfree: page_ref < 0");
	}*/
	
	// 计数为1时才释放物理内存
	if ((uint64)pa >= KERNBASE && page_ref[(uint64)(pa - KERNBASE) / PGSIZE] > 1) 
		return;
		
	
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r)
    kmem.freelist = r->next;
  release(&kmem.lock);

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}
