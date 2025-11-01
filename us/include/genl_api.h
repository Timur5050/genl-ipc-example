#ifndef GENL_API_H
#define GENL_API_H

#include "genl_common.h"
#include "genl_queue.h"


void add_attr(struct nlmsghdr *nlh, int attr_type, const void *data, int data_len);

void parse_attrs(struct nlattr *nla_head, struct nlattr *tb[], int attrs_len, int max_attr);

void setup_kernel_address_unicast(void);

int send_testfamily_msg_unicast(int sock_fd, const struct nlmsghdr *nlh);

int receive_testfamily_msg_unicast(int sock_fd, struct nlmsghdr *nlh_buffer, int buffer_len);

int handle_stdin_command(
    char *data,
    int sock_fd, 
    struct sockaddr_nl sa_local, 
    struct nl_req_queue *q
);

int handle_socket_message(int sock_fd, struct nl_req_queue *q);

#endif /* GENL_API_H */
