#ifndef GENL_QUEUE_H
#define GENL_QUEUE_H

#include "genl_common.h"

struct nl_pending_req {
    uint32_t    seq;
    void        (*callback)(struct nlmsghdr *nlh); // pointer to any func with that signature: void fn(struct nlmsghdr *nlh)
    bool        is_dump;
};

struct nl_req_node {
    struct nl_pending_req   *req;
    struct nl_req_node      *next, *prev;
};

struct nl_req_queue {
    struct nl_req_node      *head, *tail;
    int                     count;
};

void genl_queue_init(struct nl_req_queue *q);
int genl_queue_add(struct nl_req_queue *q, uint32_t seq, genl_callback_t cb, bool is_dump);
int genl_queue_pop(struct nl_req_queue *q, uint32_t seq);
genl_callback_t genl_queue_find_and_pop(
    struct nl_req_queue *q, 
    uint32_t seq, 
    bool pop_flag // for dump it
);
void genl_queue_free(struct nl_req_queue *q);


#endif /* GENL_QUEUE_H */
