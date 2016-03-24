#ifndef VM_STATS_H
#define VM_STATS_H

/* UW specific code - This won't be needed or used until assignment 3 */

/* belongs in kern/include/uw-vmstat.h */

/* ----------------------------------------------------------------------- */
/* Virtual memory stats */
/* Tracks stats on user programs */

/* NOTE !!!!!! WARNING !!!!!
 * All of the functions (except vmstats_print) whose names begin with '_'
 * assume that atomicity is ensured elsewhere
 * (i.e., outside of these routines) by acquiring stats_lock.
 * All of the functions whose names do not begin
 * with '_' ensure atomicity locally (except vmstats_print).
 *
 * Generally you will use the functions whose names
 * do not begin with '_'.
 */


/* These are the different stats that get tracked.
 * See vmstats.c for strings corresponding to each stat.
 */

/* DO NOT ADD OR CHANGE WITHOUT ALSO CHANGING vmstats.h */
#define VMSTAT_TLB_FAULT              (0)
#define VMSTAT_TLB_FAULT_FREE         (1)
#define VMSTAT_TLB_FAULT_REPLACE      (2)
#define VMSTAT_TLB_INVALIDATE         (3)
#define VMSTAT_TLB_RELOAD             (4)
#define VMSTAT_PAGE_FAULT_ZERO        (5)
#define VMSTAT_PAGE_FAULT_DISK        (6)
#define VMSTAT_ELF_FILE_READ          (7)
#define VMSTAT_SWAP_FILE_READ         (8)
#define VMSTAT_SWAP_FILE_WRITE        (9)
#define VMSTAT_COUNT                 (10)

/* ----------------------------------------------------------------------- */

/* Initialize the statistics: must be called before using */
void vmstats_init(void);                     /* uses locking */
void _vmstats_init(void);                    /* atomicity must be ensured elsewhere */

/* Increment the specified count 
 * Example use: 
 *   vmstats_inc(VMSTAT_TLB_FAULT);
 *   vmstats_inc(VMSTAT_PAGE_FAULT_ZERO);
 */
void vmstats_inc(unsigned int index);    /* uses locking */
void _vmstats_inc(unsigned int index);   /* atomicity must be ensured elsewhere */

/* Print the statistics: assumes that at least vmstats_init has been called */
void vmstats_print(void);                    /* Does NOT use locking */

#endif /* VM_STATS_H */
