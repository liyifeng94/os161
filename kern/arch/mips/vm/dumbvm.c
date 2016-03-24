/*
 * Copyright (c) 2000, 2001, 2002, 2003, 2004, 2005, 2008, 2009
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

#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <spl.h>
#include <spinlock.h>
#include <proc.h>
#include <current.h>
#include <mips/tlb.h>
#include <addrspace.h>
#include <vm.h>
#include <coremap.h>

/*
 * Dumb MIPS-only "VM system" that is intended to only be just barely
 * enough to struggle off the ground. You should replace all of this
 * code while doing the VM assignment. In fact, starting in that
 * assignment, this file is not included in your kernel!
 */

/* under dumbvm, always have 48k of user stack */
#define DUMBVM_STACKPAGES    12

/*
 * Wrap rma_stealmem in a spinlock.
 */
static struct spinlock stealmem_lock = SPINLOCK_INITIALIZER;

static struct coremap *coremap;

static volatile bool coremapReady = 0;

struct coremap *init_coremap()
{
    paddr_t startAddr = 0;
    paddr_t endAddr = 0;

    ram_getsize(&startAddr,&endAddr);

    struct coremap *cmap;

    cmap = kmalloc(sizeof(struct coremap));
    cmap->coremapSize = (endAddr - startAddr) / PAGE_SIZE;

    spinlock_init(&cmap->coreLock);
    
    cmap->entries = kmalloc(sizeof(struct coremap_entry) * cmap->coremapSize);

    for(unsigned long i = 0; i < cmap->coremapSize; ++i)
    {
        cmap->entries[i].paddr = startAddr + (i * PAGE_SIZE);
        cmap->entries[i].owner = 0;
        cmap->entries[i].inUse = 0;
    }

    paddr_t beginAddr = 0;
    
    ram_getsize(&beginAddr,&endAddr);

    for (unsigned long i = 0; cmap->entries[i].paddr < beginAddr; ++i)
    {
        cmap->entries[i].inUse = 1;
    }

    return cmap;
                   
}

void
vm_bootstrap(void)
{
    coremap = init_coremap();
    useCoremap();
    coremapReady = 1;
}

static
paddr_t
getppages(unsigned long npages)
{
	paddr_t addr = 0;

        if (!coremapReady)
        {
            spinlock_acquire(&stealmem_lock);

            addr = ram_stealmem(npages);
	
            spinlock_release(&stealmem_lock);
            return addr;
        }

        spinlock_acquire(&coremap->coreLock);

        //kprintf("npages: %lu\n", npages);

        unsigned int blockCount = 0;

        for(unsigned long i = 0; i<coremap->coremapSize; ++i)
        {
            if(!coremap->entries[i].inUse)
            {
                for (unsigned long j = i; j<coremap->coremapSize; ++j)
                {
                    if(!coremap->entries[j].inUse)
                    {
                        ++blockCount;
                    }
                    
                    if(blockCount == npages)
                    {
                        addr = coremap->entries[i].paddr;
                        for (unsigned long k = i; k<=j; ++k)
                        {
                            coremap->entries[k].owner = addr;
                            coremap->entries[k].inUse = 1;
                            //kprintf("Using: %lu\n", k);
                            //kprintf("k: %lu, paddr: %p, owner: %p \n",i,(void *) coremap->entries[k].paddr, (void *) coremap->entries[k].owner);
                        }

                        //kprintf("addr: %p\n", (void *)addr);
                        spinlock_release(&coremap->coreLock);
                        return addr;
                    }

                    if(coremap->entries[j].inUse)
                    {
                        break;
                    }
                }
                blockCount = 0;
            }
        }

        //error return 0;

        spinlock_release(&coremap->coreLock);
        return 0;
}

static void releaseppages(paddr_t paddr)
{
    spinlock_acquire(&coremap->coreLock);
    //kprintf("free: %p\n", (void *)paddr);
    for (unsigned long i = 0; i<coremap->coremapSize; ++i)
    {
        //kprintf("i: %lu, paddr: %p, owner: %p \n",i,(void *) coremap->entries[i].paddr, (void *) coremap->entries[i].owner);
        if(coremap->entries[i].paddr == paddr)
        {
            if(coremap->entries[i].owner == paddr)
            {
                unsigned long j = i;
                while (coremap->entries[j].owner == paddr)
                {
                    //kprintf("j: %lu, paddr: %p, owner: %p \n",j,(void *) coremap->entries[j].paddr, (void *) coremap->entries[j].owner);
                    coremap->entries[j].owner = 0;
                    coremap->entries[j].inUse = 0;
                    //kprintf("freeing: %lu\n", j);
                    ++j;
                }
                spinlock_release(&coremap->coreLock);
                return;
            }
            else
            {
                //error
                break;
            }
        }
    }
    spinlock_release(&coremap->coreLock);
}

