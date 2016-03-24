/* UW specific code - This won't be needed or used until assignment 3 */

/* belongs in kern/vm/uw-vmstats.c */

/* NOTE !!!!!! WARNING !!!!!
 * All of the functions whose names begin with '_'
 * assume that atomicity is ensured elsewhere
 * (i.e., outside of these routines) by acquiring stats_lock.
 * All of the functions whose names do not begin
 * with '_' ensure atomicity locally.
 */

#include <types.h>
#include <lib.h>
#include <synch.h>
#include <spl.h>
#include <uw-vmstats.h>

/* Counters for tracking statistics */
static unsigned int stats_counts[VMSTAT_COUNT];

struct spinlock stats_lock = SPINLOCK_INITIALIZER;

/* Strings used in printing out the statistics */
static const char *stats_names[] = {
 /*  0 */ "TLB Faults", 
 /*  1 */ "TLB Faults with Free",
 /*  2 */ "TLB Faults with Replace",
 /*  3 */ "TLB Invalidations",
 /*  4 */ "TLB Reloads",
 /*  5 */ "Page Faults (Zeroed)",
 /*  6 */ "Page Faults (Disk)",
 /*  7 */ "Page Faults from ELF",
 /*  8 */ "Page Faults from Swapfile",
 /*  9 */ "Swapfile Writes",
};


/* ---------------------------------------------------------------------- */
/* Assumes vmstat_init has already been called */
void
vmstats_inc(unsigned int index)
{
    spinlock_acquire(&stats_lock);
      _vmstats_inc(index);
    spinlock_release(&stats_lock);
}

/* ---------------------------------------------------------------------- */
void
vmstats_init(void)
{
  /* Although the spinlock is initialized at declaration time we do it here
   * again in case we want use/reset these stats repeatedly without shutting down the kernel.
   */
  spinlock_init(&stats_lock);

  spinlock_acquire(&stats_lock);
    _vmstats_init();
  spinlock_release(&stats_lock);
}

/* ---------------------------------------------------------------------- */
void
_vmstats_inc(unsigned int index)
{
  KASSERT(index < VMSTAT_COUNT);
  stats_counts[index]++;
}

/* ---------------------------------------------------------------------- */
void
_vmstats_init(void)
{
  int i = 0;

  if (sizeof(stats_names) / sizeof(char *) != VMSTAT_COUNT) {
    kprintf("vmstats_init: number of stats_names = %d != VMSTAT_COUNT = %d\n",
      (sizeof(stats_names) / sizeof(char *)), VMSTAT_COUNT);
    panic("Should really fix this before proceeding\n");
  }

  for (i=0; i<VMSTAT_COUNT; i++) {
    stats_counts[i] = 0;
  }

}

/* ---------------------------------------------------------------------- */
/* Assumes vmstat_init has already been called */
/* NOTE: We do not grab the spinlock here because kprintf may block
 * and we can't block while holding a spinlock.
 * Just use this when there is only one thread remaining.
 */

void
vmstats_print(void)
{
  int i = 0;
  int free_plus_replace = 0;
  int disk_plus_zeroed_plus_reload = 0;
  int tlb_faults = 0;
  int elf_plus_swap_reads = 0;
  int disk_reads = 0;

  kprintf("VMSTATS:\n");
  for (i=0; i<VMSTAT_COUNT; i++) {
    kprintf("VMSTAT %25s = %10d\n", stats_names[i], stats_counts[i]);
  }

  tlb_faults = stats_counts[VMSTAT_TLB_FAULT];
  free_plus_replace = stats_counts[VMSTAT_TLB_FAULT_FREE] + stats_counts[VMSTAT_TLB_FAULT_REPLACE];
  disk_plus_zeroed_plus_reload = stats_counts[VMSTAT_PAGE_FAULT_DISK] +
    stats_counts[VMSTAT_PAGE_FAULT_ZERO] + stats_counts[VMSTAT_TLB_RELOAD];
  elf_plus_swap_reads = stats_counts[VMSTAT_ELF_FILE_READ] + stats_counts[VMSTAT_SWAP_FILE_READ];
  disk_reads = stats_counts[VMSTAT_PAGE_FAULT_DISK];

  kprintf("VMSTAT TLB Faults with Free + TLB Faults with Replace = %d\n", free_plus_replace);
  if (tlb_faults != free_plus_replace) {
    kprintf("WARNING: TLB Faults (%d) != TLB Faults with Free + TLB Faults with Replace (%d)\n",
      tlb_faults, free_plus_replace); 
  }

  kprintf("VMSTAT TLB Reloads + Page Faults (Zeroed) + Page Faults (Disk) = %d\n",
    disk_plus_zeroed_plus_reload);
  if (tlb_faults != disk_plus_zeroed_plus_reload) {
    kprintf("WARNING: TLB Faults (%d) != TLB Reloads + Page Faults (Zeroed) + Page Faults (Disk) (%d)\n",
      tlb_faults, disk_plus_zeroed_plus_reload); 
  }

  kprintf("VMSTAT ELF File reads + Swapfile reads = %d\n", elf_plus_swap_reads);
  if (disk_reads != elf_plus_swap_reads) {
    kprintf("WARNING: ELF File reads + Swapfile reads != Page Faults (Disk) %d\n",
      elf_plus_swap_reads);
  }
}
/* ---------------------------------------------------------------------- */
