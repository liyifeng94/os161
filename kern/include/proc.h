/*
 * Copyright (c) 2013
 *	The President and Fellows of Harvard College.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE UNIVERSITY OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _PROC_H_
#define _PROC_H_

/*
 * Definition of a process.
 *
 * Note: curproc is defined by <current.h>.
 */

#include <spinlock.h>
#include <thread.h> /* required for struct threadarray */
#include <kern/limits.h>

struct addrspace;
struct vnode;
#ifdef UW
struct semaphore;
#endif // UW

/*
 * Process wait channel
 */
struct procWchan
{
    struct wchan *pwchan;
    volatile int exitcode;
    volatile int exitstate;

    //status of the proc,
    //-1:does not exist, 0:still running, 1:finished and exited
    volatile char procStatus;

    volatile bool needProc;
};


struct procWchan *procWchan_create(void);

void procWchan_destroy(struct procWchan *pw);

int proc_getAndCreateNewPid(pid_t *ret);

void proc_freePid(pid_t pid);

void proc_exitCodeNotNeeded(pid_t pid);

/*
 * Process structure.
 */
struct proc 
{
    char *p_name;			/* Name of this process */
    pid_t pid;
    
    struct spinlock p_lock;		/* Lock for this structure */
    struct threadarray p_threads;	/* Threads in this process */
    
    /* VM */
    struct addrspace *p_addrspace;	/* virtual address space */
    
    /* VFS */
    struct vnode *p_cwd;		/* current working directory */
    
#ifdef UW
    /* a vnode to refer to the console device */
    /* this is a quick-and-dirty way to get console writes working */
    /* you will probably need to change this when implementing file-related
       system calls, since each process will need to keep track of all files
       it has opened, not just the console. */
    struct vnode *console;                /* a vnode for the console device */
#endif

    struct array *childPids;

    struct trapframe *initTf;

    /* add more material here as needed */
};

/* This is the process structure for the kernel and for kernel-only threads. */
extern struct proc *kproc;

/* Semaphore used to signal when there are no more processes */
#ifdef UW
extern struct semaphore *no_proc_sem;
#endif // UW

/* Call once during system startup to allocate data structures. */
void proc_bootstrap(void);

/* Create a fresh process for use by runprogram(). */
struct proc *proc_create_runprogram(const char *name);

/* Destroy a process. */
void proc_destroy(struct proc *proc);

/* Attach a thread to a process. Must not already have a process. */
int proc_addthread(struct proc *proc, struct thread *t);

/* Detach a thread from its process. */
void proc_remthread(struct thread *t);

/* Fetch the address space of the current process. */
struct addrspace *curproc_getas(void);

/* Change the address space of the current process, and return the old one. */
struct addrspace *curproc_setas(struct addrspace *);

struct addrspace *proc_setas(struct proc *proc,struct addrspace *newas);

int copyInitTrapframe(struct proc *proc, struct trapframe* tf);

//add the child pid to parent
void proc_addChildPid(struct proc *proc, pid_t cpid);

//wait channel for pid, return the exitcode of the proc
int proc_waitOn(pid_t pid);

//wake any proc waitint on pid, and set the exitcode
void proc_exitOn(pid_t pid,int exitcode,int exitstate);

void proc_changeWaitStatus(pid_t pid, char status);

char proc_getWaitStatus(pid_t pid);

bool proc_isChildProc(struct proc *proc, pid_t childPid);

bool proc_exists(pid_t pid);

#endif /* _PROC_H_ */
