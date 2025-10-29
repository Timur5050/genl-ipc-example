#ifndef GENL_OPS_H
#define GENL_OPS_H

#include "genl_common.h"

struct genlmytest_cmd_config {
    char     str_param[64];
    uint32_t uint32_param;
};

int get_family_id(int sock_fd, const char *family_name, const struct sockaddr_nl *sa_local);

int send_familytest_echo(
    int sock_fd, 
    int family_id, 
    struct sockaddr_nl sa_local,
    struct genlmytest_cmd_config *in, 
    struct genlmytest_cmd_config *out
);

#endif /* GENL_OPS_H */