/* Allocate/free some kernel-space virtual pages */
vaddr_t 
alloc_kpages(int npages)
{
	paddr_t pa;
	pa = getppages(npages);
	if (pa==0) {
		return 0;
	}
	return PADDR_TO_KVADDR(pa);
}

void 
free_kpages(vaddr_t addr)
{
    //kprintf("KVADDR: %p\n", (void *)addr);
    addr = KVADDR_TO_PADDR(addr);
    //kprintf("PADDR: %p\n", (void *)addr);
    releaseppages(addr);
}

void
vm_tlbshootdown_all(void)
{
	panic("dumbvm tried to do tlb shootdown?!\n");
}

void
vm_tlbshootdown(const struct tlbshootdown *ts)
{
	(void)ts;
	panic("dumbvm tried to do tlb shootdown?!\n");
}

int
vm_fault(int faulttype, vaddr_t faultaddress)
{
	vaddr_t vbase1, vtop1, vbase2, vtop2, stackbase, stacktop;
	paddr_t paddr;
	int i;
	uint32_t ehi, elo;
	struct addrspace *as;
	int spl;
        bool readonly = 0;

	faultaddress &= PAGE_FRAME;

	DEBUG(DB_VM, "dumbvm: fault: 0x%x\n", faultaddress);

	switch (faulttype) {
	    case VM_FAULT_READONLY:
		//TODO:kill current process
                return EFAULT;
		//panic("dumbvm: got VM_FAULT_READONLY\n");
	    case VM_FAULT_READ:
	    case VM_FAULT_WRITE:
		break;
	    default:
		return EINVAL;
	}

	if (curproc == NULL) {
		/*
		 * No process. This is probably a kernel fault early
		 * in boot. Return EFAULT so as to panic instead of
		 * getting into an infinite faulting loop.
		 */
		return EFAULT;
	}

	as = curproc_getas();
	if (as == NULL) {
		/*
		 * No address space set up. This is probably also a
		 * kernel fault early in boot.
		 */
		return EFAULT;
	}

	/* Assert that the address space has been set up properly. */
	KASSERT(as->as_vbase1 != 0);
	//KASSERT(as->as_pbase1 != 0); 
	KASSERT(as->as_npages1 != 0);
	KASSERT(as->as_vbase2 != 0);
	//KASSERT(as->as_pbase2 != 0);
	KASSERT(as->as_npages2 != 0);
	//KASSERT(as->as_stackpbase != 0);
	KASSERT((as->as_vbase1 & PAGE_FRAME) == as->as_vbase1);
	//KASSERT((as->as_pbase1 & PAGE_FRAME) == as->as_pbase1);
	KASSERT((as->as_vbase2 & PAGE_FRAME) == as->as_vbase2);
	//KASSERT((as->as_pbase2 & PAGE_FRAME) == as->as_pbase2);
	//KASSERT((as->as_stackpbase & PAGE_FRAME) == as->as_stackpbase);

        //KASSERT(as->as_ptable1 != NULL);
        //KASSERT(as->as_ptable2 != NULL);
        //KASSERT(as->as_ptableStack != NULL);

	vbase1 = as->as_vbase1;
	vtop1 = vbase1 + as->as_npages1 * PAGE_SIZE;
	vbase2 = as->as_vbase2;
	vtop2 = vbase2 + as->as_npages2 * PAGE_SIZE;
	//stackbase = USERSTACK - DUMBVM_STACKPAGES * PAGE_SIZE;
	stacktop = USERSTACK;

        /*
        if (faultaddress >= vbase1 && faultaddress < vtop1) {
            paddr = (faultaddress - vbase1) + as->as_pbase1;
        }
        else if (faultaddress >= vbase2 && faultaddress < vtop2) {
            paddr = (faultaddress - vbase2) + as->as_pbase2;
        }
        else if (faultaddress >= stackbase && faultaddress < stacktop) {
            paddr = (faultaddress - stackbase) + as->as_stackpbase;
        }
        else {
            return EFAULT;
        }
        */
        
        
        
        
	if (faultaddress >= vbase1 && faultaddress < vtop1) 
        {
            
            kprintf("ptable1: %p\n" , (void *)as->as_ptable1);
            for(size_t i = 0; i < as->as_npages1; ++i)
            {
                kprintf("i: %d, as->as_ptable1[i].pframebase: %p\n", (int)i, (void *)as->as_ptable1[i].pframebase);
            } 
            
            
            readonly = 1;
            vaddr_t offset = (faultaddress - vbase1);
            vaddr_t pageNum = offset / PAGE_SIZE;
            paddr = as->as_ptable1[pageNum].pframebase;

            
	}
	else if (faultaddress >= vbase2 && faultaddress < vtop2) 
        {
            
            kprintf("ptable2: %p\n" , (void *)as->as_ptable2);
            for(size_t i = 0; i < as->as_npages2; ++i)
            {
                kprintf("i: %d, as->as_ptable2[i].pframebase: %p\n", (int)i, (void *)as->as_ptable2[i].pframebase);
            }
            

            vaddr_t offset = (faultaddress - vbase2);
            vaddr_t pageNum = offset / PAGE_SIZE;
            
            paddr = as->as_ptable2[pageNum].pframebase;
            kprintf("offset: %d, pageNum: %d, as->as_ptable2[pageNum].pframebase: %p, paddr: %p\n", offset, pageNum, (void *) as->as_ptable2[pageNum].pframebase, (void *)paddr);
	}
	else if (faultaddress >= stackbase && faultaddress < stacktop) 
        {
            kprintf("ptableStack: %p\n" , (void *)as->as_ptableStack);
            for(size_t i = 0; i < DUMBVM_STACKPAGES; ++i)
            {
                kprintf("i: %d, as->as_ptableStack[i].pframebase: %p\n", (int)i, (void *)as->as_ptableStack[i].pframebase);
            }
            
            

            vaddr_t offset = (faultaddress - stackbase);
            vaddr_t pageNum = offset / PAGE_SIZE;
            paddr = as->as_ptableStack[pageNum].pframebase;
	}
	else {
		return EFAULT;
        }

	/* make sure it's page-aligned */
	KASSERT((paddr & PAGE_FRAME) == paddr);

	/* Disable interrupts on this CPU while frobbing the TLB. */
	spl = splhigh();

	for (i=0; i<NUM_TLB; i++) {
		tlb_read(&ehi, &elo, i);
		if (elo & TLBLO_VALID) {
			continue;
		}
		ehi = faultaddress;
                elo = paddr | TLBLO_DIRTY | TLBLO_VALID;

                if(readonly && as->as_elfLoaded)
                {
                    elo &= ~TLBLO_DIRTY;
                }

		DEBUG(DB_VM, "dumbvm: 0x%x -> 0x%x\n", faultaddress, paddr);
		tlb_write(ehi, elo, i);
		splx(spl);
		return 0;
	}


        //Ran out of TLB entries. replace random entry.
        ehi = faultaddress;
        elo = paddr | TLBLO_DIRTY | TLBLO_VALID;
        
        if(readonly && as->as_elfLoaded)
        {
            elo &= ~TLBLO_DIRTY;
        }


        tlb_random(ehi,elo);
        splx(spl);
        return 0;

        /*
	kprintf("dumbvm: Ran out of TLB entries - cannot handle page fault\n");
	splx(spl);
	return EFAULT;
        */
}

