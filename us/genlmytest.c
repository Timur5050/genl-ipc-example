#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <linux/genetlink.h>

#include "../ks/genlmytest.h"
#include "genldebug.h" 

#define NLA_DATA(nla) ((void *)((char *)(nla) + NLA_HDRLEN))

struct sockaddr_nl sa_kernel_unicast;

void add_attr(struct nlmsghdr *nlh, int attr_type, const void *data, int data_len)
{
    struct nlattr *nla;

    nla = (struct nlattr *)((char *)nlh + NLMSG_ALIGN(nlh->nlmsg_len));

    nla->nla_type = attr_type;
    nla->nla_len = NLA_HDRLEN + data_len;
    memcpy(NLA_DATA(nla), data, data_len);

    nlh->nlmsg_len += NLA_ALIGN(nla->nla_len);
}

void parse_attrs(struct nlattr *nla_head, struct nlattr *tb[], int attrs_len)
{
    struct nlattr *nla;

    memset(tb, 0, sizeof(struct nlattr *) * (CTRL_ATTR_MAX + 1));

    for(nla = nla_head; NLATTR_OK(nla, attrs_len); nla = NLATTR_NEXT(nla, attrs_len)) {
        if (nla->nla_type <= CTRL_ATTR_MAX){
            tb[nla->nla_type] = nla;
        }
    }
}

void setup_kernel_address_unicast(struct sockaddr_nl *sa_kernel) 
{
    memset(sa_kernel, 0, sizeof(*sa_kernel));
    sa_kernel->nl_family = AF_NETLINK;
    sa_kernel->nl_pid = 0; // kernel
    sa_kernel->nl_groups = 0; // unicast
}

struct msghdr *build_msg_body(struct iovec *iov, int len)
{
    struct msghdr *msg = (struct msghdr *)malloc(sizeof(struct msghdr));
    
    if (msg == NULL)
    {
        perror("failed to malloc msg\n");
        return NULL;
    }

    msg->msg_name       = (void *)&sa_kernel_unicast;
    msg->msg_namelen    = sizeof(sa_kernel_unicast);
    msg->msg_iov        = iov;
    msg->msg_iovlen     = len;

    return msg;
}

int send_testfamily_msg_unicast(int sock_fd, const struct nlmsghdr *nlh)
{
    int ret = 0;

    struct iovec iov[1] = {
        [0] = {
            .iov_base = (void *)nlh, 
            .iov_len  = nlh->nlmsg_len
        }
    };

    struct msghdr *msg = build_msg_body(iov, sizeof(iov) / sizeof(struct iovec));
    printf("send message :\n");
    print_full_nlmsg(msg);
    
    ret = sendmsg(sock_fd, msg, 0);
    if (ret < 0) {
        perror("sendmsg");
    }

    free(msg);
    return ret;
}

struct nlmsghdr* receive_testfamily_msg_unicast(int sock_fd)
{
    int ret = 0;
    char *buffer = (char *)malloc(BUFFER_RECEIVE_SIZE);

    struct iovec iov[1] = {
        [0] = {
            .iov_base = buffer,
            .iov_len  = BUFFER_RECEIVE_SIZE
        }
    };

    struct msghdr *msg = build_msg_body(iov, sizeof(iov) / sizeof(struct iovec));

    ret = recvmsg(sock_fd, msg, 0);
    if (ret < 0)
    {
        perror("recvmsg");
        free(buffer);
        free(msg);
        return NULL;
    }

    if ((uint32_t)ret < sizeof(struct nlmsghdr)) {
        fprintf(stderr, "Message too short\n");
        free(buffer);
        free(msg);
        return NULL;
    }

    printf("receive msg: \n");
    print_full_nlmsg(msg);

    free(msg);
    return (struct nlmsghdr *)buffer;
}

