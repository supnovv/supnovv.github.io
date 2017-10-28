#ifndef l_core_queue_h
#define l_core_queue_h
#include "core/base.h"

/**
 * simple linked queue
 */

typedef struct l_squeue {
  l_smplnode head;
  l_smplnode* tail;
} l_squeue;

L_EXTERN void l_squeue_init(l_squeue* self);
L_EXTERN void l_squeue_push(l_squeue* self, l_smplnode* newnode);
L_EXTERN void l_squeue_pushQueue(l_squeue* self, l_squeue* queue);
L_EXTERN int l_squeue_isEmpty(l_squeue* self);
L_EXTERN l_smplnode* l_squeue_pop(l_squeue* self);

/**
 * bidirectional queue
 */

typedef struct l_dqueue {
  l_linknode head;
} l_dqueue;

L_EXTERN void l_dqueue_init(l_dqueue* self);
L_EXTERN void l_dqueue_push(l_dqueue* self, l_linknode* newnode);
L_EXTERN void l_dqueue_pushQueue(l_dqueue* self, l_dqueue* queue);
L_EXTERN int l_dqueue_isEmpty(l_dqueue* self);
L_EXTERN l_linknode* l_dqueue_pop(l_dqueue* self);

/**
 * priority queue
 */

typedef struct l_priorq {
  l_linknode node;
  /* elem with less number has higher priority, i.e., 0 is the highest */
  int (*less)(void* elem_is_less_than, void* this_one);
} l_priorq;

L_EXTERN void l_priorq_init(l_priorq* self, int (*less)(void*, void*));
L_EXTERN void l_priorq_push(l_priorq* self, l_linknode* elem);
L_EXTERN void l_priorq_remove(l_priorq* self, l_linknode* elem);
L_EXTERN int l_priorq_isEmpty(l_priorq* self);
L_EXTERN l_linknode* l_priorq_pop(l_priorq* self);

#endif /* l_core_queue_h */

