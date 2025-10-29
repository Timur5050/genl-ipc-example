#include "genl_debug.h"

/* ---------------- ATTRIBUTES ---------------- */
void print_nl_attrs(struct nlattr *nla_head, int attrs_len)
{
    struct nlattr *nla;
    int index = 0;

    printf("  [ATTRS]\n");
    for (nla = nla_head; NLATTR_OK(nla, attrs_len); nla = NLATTR_NEXT(nla, attrs_len)) {
        printf("    • Attr %d: type=%u, len=%u\n",
               index, nla->nla_type, nla->nla_len);
        index++;
    }
}

/* ---------------- NLMSG ---------------- */
void print_nl_msg(struct nlmsghdr *nlh)
{
    printf("[NLMSG HDR]\n");
    printf("  len=%u, type=%u, flags=0x%x, seq=%u, pid=%u\n",
           nlh->nlmsg_len, nlh->nlmsg_type, nlh->nlmsg_flags,
           nlh->nlmsg_seq, nlh->nlmsg_pid);

    switch (nlh->nlmsg_type) {
        case NLMSG_ERROR: {
            struct nlmsgerr *err = (struct nlmsgerr *)NLMSG_DATA(nlh);
            printf("  Message type: NLMSG_ERROR\n");
            printf("  → Error code: %d\n", err->error);
            break;
        }

        default: {
            struct genlmsghdr *gh = NLMSG_DATA(nlh);

            if (nlh->nlmsg_type == family_id) {
                printf("  Message type: MY_FAMILY (%d)\n", family_id);
            } else if (nlh->nlmsg_type == GENL_ID_CTRL) {
                printf("  Message type: GENL_ID_CTRL\n");
            } else {
                printf("  Message type: UNKNOWN (%u)\n", nlh->nlmsg_type);
            }

            printf("[GENL HDR]\n");
            printf("  cmd=%u, version=%u, reserved=%u\n",
                   gh->cmd, gh->version, gh->reserved);

            print_nl_attrs(
                (struct nlattr *)((char *)gh + GENL_HDRLEN),
                nlh->nlmsg_len - NLMSG_HDRLEN - GENL_HDRLEN
            );
            break;
        }
    }
}

/* ---------------- FULL MSG (msghdr + iovec) ---------------- */
void print_full_nlmsg(const struct msghdr *msg)
{
    if (!msg) return;    

    printf("==============================================\n");
    printf(" NETLINK MESSAGE DEBUG DUMP\n");
    printf("==============================================\n");

    printf("[MSGHDR]\n");
    printf("  msg_name = %p, msg_namelen = %d, msg_iovlen = %zu\n",
        msg->msg_name, (int)msg->msg_namelen, msg->msg_iovlen);

    if (msg->msg_name && msg->msg_namelen >= (int)sizeof(struct sockaddr_nl)) {
        struct sockaddr_nl *sa = (struct sockaddr_nl *)msg->msg_name;
        printf("[SOCKADDR_NL]\n");
        printf("  nl_family=%u, nl_pid=%u, nl_groups=0x%x\n",
               sa->nl_family, sa->nl_pid, sa->nl_groups);
    }

    for (size_t i = 0; i < msg->msg_iovlen; i++) {
        struct iovec *iov = &msg->msg_iov[i];
        printf("[IOV %zu]\n", i);
        printf("  iov_base = %p, iov_len = %zu\n", iov->iov_base, iov->iov_len);

        if (iov->iov_base && iov->iov_len >= sizeof(struct nlmsghdr)) {
            struct nlmsghdr *nlh = (struct nlmsghdr *)iov->iov_base;
            print_nl_msg(nlh);
        }
    }

    printf("==============================================\n\n");
}