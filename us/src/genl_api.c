#include "genl_api.h"
#include "genl_debug.h" 
#include "genl_ops.h"

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

int handle_stdin_command(
    char *data,
    int sock_fd, 
    struct sockaddr_nl sa_local, 
    struct nl_req_queue *q
) 
{
    int ret = 0;

    char *ptr = data;
    char *word_end = ptr;
    char *args[64];
    int words_counter = 0;

    while (*word_end != '\0') {
        while (isspace(*ptr)) ptr++;
        if (*ptr == '\0') break;
    
        char *start = ptr;
        while (*ptr && !isspace(*ptr)) ptr++;
    
        int len = ptr - start;
        args[words_counter] = strndup(start, len);
        words_counter++;
    }
    
    for(int i = 0; i < words_counter; i++) {
        printf("index : %d, word: %s, len : %d\n", i, args[i], strlen(args[i]));
    }

    if (strcmp(args[0], "echo") == 0) {
        struct genlmytest_cmd_config cmd_config = {
            .str_param      = args[1],
            .uint32_param   = (uint32_t)strtoul(args[2], NULL, 0)
        };
        printf("calling echo func with params : %s,%d\n", cmd_config.str_param, cmd_config.uint32_param);
        ret = send_familytest_echo(sock_fd, sa_local, &cmd_config, send_family_echo_callback, q);
        goto out;
    }
out: 
    return ret; 
}

int handle_socket_message(int sock_fd, struct nl_req_queue *q) {
    int ret = 0;

    char recv_buf[BUFFER_RECEIVE_SIZE];
    struct nlmsghdr *nlh_recv = (struct nlmsghdr *)recv_buf;
    printf("recesved socket in handler: ");
    ret = receive_testfamily_msg_unicast(sock_fd, nlh_recv, BUFFER_RECEIVE_SIZE);

    if (ret < 0) {
        fprintf(stderr, "failed to receive message in socket handler\n");
        return ret;
    }
    printf("socket handler type: %d, seq: %d\n",nlh_recv->nlmsg_type ,nlh_recv->nlmsg_seq);
    if (nlh_recv->nlmsg_type == family_id) {
        genl_callback_t cb = genl_queue_find_and_pop(q, nlh_recv->nlmsg_seq, 0);
        if (cb) {
            cb(nlh_recv);
        }
    }
}
