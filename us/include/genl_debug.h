#ifndef GENL_DEBUG_H
#define GENL_DEBUG_H

#include "genl_common.h"

void print_nl_attrs(struct nlattr *nla_head, int attrs_len);
void print_nl_msg(struct nlmsghdr *nlh);
void print_full_nlmsg(const struct msghdr *msg);

#endif /* GENL_DEBUG_H */