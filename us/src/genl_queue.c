#include "genl_queue.h"


void genl_queue_init(struct nl_req_queue *q) {
    if (!q) {
        return;
    }
    q->count    = 0;
    q->head     = NULL;
    q->tail     = NULL;
    }

int genl_queue_add(struct nl_req_queue *q, uint32_t seq, genl_callback_t cb, bool is_dump) {
    int ret = 0;

    if (!q) {
        ret = -1;
        goto out;
    }

    struct nl_pending_req *temp_req = (struct nl_pending_req *)malloc(sizeof(struct nl_pending_req));
    if (NULL == temp_req) {
        ret = -1;
        goto out;
    }
    temp_req->seq       = seq;
    temp_req->callback  = cb;
    temp_req->is_dump   = is_dump;

    struct nl_req_node *temp_node = (struct nl_req_node*)malloc(sizeof(struct nl_req_node));
    if (NULL == temp_node) {
        free(temp_req);
        ret = -1;
        goto out;
    }
    temp_node->req      = temp_req;
    temp_node->next     = NULL;
    ret                 = seq;

    if (q->tail) {
        q->tail->next   = temp_node;
        temp_node->prev = q->tail;
        q->tail         = temp_node;
    }
    else {
        temp_node->prev = NULL;
        q->head         = temp_node;
        q->tail         = temp_node;
    }

    q->count++;

out:
    return ret;
}

int genl_queue_pop(struct nl_req_queue *q, uint32_t seq) {
    int ret = 0;

    if (!q || !q->head) {
        goto out;
    }

    struct nl_req_node *temp_node = q->head;
    while (temp_node) {
        if (temp_node->req->seq == seq) {
            if (temp_node == q->head) {
                q->head = temp_node->next;
                if (q->head)    q->head->prev = NULL;
            } else if (temp_node == q->tail) {
                q->tail = temp_node->prev;
                if (q->tail)    q->tail->next = NULL;
            } else {
                temp_node->prev->next = temp_node->next;
                temp_node->next->prev = temp_node->prev; 
            }
            free(temp_node->req);
            free(temp_node);
            q->count--;
            ret = seq;
            goto out;
        }
        temp_node = temp_node->next;
    }

out:
    return ret;
}

genl_callback_t genl_queue_find_and_pop(
    struct nl_req_queue *q,
    uint32_t seq,
    bool pop_flag
)
{
    genl_callback_t ret = NULL;

    if (!q) {
        goto out;
    }

    struct nl_req_node *temp_node = q->head;

    while (temp_node) {
        if (temp_node->req->seq == seq) {
            ret = temp_node->req->callback;
            if (pop_flag) genl_queue_pop(q, seq);
            goto out;
        }
        temp_node = temp_node->next;
    }

out:
    return ret;
}

void genl_queue_free(struct nl_req_queue *q) {
    if (!q) {
        return;
    }
    struct nl_req_node *temp_node = q->head;
    while (temp_node) {
        struct nl_req_node *next = temp_node->next;
        free(temp_node->req);       
        free(temp_node);
        temp_node = next;
    }
    q->head     = NULL;
    q->tail     = NULL;
    q->count    = 0;
}
