#ifndef _SYNCHPROBS_H_
#define _SYNCHPROBS_H_

/* student-implemented functions for the cat/mouse problem */

void cat_before_eating(unsigned int bowl);
void cat_after_eating(unsigned int bowl);
void mouse_before_eating(unsigned int bowl);
void mouse_after_eating(unsigned int bowl);
void catmouse_sync_init(int bowls);
void catmouse_sync_cleanup(int bowls);

#endif /* _SYNCHPROBS_H_ */
