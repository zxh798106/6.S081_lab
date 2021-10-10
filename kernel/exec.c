#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"
#include "elf.h"

static int loadseg(pde_t *pgdir, uint64 addr, struct inode *ip, uint offset, uint sz);

int
exec(char *path, char **argv)
{
  if (PRINTF_SWTCH) printf("exec can reach\n");
  char *s, *last;
  int i, off;
  uint64 argc, sz = 0, sp, ustack[MAXARG+1], stackbase;
  struct elfhdr elf;
  struct inode *ip;
  struct proghdr ph;
  pagetable_t pagetable = 0, oldpagetable;
  //pagetable_t k_pagetable = 0; // lab:
  struct proc *p = myproc();

  begin_op();

  if((ip = namei(path)) == 0){
    end_op();
    return -1;
  }
  ilock(ip);

  // Check ELF header
  if(readi(ip, 0, (uint64)&elf, 0, sizeof(elf)) != sizeof(elf))
    goto bad;
  if(elf.magic != ELF_MAGIC)
    goto bad;
  
  
  
  //proc_free_k_pagetable(p->k_pagetable, oldsz); // lab:释放内核栈、内核页表
  // lab:创建内核页表并初始化
  //k_pagetable = kvminit_lab();
  

  if((pagetable = proc_pagetable(p)) == 0)
    goto bad;
    
  
  
  // Load program into memory.
  for(i=0, off=elf.phoff; i<elf.phnum; i++, off+=sizeof(ph)){
    if(readi(ip, 0, (uint64)&ph, off, sizeof(ph)) != sizeof(ph))
      goto bad;
    if(ph.type != ELF_PROG_LOAD)
      continue;
    if(ph.memsz < ph.filesz)
      goto bad;
    if(ph.vaddr + ph.memsz < ph.vaddr)
      goto bad;
    if (PRINTF_SWTCH) printf("exec can reach1\n");
    uint64 sz1;
    if((sz1 = uvmalloc(pagetable, sz, ph.vaddr + ph.memsz)) == 0)
      goto bad;
    // lab:防止用户地址超出
    if (sz1 >= USER_MAXVA) {
    	printf("va is higher than USER_MAXVA\n");
    	goto bad;
    }
      
    
    sz = sz1;
    if (PRINTF_SWTCH) printf("exec can reach2\n");
    if(ph.vaddr % PGSIZE != 0)
      goto bad;
    if(loadseg(pagetable, ph.vaddr, ip, ph.off, ph.filesz) < 0)
      goto bad;
    if (PRINTF_SWTCH) printf("exec can reach3\n");
  }
  iunlockput(ip);
  end_op();
  ip = 0;
	
  p = myproc();
  uint64 oldsz = p->sz;

  // Allocate two pages at the next page boundary.
  // Use the second as the user stack.
  sz = PGROUNDUP(sz);
  uint64 sz1;
  if((sz1 = uvmalloc(pagetable, sz, sz + 2*PGSIZE)) == 0)
    goto bad;
  sz = sz1;
  uvmclear(pagetable, sz-2*PGSIZE);
  sp = sz;
  stackbase = sp - PGSIZE;

  // Push argument strings, prepare rest of stack in ustack.
  for(argc = 0; argv[argc]; argc++) {
    if(argc >= MAXARG)
      goto bad;
    sp -= strlen(argv[argc]) + 1;
    sp -= sp % 16; // riscv sp must be 16-byte aligned
    if(sp < stackbase)
      goto bad;
    if(copyout(pagetable, sp, argv[argc], strlen(argv[argc]) + 1) < 0)
      goto bad;
    ustack[argc] = sp;
  }
  ustack[argc] = 0;

  // push the array of argv[] pointers.
  sp -= (argc+1) * sizeof(uint64);
  sp -= sp % 16;
  if(sp < stackbase)
    goto bad;
  if(copyout(pagetable, sp, (char *)ustack, (argc+1)*sizeof(uint64)) < 0)
    goto bad;

  // arguments to user main(argc, argv)
  // argc is returned via the system call return
  // value, which goes in a0.
  p->trapframe->a1 = sp;

  // Save program name for debugging.
  for(last=s=path; *s; s++)
    if(*s == '/')
      last = s+1;
  safestrcpy(p->name, last, sizeof(p->name));
  
  
  
  // Commit to the user image.
  // lab:内核页表的更替
  //oldk_pagetable = p->k_pagetable;
  //p->k_pagetable = k_pagetable;
  
  oldpagetable = p->pagetable;
  p->pagetable = pagetable;
  p->sz = sz;
  p->trapframe->epc = elf.entry;  // initial program counter = main
  p->trapframe->sp = sp; // initial stack pointer
  //proc_free_k_pagetable(oldk_pagetable, oldsz); // lab:释放内核栈、内核页表
  proc_freepagetable(oldpagetable, oldsz);
  
  uvmunmap(p->k_pagetable, 0, PGROUNDUP(oldsz)/PGSIZE, 0); // lab:释放进程旧内核页表映射
  pagetable_copy(pagetable, p->k_pagetable, 0, p->sz); //lab:将新用户页表拷贝至内核页表
  
  // lab:k_pagetable建立内核栈的映射
  /*char *pa = kalloc();
  if(pa == 0)
    panic("kalloc");
  uint64 va = KSTACK((int) (0));
  kvmmap_lab(p->k_pagetable, va, (uint64)pa, PGSIZE, PTE_R | PTE_W);
  p->kstack = va;
  // lab:建立内核页表TRAPFRAME映射
  if(mappages(p->k_pagetable, TRAPFRAME, PGSIZE,
              (uint64)(p->trapframe), PTE_R | PTE_W) < 0){
    printf("kernel TRAPFRAME wrong\n");
    freeproc(p);
    release(&p->lock);
    return 0;
  }*/
   
  // 添加vmprint
  if (p->pid == 1)
  	vmprint(p->pagetable);

  return argc; // this ends up in a0, the first argument to main(argc, argv)

 bad:
  printf("exec bad\n");
  /*if (k_pagetable)
  	proc_free_k_pagetable(k_pagetable, sz); // lab:释放内核栈、内核页表*/
  if(pagetable)
    proc_freepagetable(pagetable, sz);
  if(ip){
    iunlockput(ip);
    end_op();
  }
  return -1;
}

// Load a program segment into pagetable at virtual address va.
// va must be page-aligned
// and the pages from va to va+sz must already be mapped.
// Returns 0 on success, -1 on failure.
static int
loadseg(pagetable_t pagetable, uint64 va, struct inode *ip, uint offset, uint sz)
{
  uint i, n;
  uint64 pa;

  if((va % PGSIZE) != 0)
    panic("loadseg: va must be page aligned");

  for(i = 0; i < sz; i += PGSIZE){
    pa = walkaddr(pagetable, va + i);
    if(pa == 0)
      panic("loadseg: address should exist");
    if(sz - i < PGSIZE)
      n = sz - i;
    else
      n = PGSIZE;
    if(readi(ip, 0, (uint64)pa, offset+i, n) != n)
      return -1;
  }
  
  return 0;
}
