#include "genl_ops.h"
#include "genl_api.h" 
#include "genl_common.h" 
#include "genl_queue.h"

int get_family_id(int sock_fd, const char *family_name, const struct sockaddr_nl *sa_local) 
{
    int ret = 0;
    
    char send_buf[4096]; 
    
    char recv_buf[BUFFER_RECEIVE_SIZE]; 
    
    struct nlmsghdr *nlh_send;
    struct nlmsghdr *nlh_recv;
    struct genlmsghdr *genlh;

    nlh_send = (struct nlmsghdr *)send_buf;
    genlh = (struct genlmsghdr *)NLMSG_DATA(nlh_send);

    nlh_send->nlmsg_type     = GENL_ID_CTRL;
    nlh_send->nlmsg_flags    = NLM_F_REQUEST | NLM_F_ACK;
    nlh_send->nlmsg_seq      = 0; 
    nlh_send->nlmsg_pid      = sa_local->nl_pid;
    nlh_send->nlmsg_len      = NLMSG_HDRLEN + GENL_HDRLEN;

    genlh->cmd = CTRL_CMD_GETFAMILY;
    genlh->version = 1;

    add_attr(nlh_send, CTRL_ATTR_FAMILY_NAME, family_name, strlen(family_name) + 1);

    ret = send_testfamily_msg_unicast(sock_fd, nlh_send);
    if (ret < 0) {
        perror("sendmsg");
        goto out;
    }

    nlh_recv = (struct nlmsghdr *)recv_buf;
    ret = receive_testfamily_msg_unicast(sock_fd, nlh_recv, BUFFER_RECEIVE_SIZE);
    
    if (ret < 0) { 
        fprintf(stderr, "Failed to receive message\n");
        goto out;
    }

    if (nlh_recv->nlmsg_type != GENL_ID_CTRL) {
        if (nlh_recv->nlmsg_type == NLMSG_ERROR) {
            struct nlmsgerr *err = NLMSG_DATA(nlh_recv);
            if (err->error == -ENOENT)
            {
                fprintf(stderr, "family '%s' not found (ENOENT)\n", family_name);
                ret = -ENOENT;
            } else {
                fprintf(stderr, "Got error: %d\n", err->error);
                ret = err->error;
            }
        } else {
            fprintf(stderr, "Got unexpected message type %d\n", nlh_recv->nlmsg_type);
            ret = -1;
        }
        goto out;
    }

    genlh = (struct genlmsghdr *)NLMSG_DATA(nlh_recv);
    struct nlattr *tb[CTRL_ATTR_MAX + 1];

    parse_attrs(
        (struct nlattr *)((char *)genlh + GENL_HDRLEN),
        tb,
        nlh_recv->nlmsg_len - NLMSG_HDRLEN - GENL_HDRLEN,
        CTRL_ATTR_MAX 
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

void send_family_echo_callback(struct nlmsghdr *nlh) {
    struct nlattr *tb_echo[GENLMYTEST_ATTR_MAX + 1];
    struct genlmsghdr *genlh = (struct genlmsghdr *)NLMSG_DATA(nlh);

    parse_attrs(
        (struct nlattr *)((char *)genlh + GENL_HDRLEN),
        tb_echo,
        nlh->nlmsg_len - NLMSG_HDRLEN - GENL_HDRLEN,
        GENLMYTEST_ATTR_MAX
    );

    printf("echo callback : type : %d, seq : %d\n", nlh->nlmsg_type, nlh->nlmsg_seq);
    if (tb_echo[GENLMYTEST_ATTR_TEXT]) {
        printf("returnet text : %s\n", (char *)NLA_DATA(tb_echo[GENLMYTEST_ATTR_TEXT]));
    }

    if (tb_echo[GENLMYTEST_ATTR_NUM]) {
        printf("returned num + 1: %d\n", *(uint32_t*)NLA_DATA(tb_echo[GENLMYTEST_ATTR_NUM]));
    }
}


int send_familytest_echo(
    int sock_fd, 
    struct sockaddr_nl sa_local,
    struct genlmytest_cmd_config *in, 
    genl_callback_t callback,
    struct nl_req_queue *q
)
{
    int ret = 0;
    char buf[4096];
    struct nlmsghdr *nlh;
    struct genlmsghdr *genlh;

    memset(buf, 0, sizeof(buf));

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
    printf("params: %s, %d\n", in->str_param, in->uint32_param);
    add_attr(nlh, GENLMYTEST_ATTR_TEXT, in->str_param, strlen(in->str_param) + 1);
    add_attr(nlh, GENLMYTEST_ATTR_NUM, &(in->uint32_param), sizeof(in->uint32_param));

    /* register callback before sending message */
    ret = genl_queue_add(q, nlh->nlmsg_seq, callback, 0);
    if (ret < 0) {
        fprintf(stderr, "error while adding callback to the ll\n");
        goto out;
    }

    /* send message to kernel */
    ret = send_testfamily_msg_unicast(sock_fd, nlh);
    if (ret < 0) {
        perror("error while sending fanilytest cmd");
        genl_queue_pop(q, nlh->nlmsg_seq);
        goto out;
    }

out:
    return (ret < 0) ? ret : 0;
}