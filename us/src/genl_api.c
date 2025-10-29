#include "genl_api.h"
#include "genl_debug.h" 

static struct sockaddr_nl sa_kernel_unicast;


void add_attr(struct nlmsghdr *nlh, int attr_type, const void *data, int data_len)
{
    struct nlattr *nla;

    nla = (struct nlattr *)((char *)nlh + NLMSG_ALIGN(nlh->nlmsg_len));

    nla->nla_type = attr_type;
    nla->nla_len = NLA_HDRLEN + data_len;
    memcpy(NLA_DATA(nla), data, data_len);

    nlh->nlmsg_len += NLA_ALIGN(nla->nla_len);
}

void parse_attrs(struct nlattr *nla_head, struct nlattr *tb[], int attrs_len, int max_attr)
{
    struct nlattr *nla;

    memset(tb, 0, sizeof(struct nlattr *) * (max_attr + 1));

    for(nla = nla_head; NLATTR_OK(nla, attrs_len); nla = NLATTR_NEXT(nla, attrs_len)) {
        if (nla->nla_type <= max_attr){
            tb[nla->nla_type] = nla;
        }
    }
}

void setup_kernel_address_unicast(void) 
{
    memset(&sa_kernel_unicast, 0, sizeof(sa_kernel_unicast));
    sa_kernel_unicast.nl_family = AF_NETLINK;
    sa_kernel_unicast.nl_pid = 0; // kernel
    sa_kernel_unicast.nl_groups = 0; // unicast
}

static struct msghdr *build_msg_body(struct iovec *iov, int len)
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
    if (!msg) {
        return -ENOMEM;
    }

    printf("send message :\n");
    print_full_nlmsg(msg);
    
    ret = sendmsg(sock_fd, msg, 0);
    if (ret < 0) {
        perror("sendmsg");
    }

    free(msg);
    return ret;
}

int receive_testfamily_msg_unicast(int sock_fd, struct nlmsghdr *nlh_buffer, int buffer_len)
{
    int ret = 0;

    if (!nlh_buffer) {
        return -EINVAL;
    }

    struct iovec iov[1] = {
        [0] = {
            .iov_base = (void *)nlh_buffer,
            .iov_len  = (size_t)buffer_len
        }
    };

    struct msghdr *msg = build_msg_body(iov, sizeof(iov) / sizeof(struct iovec));
    if (!msg) {
        return -ENOMEM;
    }

    ret = recvmsg(sock_fd, msg, 0);
    if (ret < 0)
    {
        perror("recvmsg");
        free(msg);
        return ret; 
    }

    if ((uint32_t)ret < sizeof(struct nlmsghdr)) {
        fprintf(stderr, "Message too short\n");
        free(msg);
        return -EMSGSIZE;
    }

    printf("receive msg: \n");
    print_full_nlmsg(msg);

    free(msg);
    
    return ret;
}
