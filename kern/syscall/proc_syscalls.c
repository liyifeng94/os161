#include <types.h>
#include <kern/errno.h>
#include <kern/unistd.h>
#include <kern/fcntl.h>
#include <kern/wait.h>
#include <mips/trapframe.h>
#include <lib.h>
#include <syscall.h>
#include <current.h>
#include <proc.h>
#include <vfs.h>
#include <synch.h>
#include <thread.h>
#include <addrspace.h>
#include <copyinout.h>


void sys__exit(int exitcode) {

  struct addrspace *as;
  struct proc *p = curproc;

  DEBUG(DB_SYSCALL,"Syscall: _exit(%d)\n",exitcode);

  KASSERT(curproc->p_addrspace != NULL);

  proc_exitOn(p->pid,exitcode,__WEXITED);

  as_deactivate();
  /*
   * clear p_addrspace before calling as_destroy. Otherwise if
   * as_destroy sleeps (which is quite possible) when we
   * come back we'll be calling as_activate on a
   * half-destroyed address space. This tends to be
   * messily fatal.
   */
  as = curproc_setas(NULL);
  as_destroy(as);

  /* detach this thread from its process */
  /* note: curproc cannot be used after this call */
  proc_remthread(curthread);

  /* if this is the last user process in the system, proc_destroy()
     will wake up the kernel menu thread */
  proc_destroy(p);

  thread_exit();
  /* thread_exit() does not return, so we should never get here */
  panic("return from thread_exit in sys_exit\n");
}


/* handler for getpid() system call                */
int
sys_getpid(pid_t *retval)
{
    struct proc *p = curproc;
    *retval = p->pid;
    return(0);
}

/* handler for waitpid() system call                */

int
sys_waitpid(pid_t pid,
	    userptr_t status,
	    int options,
	    pid_t *retval)
{
  int exitstatus;
  int result;


  if (options != 0) 
  {
    *retval = -1;
    return(EINVAL);
  }

  if(!proc_isChildProc(curproc,pid))
  {
      *retval = -1;
      return(ECHILD);
  }
  if(!proc_exists(pid))
  {
      *retval = -1;
      return(ESRCH);
  }
  
  exitstatus = proc_waitOn(pid);
  result = copyout((void *)&exitstatus,status,sizeof(int));

  if (result) 
  {
      *retval = -1;
      return(result);
  }
  *retval = pid;
  return(0);
}


void init_forkedProcess(void *data1, unsigned long data2)
{
    struct trapframe *tf = (struct trapframe *)data1;
    (void) data2;
    enter_forked_process(tf);
}

int sys_fork(struct trapframe *tf,pid_t *retval)
{
    int error = 0;
    struct addrspace *as = NULL;

    //create proc structure
    struct proc *childProc = proc_create_runprogram(curproc->p_name);
    if(childProc == NULL)
    {
        return ENOMEM;
    }

    //create and assign pid
    pid_t cpid = 0;
    error = proc_getAndCreateNewPid(&cpid);
    if (error)
    {
        proc_destroy(childProc);
        return ENPROC;
    }
    childProc->pid = cpid;
    proc_addChildPid(curproc,cpid);

    //create and copy child trapframe
    error = copyInitTrapframe(childProc,tf);
    if(error)
    {
        proc_destroy(childProc);
        return error;
    }

    //copy address space
    error = as_copy(curproc->p_addrspace,&(childProc->p_addrspace));
    if(error)
    {
        proc_destroy(childProc);
        return error;
    }
    as_activate();
    

    //create new thread for the process
    error = thread_fork(curproc->p_name,childProc,&init_forkedProcess,(void *)childProc->initTf,1);
    if(error)
    {
        as_deactivate();
        as = proc_setas(childProc,NULL);
        as_destroy(as);
        proc_destroy(childProc);
        return error;
    }

    //assign pid as the return value of the folk call
    *retval = cpid;

    return 0;
}

