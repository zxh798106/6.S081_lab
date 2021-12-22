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
  
  uint64 sp = p->trapframe->sp;
  uint64 sp_base = PGROUNDUP(sp); // 获取用户栈的最高地址
  
  // 超出最高地址
  if (p->sz + n >= MAXVA) {
  	return addr;
  }
  else if (n >= 0)
  	p->sz += n; 
  // 若n负数，使得缩减后的空间低于栈的基地址，则返回错误。
  else if (n < 0 && p->sz - sp_base < -n) {
  	printf("sbrk n negtive than sp_base, p->sz = %p, sp_base = %p, diff = %d, n = %d\n", p->sz, sp_base, p->sz - sp_base, n);
  	return -1;
  }
  // n<0 立即释放内存
  else if (growproc(n) < 0) 
  	return -1;
  
  //if(growproc(n) < 0)
  //  return -1;
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
