#ifndef GENL_COMMON_H
#define GENL_COMMON_H

#include <stdio.h>
#include <poll.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <stdbool.h>
#include <ctype.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <linux/genetlink.h>
#include <sys/uio.h>

#include "../ks/genlmytest.h" 

#define BUFFER_RECEIVE_SIZE 4096
#define MAX_LINE 1024

#define NLA_DATA(nla)   ((void *)((char *)(nla) + NLA_HDRLEN))
#define NLA_LEN(nla)    ((int)((nla)->nla_len - NLA_HDRLEN))

#define NLATTR_OK(nla, len) ((len) >= (int)sizeof(struct nlattr) && \
                             (nla->nla_len >= sizeof(struct nlattr)) && \
                             (nla->nla_len <= (len)))

#define NLATTR_NEXT(nla, len) ((len) -= NLA_ALIGN((nla)->nla_len), \
                               (struct nlattr*)((char *)(nla) + NLA_ALIGN((nla)->nla_len)))

extern int family_id;
extern uint32_t global_message_counter;

typedef void (*genl_callback_t)(struct nlmsghdr *nlh);

#endif 