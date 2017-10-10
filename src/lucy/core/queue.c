#define L_LIBRARY_IMPL
#include "core/queue.h"

L_EXTERN void
l_squeue_init(l_squeue* self)
{
  l_smplnode_init(&self->head);
  self->tail = &self->head;
}

L_EXTERN void
l_squeue_push(l_squeue* self, l_smplnode* newnode)
{
  l_smplnode_insertAfter(self->tail, newnode);
  self->tail = newnode;
}

L_EXTERN void
l_squeue_pushQueue(l_squeue* self, l_squeue* q)
{
  if (l_squeue_isEmpty(q)) return;
  self->tail->next = q->head.next;
  self->tail = q->tail;
  self->tail->next = &self->head;
  l_squeue_init(q);
}

L_EXTERN int
l_squeue_isEmpty(l_squeue* self)
{
  return (self->head.next == &self->head);
}

L_EXTERN l_smplnode*
l_squeue_pop(l_squeue* self)
{
  l_smplnode* node = 0;
  if (l_squeue_isEmpty(self)) {
    return 0;
  }
  node = l_smplnode_removeNext(&self->head);
  if (node == self->tail) {
    self->tail = &self->head;
  }
  return node;
}

L_EXTERN void
l_dqueue_init(l_dqueue* self)
{
  l_linknode_init(&self->head);
}

L_EXTERN void
l_dqueue_push(l_dqueue* self, l_linknode* newnode)
{
  l_linknode_insertAfter(&self->head, newnode);
}

L_EXTERN void
l_dqueue_pushQueue(l_dqueue* self, l_dqueue* q)
{
  l_linknode* tail = 0;
  if (l_dqueue_isEmpty(q)) return;
  /* chain self's tail with q's first element */
  tail = self->head.prev;
  tail->next = q->head.next;
  q->head.next->prev = tail;
  /* chain q's tail with self's head */
  tail = q->head.prev;
  tail->next = &self->head;
  self->head.prev = tail;
  /* init q to empty */
  l_dqueue_init(q);
}

L_EXTERN int
l_dqueue_isEmpty(l_dqueue* self)
{
  return l_linknode_isEmpty(&self->head);
}

L_EXTERN l_linknode*
l_dqueue_pop(l_dqueue* self)
{
  if (l_dqueue_isEmpty(self)) return 0;
  return l_linknode_remove(self->head.prev);
}

L_EXTERN void
l_priorq_init(l_priorq* self, int (*less)(void*, void*))
{
  l_linknode_init(&self->node);
  self->less = less;
}

L_EXTERN int
l_priorq_isEmpty(l_priorq* self)
{
  return l_linknode_isEmpty(&self->node);
}

L_EXTERN void
l_priorq_push(l_priorq* self, l_linknode* elem)
{
  l_linknode* head = &(self->node);
  l_linknode* cur = head->next;
  while (cur != head && self->less(cur, elem)) {
    cur = cur->next;
  }
  /* insert elem before current node  or the head node */
  cur->prev->next = elem;
  elem->prev = cur->prev;
  cur->prev = elem;
  elem->next = cur;
}

L_EXTERN void
l_priorq_remove(l_priorq* self, l_linknode* elem)
{
  if (elem == &(self->node)) return;
  l_linknode_remove(elem);
}

L_EXTERN l_linknode*
l_priorq_pop(l_priorq* self)
{
  l_linknode* head;
  l_linknode* first;
  if (l_priorq_isEmpty(self)) {
    return 0;
  }
  head = &(self->node);
  first = head->next;
  head->next = first->next;
  first->next->prev = head;
  return first;
}

