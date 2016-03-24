#ifndef _COREMAP_H_
#define _COREMAP_H_

#include <spinlock.h>

struct coremap_entry
{
    paddr_t paddr;
    paddr_t owner;
    volatile bool inUse;
};

struct coremap
{
    struct spinlock coreLock;
    struct coremap_entry *entries;
    unsigned int coremapSize;
};

struct coremap *init_coremap(void);
static void releaseppages(paddr_t paddr);

#endif /* _COREMAP_H_ */