int get_family_id(int sock_fd, const char *family_name, const struct sockaddr_nl *sa_local) 
{
    int ret = 0;
    char buf[4096];
    struct nlmsghdr *nlh;
    struct genlmsghdr *genlh;

    /* prepare message */
    nlh = (struct nlmsghdr *)buf;
    genlh = (struct genlmsghdr *)NLMSG_DATA(nlh);

    nlh->nlmsg_type     = GENL_ID_CTRL;
    nlh->nlmsg_flags    = NLM_F_REQUEST | NLM_F_ACK;
    nlh->nlmsg_seq      = 1;
    nlh->nlmsg_pid      = sa_local->nl_pid;
    nlh->nlmsg_len      = NLMSG_HDRLEN + GENL_HDRLEN;

    genlh->cmd = CTRL_CMD_GETFAMILY;
    genlh->version = 1;

    add_attr(nlh, CTRL_ATTR_FAMILY_NAME, family_name, strlen(family_name) + 1);

    /* send message to kernel*/
    ret = send_testfamily_msg_unicast(sock_fd, nlh);
    if (ret < 0) {
        perror("sendmsg");
        goto out;
    }

    /* receive response from kernel */
    nlh = receive_testfamily_msg_unicast(sock_fd);

    /* parse response */
    if (nlh->nlmsg_type != GENL_ID_CTRL) {
        if (nlh->nlmsg_type == NLMSG_ERROR) {
            struct nlmsgerr *err = NLMSG_DATA(nlh);
            if (err->error == -ENOENT)
            {
                fprintf(stderr, "family '%s' not found (ENOENT)\n", family_name);
                ret = -ENOENT;
                goto out;
            }
            fprintf(stderr, "Got error: %d\n", err->error);
            ret = err->error;
            goto out;
        }
        fprintf(stderr, "Got unexpected message type %d\n", nlh->nlmsg_type);
        ret = -1;
        goto out;
    }

    genlh = (struct genlmsghdr *)NLMSG_DATA(nlh);
    struct nlattr *tb[CTRL_ATTR_MAX + 1];

    parse_attrs(
        (struct nlattr *)((char *)genlh + GENL_HDRLEN),
        tb,
        nlh->nlmsg_len - NLMSG_HDRLEN - GENL_HDRLEN
    );

    if (tb[CTRL_ATTR_FAMILY_ID]) {
        ret = *( (uint16_t *) NLA_DATA(tb[CTRL_ATTR_FAMILY_ID]) );
    } else {
        fprintf(stderr, "Failed to get family id for %s\n", family_name);
        ret = -1;
    }

out:
    return ret;

}

int send_familytest_echo(
    int sock_fd, 
    int family_id, 
    struct sockaddr_nl sa_local,
    struct genlmytest_cmd_config *in, 
    struct genlmytest_cmd_config *out
)
{
    int ret = 0;
    char buf[4096];
    struct nlmsghdr *nlh;
    struct genlmsghdr *genlh;

    /* prepare message */
    nlh = (struct nlmsghdr *)buf;
    genlh = (struct genlmsghdr *)NLMSG_DATA(nlh);

    nlh->nlmsg_type     = family_id;
    nlh->nlmsg_flags    = NLM_F_REQUEST | NLM_F_ACK;
    nlh->nlmsg_seq      = global_message_counter++;
    nlh->nlmsg_pid      = sa_local.nl_pid,
    nlh->nlmsg_len      = NLMSG_HDRLEN + GENL_HDRLEN;

    genlh->cmd          = GENLMYTEST_CMD_ECHO;
    genlh->version      = 1;

    add_attr(nlh, GENLMYTEST_ATTR_TEXT, &(in->str_param), strlen(in->str_param));
    add_attr(nlh, GENLMYTEST_ATTR_NUM, &(in->uint32_param), sizeof(in->uint32_param));

    /* send message to kernel */
    ret = send_testfamily_msg_unicast(sock_fd, nlh);
    if (ret < 0) {
        perror("error while sending fanilytest cmd");
        goto out;
    }

    /* receive response from kernel*/
    nlh = reveice_testfamily_msg_unicast(sock_fd);




out:
    return ret;
}

int main() 
{
    int ret = 0;

    int sock_fd;
    struct sockaddr_nl sa_local;

    sock_fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_GENERIC);
    if (sock_fd < 0) {
        perror("socket");
        return EXIT_FAILURE;
    }

    /* bind local pid to socket */
    memset(&sa_local, 0, sizeof(sa_local));
    sa_local.nl_family = AF_NETLINK;
    sa_local.nl_pid = getpid(); // self pid
    ret = bind(sock_fd, (struct sockaddr *)&sa_local, sizeof(sa_local));
    if (ret < 0) {
        perror("bind");
        close(sock_fd);
        return EXIT_FAILURE;
    }

    setup_kernel_address_unicast(&sa_kernel_unicast);

    /* get family id */
    family_id = get_family_id(sock_fd, GENLMYTEST_GENL_NAME, &sa_local);
    if (family_id < 0) {
        printf("family is not found '%s', error : %d\n", GENLMYTEST_GENL_NAME, family_id);
        close(sock_fd);
        return EXIT_FAILURE;
    }

    printf("family id %x for name: %s\n", family_id, GENLMYTEST_GENL_NAME);
    
    


    close(sock_fd);
    return EXIT_SUCCESS;

}

