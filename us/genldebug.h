#ifndef GENLDEBUG_H
#define GENLDEBUG_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <linux/genetlink.h>
#include <sys/uio.h>
#include <unistd.h>

#define BUFFER_RECEIVE_SIZE 4096

extern int family_id;
uint32_t global_message_counter = 0;

#define NLATTR_OK(nla, len) ((len) >= (int)sizeof(struct nlattr) && \
                                (nla->nla_len >= sizeof(struct nlattr)) && \
                                (nla->nla_len <= (len)))

#define NLATTR_NEXT(nla, len) ((len) -= NLA_ALIGN((nla)->nla_len), \
                                (struct nlattr*)((char *)(nla) + NLA_ALIGN((nla)->nla_len)))

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

struct genlmytest_cmd_config {
    char     str_param[64];
    uint32_t uint32_param;
};

int get_familt_id(const int *sock_fd, const char *family_name, const struct sockaddr_nl *sa_local);
void add_attr(struct nlmsghdr *nlh, int attr_type, const void *data, int data_len);

void print_nl_attrs(struct nlattr *nla_head, int attrs_len);
void print_nl_msg(struct nlmsghdr *nlh);
void print_full_nlmsg(const struct msghdr *msg);


#endif /* GENLDEBUG_H */