struct addrspace *
as_create(void)
{
	struct addrspace *as = kmalloc(sizeof(struct addrspace));
	if (as==NULL) {
		return NULL;
	}

        as->as_elfLoaded = 0;

	as->as_vbase1 = 0;
	
	as->as_npages1 = 0;
	as->as_vbase2 = 0;
	
	as->as_npages2 = 0;
	

        //as->as_pbase1 = 0;
        //as->as_pbase2 = 0;
        //as->as_stackpbase = 0;
        

        as->as_ptable1 = NULL;
        as->as_ptable2 = NULL;
        as->as_ptableStack = NULL;

	return as;
}

void
as_destroy(struct addrspace *as)
{
    
    for(size_t i = 0; i < DUMBVM_STACKPAGES; ++i)
    {
        releaseppages(as->as_ptableStack[i].pframebase);
    }
    free_kpages((vaddr_t) as->as_ptableStack);

    for(size_t i = 0; i < as->as_npages2; ++i)
    {
        releaseppages(as->as_ptable2[i].pframebase);
    }
    free_kpages((vaddr_t) as->as_ptable2);

    for(size_t i = 0; i < as->as_npages1; ++i)
    {
        releaseppages(as->as_ptable1[i].pframebase);
    }
    free_kpages((vaddr_t) as->as_ptable1);
    

    //releaseppages(as->as_stackpbase);
    //releaseppages(as->as_pbase2);
    //releaseppages(as->as_pbase1);
    
    kfree(as);
}

