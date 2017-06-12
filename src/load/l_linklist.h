#ifndef l_linklist_lib_h
#define l_linklist_lib_h

typedef struct cclinknode {
  struct cclinknode* next;
  struct cclinknode* prev;
} cclinknode;

CORE_API void cclinknode_init(cclinknode* node);
CORE_API nauty_bool cclinknode_isempty(cclinknode* node);
CORE_API void cclinknode_insertafter(cclinknode* node, cclinknode* newnode);
CORE_API cclinknode* cclinknode_remove(cclinknode* node);

typedef struct ccsmplnode {
  struct ccsmplnode* next;
} ccsmplnode;

CORE_API void ccsmplnode_init(ccsmplnode* node);
CORE_API nauty_bool ccsmplnode_isempty(ccsmplnode* node);
CORE_API void ccsmplnode_insertafter(ccsmplnode* node, ccsmplnode* newnode);
CORE_API ccsmplnode* ccsmplnode_removenext(ccsmplnode* node);

typedef struct ccsqueue {
  struct ccsmplnode head;
  struct ccsmplnode* tail;
} ccsqueue;

CORE_API void ccsqueue_init(ccsqueue* self);
CORE_API void ccsqueue_push(ccsqueue* self, ccsmplnode* newnode);
CORE_API void ccsqueue_pushqueue(ccsqueue* self, ccsqueue* queue);
CORE_API nauty_bool ccsqueue_isempty(ccsqueue* self);
CORE_API ccsmplnode* ccsqueue_pop(ccsqueue* self);

typedef struct ccdqueue {
  struct cclinknode head;
} ccdqueue;

CORE_API void ccdqueue_init(ccdqueue* self);
CORE_API void ccdqueue_push(ccdqueue* self, cclinknode* newnode);
CORE_API void ccdqueue_pushqueue(ccdqueue* self, ccdqueue* queue);
CORE_API nauty_bool ccdqueue_isempty(ccdqueue* self);
CORE_API cclinknode* ccdqueue_pop(ccdqueue* self);

typedef struct ccpriorq {
  struct cclinknode node;
  /* elem with less number has higher priority, i.e., 0 is the highest */
  nauty_bool (*less)(void* elem_is_less_than, void* this_one);
} ccpriorq;

CORE_API void ccpriorq_init(ccpriorq* self, nauty_bool (*less)(void*, void*));
CORE_API void ccpriorq_push(ccpriorq* self, cclinknode* elem);
CORE_API void ccpriorq_remove(ccpriorq* self, cclinknode* elem);
CORE_API nauty_bool ccpriorq_isempty(ccpriorq* self);
CORE_API cclinknode* ccpriorq_pop(ccpriorq* self);

#endif /* l_linklist_lib_h */

