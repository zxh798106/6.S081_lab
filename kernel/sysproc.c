#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64
sys_exit(void)
{
  int n;
  if(argint(0, &n) < 0)
    return -1;
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  if(argaddr(0, &p) < 0)
    return -1;
  return wait(p);
}

uint64
sys_sbrk(void)
{
  int addr;
  int n;
  struct proc *p = myproc();

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  
  // lab:防止用户地址超出，和内核IO最低地址PLIC重叠
  if (addr + n >= USER_MAXVA) {
  	printf("va is higher than USER_MAXVA\n");
  	return -1;
  }
  
  if(growproc(n) < 0)
    return -1;
  
  // lab:将改变的用户页表拷贝至内核页表
  // lab:增加内存新增部分的页表以及建立映射
  if (n > 0) {
  	pagetable_copy(p->pagetable, p->k_pagetable, addr, p->sz);
  }
  // lab:内存缩减，要解除缩减部分的映射
  else if (n < 0) {
    int oldsz = addr;
    int newsz = p->sz;
  	int npages = (PGROUNDUP(oldsz) - PGROUNDUP(newsz)) / PGSIZE;
    uvmunmap(p->k_pagetable, PGROUNDUP(newsz), npages, 0); // lab:解除内核页表pa va映射，但不释放对应pa。
  }
  
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}