void
as_activate(void)
{
	int i, spl;
	struct addrspace *as;

	as = curproc_getas();
#ifdef UW
        /* Kernel threads don't have an address spaces to activate */
#endif
	if (as == NULL) {
		return;
	}

	/* Disable interrupts on this CPU while frobbing the TLB. */
	spl = splhigh();

	for (i=0; i<NUM_TLB; i++) {
		tlb_write(TLBHI_INVALID(i), TLBLO_INVALID(), i);
	}

	splx(spl);
}

void
as_deactivate(void)
{
	/* nothing */
}

int
as_define_region(struct addrspace *as, vaddr_t vaddr, size_t sz,
		 int readable, int writeable, int executable)
{
	size_t npages; 

	/* Align the region. First, the base... */
	sz += vaddr & ~(vaddr_t)PAGE_FRAME;
	vaddr &= PAGE_FRAME;

	/* ...and now the length. */
	sz = (sz + PAGE_SIZE - 1) & PAGE_FRAME;

	npages = sz / PAGE_SIZE;

	/* We don't use these - all pages are read-write */
	(void)readable;
	(void)writeable;
	(void)executable;

	if (as->as_vbase1 == 0) {
		as->as_vbase1 = vaddr;
		as->as_npages1 = npages;                           
		return 0;
	}

	if (as->as_vbase2 == 0) {
		as->as_vbase2 = vaddr;
		as->as_npages2 = npages;
		return 0;
	}

	/*
	 * Support for more than two regions is not available.
	 */
	kprintf("dumbvm: Warning: too many regions\n");
	return EUNIMP;
}

static
void
as_zero_region(paddr_t paddr, unsigned npages)
{
	bzero((void *)PADDR_TO_KVADDR(paddr), npages * PAGE_SIZE);
}

int
as_prepare_load(struct addrspace *as)
{        
        as->as_ptable1 = kmalloc(sizeof(struct pageEntiry) * as->as_npages1);
        if(as->as_ptable1 == NULL)
        {
            return ENOMEM;
        }
        for(size_t i = 0; i < as->as_npages1; ++i)
        {
            as->as_ptable1[i].pframebase = getppages(1);
            as_zero_region(as->as_ptable1[i].pframebase,1);
            //kprintf("i: %d, as->as_ptable1[i].pframebase: %p\n", (int)i, (void *)as->as_ptable1[i].pframebase);
        }
        
        kprintf("ptable1: %p\n" , (void *)as->as_ptable1);
        for(size_t i = 0; i < as->as_npages1; ++i)
        {
            kprintf("i: %d, as->as_ptable1[i].pframebase: %p\n", (int)i, (void *)as->as_ptable1[i].pframebase);
        }



        as->as_ptable2 = kmalloc(sizeof(struct pageEntiry) * as->as_npages2);
        if(as->as_ptable2 == NULL)
        {
            return ENOMEM;
        }
        for(size_t i = 0; i < as->as_npages2; ++i)
        {
            as->as_ptable2[i].pframebase = getppages(1);
            as_zero_region(as->as_ptable2[i].pframebase,1);
            //kprintf("i: %d, as->as_ptable2[i].pframebase: %p\n", (int)i, (void *)as->as_ptable2[i].pframebase);
        }

        kprintf("ptable2: %p\n" , (void *)as->as_ptable2);
        for(size_t i = 0; i < as->as_npages2; ++i)
        {
            kprintf("i: %d, as->as_ptable2[i].pframebase: %p\n", (int)i, (void *)as->as_ptable2[i].pframebase);
        } 



        as->as_ptableStack = kmalloc(sizeof(struct pageEntiry) * DUMBVM_STACKPAGES);
        if(as->as_ptableStack == NULL)
        {
            return ENOMEM;
        }
        for(size_t i = 0; i < DUMBVM_STACKPAGES; ++i)
        {
            as->as_ptableStack[i].pframebase = getppages(1);
            as_zero_region(as->as_ptableStack[i].pframebase,1);
            //kprintf("i: %d, as->as_ptableStack[i].pframebase: %p\n", (int)i, (void *)as->as_ptableStack[i].pframebase);
        }
        kprintf("ptableStack: %p\n" , (void *)as->as_ptableStack);
        for(size_t i = 0; i < DUMBVM_STACKPAGES; ++i)
        {
            kprintf("i: %d, as->as_ptableStack[i].pframebase: %p\n", (int)i, (void *)as->as_ptableStack[i].pframebase);
        } 
        
	return 0;
}

