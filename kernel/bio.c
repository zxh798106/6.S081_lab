// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

#define NBUC 13

// 哈希结构体
struct {
  struct spinlock lock;
  struct buf head;
} bhash[NBUC];

struct {
  struct spinlock lock;
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  //struct buf head;
} bcache;

void
binit(void)
{
  struct buf *b;

  initlock(&bcache.lock, "bcache");

  // Create linked list of buffers
  /*bcache.head.prev = &bcache.head;
  bcache.head.next = &bcache.head;*/
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    /*b->next = bcache.head.next;
    b->prev = &bcache.head;*/
    initsleeplock(&b->lock, "buffer");
    /*bcache.head.next->prev = b;
    bcache.head.next = b;*/
  }

  // 初始化bhash数组元素的锁和哈希链表
  for (int i = 0; i < NBUC; ++i) {
    initlock(&bhash[i].lock, "bhash");
    bhash[i].head.prev = &bhash[i].head;
    bhash[i].head.next = &bhash[i].head;
  }  
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;

  // 在哈希表中查找block
  int idx = blockno % NBUC;
  acquire(&bhash[idx].lock);
  for (b = bhash[idx].head.next; b != &bhash[idx].head; b = b->next) {
    if (b->dev == dev && b->blockno == blockno) {
      b->refcnt++;
      b->ticks = ticks;
      release(&bhash[idx].lock);
      acquiresleep(&b->lock);
      return b;
    }
  }

  struct buf *LRU_b = 0;
  acquire(&bcache.lock); // bcache.lock不能去掉，去掉了usertests过不去。
  // 若哈希表中没有目标block，寻找ticks最小的block
  for (b = bcache.buf; b < bcache.buf + NBUF; b++) {
    if (b->refcnt == 0) {
      if (!LRU_b) {
        LRU_b = b;
      }
      else if (b->ticks < LRU_b->ticks) {
        LRU_b = b;
      }
    }
  }
  
  if (LRU_b) {
    int LRU_idx = LRU_b->blockno % NBUC;
    if (LRU_idx != idx) {
      acquire(&bhash[LRU_idx].lock);
      // 将旧的block从原哈希表中移除
      if (LRU_b->prev)
        LRU_b->prev->next = LRU_b->next;
      if (LRU_b->next)
        LRU_b->next->prev = LRU_b->prev;
      // 将新的block插入到哈希表中
      LRU_b->prev = &bhash[idx].head;
      LRU_b->next = bhash[idx].head.next;
      bhash[idx].head.next->prev = LRU_b;
      bhash[idx].head.next = LRU_b;

      LRU_b->dev = dev;
      LRU_b->blockno = blockno;
      LRU_b->valid = 0;
      LRU_b->refcnt = 1;
      LRU_b->ticks = ticks;
      release(&bhash[LRU_idx].lock); // 考虑一下这两个idx的锁无序的话，是否会死锁。
    }
    else {
      LRU_b->dev = dev;
      LRU_b->blockno = blockno;
      LRU_b->valid = 0;
      LRU_b->refcnt = 1;
      LRU_b->ticks = ticks;
    }
    release(&bcache.lock);
    release(&bhash[idx].lock);
    acquiresleep(&LRU_b->lock);
    return LRU_b;
  }
  else 
    panic("bget: no buffers");

  /*acquire(&bcache.lock);

  // Is the block already cached?
  for(b = bcache.head.next; b != &bcache.head; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.lock);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  for(b = bcache.head.prev; b != &bcache.head; b = b->prev){
    if(b->refcnt == 0) {
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;
      release(&bcache.lock);
      acquiresleep(&b->lock);
      return b;
    }
  }
  panic("bget: no buffers");*/
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  int idx = b->blockno % NBUC;
  acquire(&bhash[idx].lock);
  //acquire(&bcache.lock);
  b->refcnt--;
  //b->ticks = ticks; // csdn代码注释了，我觉得不应该注释。结果谁注释了跑出来的。 
  release(&bhash[idx].lock);
  //release(&bcache.lock);

  /*acquire(&bcache.lock);
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = bcache.head.next;
    b->prev = &bcache.head;
    bcache.head.next->prev = b;
    bcache.head.next = b;
  }
  
  release(&bcache.lock);*/
}

void
bpin(struct buf *b) {
  int idx = b->blockno % NBUC;
  acquire(&bhash[idx].lock);
  b->refcnt++;
  release(&bhash[idx].lock);

  /*acquire(&bcache.lock);
  b->refcnt++;
  release(&bcache.lock);*/
}

void
bunpin(struct buf *b) {
  int idx = b->blockno % NBUC;
  acquire(&bhash[idx].lock);
  b->refcnt--;
  release(&bhash[idx].lock);
  
  /*acquire(&bcache.lock);
  b->refcnt--;
  release(&bcache.lock);*/
}