int sys_execv(const char *progname, char **args, int *retval)
{
    //kprintf("SYS_execv\n");
    struct addrspace *as;
    struct vnode *v;
    vaddr_t entrypoint, stackptr;
    int result;

    if(progname == NULL)
    {
        *retval = -1;
        return EFAULT;
    }

    //copy in progname to pname
    size_t pnameLen = strlen(progname);
    size_t pnameAllocSize = (pnameLen + 1) * sizeof(char);

    char *pname =  (char *)kmalloc(pnameAllocSize);
    if(pname == NULL)
    {
        *retval = -1;
        return ENOMEM;
    }

    result = copyin((const_userptr_t)progname, (void *)pname, pnameAllocSize);
    if(result)
    {
        kfree(pname);
        *retval = -1;
        return result;
    }

    //count args
    int argc = 0;
    while(args[argc] != NULL)
    {
        ++argc;
    }

    //copy in args to argv

    char **argv = (char **)kmalloc((argc + 1) * sizeof(char *));
    if(argv == NULL)
    {
        kfree(pname);
        *retval = -1;
        return ENOMEM;
    }

    for(int i = 0; i < argc; ++i)
    {
        size_t len = strlen(args[i]) + 1;
        argv[i] = (char *)kmalloc(len * sizeof(char));
        if(argv[i] == NULL)
        {
            kfree(pname);
            for(int j = 0; j<i; ++j)
            {
                kfree(argv[j]);
            }
            kfree(argv);
            *retval = -1;
            return ENOMEM;
        }
        result = copyin((const_userptr_t)args[i],argv[i],len * sizeof(char));
        if(result)
        {
            kfree(pname);
            for(int j = 0; j<=i; ++j)
            {
                kfree(argv[j]);
            }
            kfree(argv);
            *retval = -1;
            return result;
        }        
    }
    argv[argc] = NULL;
    
    //open file
    result = vfs_open(pname, O_RDONLY, 0, &v);
    if(result)
    {
        kfree(pname);

        for(int i = 0; i < argc; ++i)
        {
            kfree(argv[i]);
        }
        kfree(argv);
        *retval = -1;
        return result;
    }

    //create address space
    as = as_create();
    if(as == NULL)
    {
        kfree(pname);
        for(int i = 0; i < argc; ++i)
        {
            kfree(argv[i]);
        }
        kfree(argv);
        vfs_close(v);
        *retval = -1;
        return ENOMEM;
    }

    struct addrspace *oldas = curproc_setas(as);

    //load elf
    result = load_elf(v,&entrypoint);
    if(result)
    {
        kfree(pname);
        for(int i = 0; i < argc; ++i)
        {
            kfree(argv[i]);
        }
        kfree(argv);
        as_deactivate();
        as = curproc_setas(oldas);
        as_destroy(as);
        vfs_close(v);
        *retval = -1;
        return result;
    }

    
    vfs_close(v);
    kfree(pname);


    //define user stack
    result = as_define_stack(as,&stackptr);
    if(result)
    {
        for(int i = 0; i < argc; ++i)
        {
            kfree(argv[i]);
        }
        kfree(argv);
        as_deactivate();
        as = curproc_setas(oldas);
        as_destroy(as);
        *retval = -1;
        return result;
    }

        
    /*------copy args to user stack-----------*/

    vaddr_t *argPtrs = (vaddr_t *)kmalloc((argc + 1) * sizeof(vaddr_t));
    if(argPtrs == NULL)
    {
        for(int i = 0; i < argc; ++i)
        {
            kfree(argv[i]);
        }
        kfree(argv);
        as_deactivate();
        as = curproc_setas(oldas);
        as_destroy(as);
        *retval = -1;
        return ENOMEM;
    }

    for(int i = argc-1; i >= 0; --i)
    {
        //arg length with null
        size_t curArgLen = strlen(argv[i]) + 1;
        
        size_t argLen = ROUNDUP(curArgLen,4);
            
        stackptr -= (argLen * sizeof(char));
        
        //kprintf("copying arg: %s to addr: %p\n", temp, (void *)stackptr);

        //copy to stack
        result = copyout((void *) argv[i], (userptr_t)stackptr, curArgLen);
        if(result)
        {
            kfree(argPtrs);
            for(int i = 0; i < argc; ++i)
            {
                kfree(argv[i]);
            }
            kfree(argv);
            as_deactivate();
            as = curproc_setas(oldas);
            as_destroy(as);
            *retval = -1;
            return result;
        }
        
        argPtrs[i] = stackptr;        
    }    
        
    argPtrs[argc] = (vaddr_t)NULL;
    
    //copy arg pointers
    for(int i = argc; i >= 0; --i)
    {
        stackptr -= sizeof(vaddr_t);
        result = copyout((void *) &argPtrs[i], ((userptr_t)stackptr),sizeof(vaddr_t));
        if(result)
        {
            kfree(argPtrs);
            for(int i = 0; i < argc; ++i)
            {
                kfree(argv[i]);
            }
            kfree(argv);
            as_deactivate();
            as = curproc_setas(oldas);
            as_destroy(as);
            *retval = -1;
            return result;
        }
    }
    
    
    kfree(argPtrs);



    
    vaddr_t baseAddress = USERSTACK;

    vaddr_t argvPtr = stackptr;

    vaddr_t offset = ROUNDUP(USERSTACK - stackptr,8);

    stackptr = baseAddress - offset;

/*
    for(vaddr_t i = baseAddress; i >= stackptr; --i)
    {
        char *temp;
        temp = (char *)i;
        //kprintf("%p: %c\n",(void *)i,*temp);

        kprintf("%p: %x\n", (void *)i, *temp & 0xff);
    }
*/
    

    /*-done-copy args to user stack-----------*/

    for(int i = 0; i < argc; ++i)
    {
        kfree(argv[i]);
    }
    kfree(argv);
    
    as_deactivate();
    as_destroy(oldas);
    as_activate();
    
    *retval = 0;
    //enter new process
    enter_new_process(argc,(userptr_t)argvPtr,
                      stackptr, entrypoint);

    
    panic("enter_new_process returned\n");
    return EINVAL;
}