/*
int
as_prepare_load(struct addrspace *as)
{
    KASSERT(as->as_pbase1 == 0);
    KASSERT(as->as_pbase2 == 0);
    KASSERT(as->as_stackpbase == 0);

        
	as->as_pbase1 = getppages(as->as_npages1);
	if (as->as_pbase1 == 0) {
		return ENOMEM;
	}
        
        
        
	as->as_pbase2 = getppages(as->as_npages2);
	if (as->as_pbase2 == 0) {
		return ENOMEM;
	}
        

        
	as->as_stackpbase = getppages(DUMBVM_STACKPAGES);
	if (as->as_stackpbase == 0) {
		return ENOMEM;
	}
        
	
	as_zero_region(as->as_pbase1, as->as_npages1);
	as_zero_region(as->as_pbase2, as->as_npages2);
	as_zero_region(as->as_stackpbase, DUMBVM_STACKPAGES);

	return 0;
}
*/

int
as_complete_load(struct addrspace *as)
{
    
    as->as_elfLoaded = 1;
        
    for (int i=0; i<NUM_TLB; i++) 
    {
        tlb_write(TLBHI_INVALID(i), TLBLO_INVALID(), i);
    }

    return 0;
}

int
as_define_stack(struct addrspace *as, vaddr_t *stackptr)
{
    KASSERT(as->as_ptableStack != 0);
    //KASSERT(as->as_stackpbase);

	*stackptr = USERSTACK;
	return 0;
}

int
as_copy(struct addrspace *old, struct addrspace **ret)
{
	struct addrspace *new;

	new = as_create();
	if (new==NULL) {
		return ENOMEM;
	}

	new->as_vbase1 = old->as_vbase1;
	new->as_npages1 = old->as_npages1;
	new->as_vbase2 = old->as_vbase2;
	new->as_npages2 = old->as_npages2;

	/* (Mis)use as_prepare_load to allocate some physical memory. */
	if (as_prepare_load(new)) {
		as_destroy(new);
		return ENOMEM;
	}

	//KASSERT(new->as_pbase1 != 0);
	//KASSERT(new->as_pbase2 != 0);
	//KASSERT(new->as_stackpbase != 0);

        KASSERT(new->as_ptable1 != 0);
	KASSERT(new->as_ptable2 != 0);
	KASSERT(new->as_ptableStack != 0);

        /*
	memmove((void *)PADDR_TO_KVADDR(new->as_pbase1),
		(const void *)PADDR_TO_KVADDR(old->as_pbase1),
        	old->as_npages1*PAGE_SIZE);
        

        
	memmove((void *)PADDR_TO_KVADDR(new->as_pbase2),
		(const void *)PADDR_TO_KVADDR(old->as_pbase2),
		old->as_npages2*PAGE_SIZE);
        

        
	memmove((void *)PADDR_TO_KVADDR(new->as_stackpbase),
		(const void *)PADDR_TO_KVADDR(old->as_stackpbase),
		DUMBVM_STACKPAGES*PAGE_SIZE);
        */

        
        for(size_t i = 0; i < old->as_npages1; ++i)
        {
            memmove((void *)PADDR_TO_KVADDR(new->as_ptable1[i].pframebase),
                    (const void *)PADDR_TO_KVADDR(old->as_ptable1[i].pframebase),
                    PAGE_SIZE);
        }

        for(size_t i = 0; i < old->as_npages2; ++i)
        {
            memmove((void *)PADDR_TO_KVADDR(new->as_ptable2[i].pframebase),
                    (const void *)PADDR_TO_KVADDR(old->as_ptable2[i].pframebase),
                    PAGE_SIZE);
        }

        for(size_t i = 0; i < DUMBVM_STACKPAGES; ++i)
        {
            memmove((void *)PADDR_TO_KVADDR(new->as_ptableStack[i].pframebase),
                    (const void *)PADDR_TO_KVADDR(old->as_ptableStack[i].pframebase),
                    PAGE_SIZE);
        }
        
	
	*ret = new;
	return 0;
}
