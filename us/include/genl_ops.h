#ifndef GENL_OPS_H
#define GENL_OPS_H

#include "genl_common.h"
#include "genl_queue.h"

struct genlmytest_cmd_config {
    char     *str_param;
    uint32_t uint32_param;
};

int get_family_id(int sock_fd, const char *family_name, const struct sockaddr_nl *sa_local);

void send_family_echo_callback(struct nlmsghdr *nlh);
int send_familytest_echo(
    int sock_fd, 
    struct sockaddr_nl sa_local,
    struct genlmytest_cmd_config *in, 
    genl_callback_t callback,
    struct nl_req_queue *q
);

#endif /* GENL_OPS_H */