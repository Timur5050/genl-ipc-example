#include "genlmytest.h"

#include <linux/module.h>
#include <net/genetlink.h>


static int genlmytest_echo_cmd(struct sk_buff *skb, struct genl_info *info)
{
    int      ret = 0;
    int             num_payload;
    char            *text_payload;
    struct nlattr   *nla; // netlink attribute

    struct sk_buff  *msg;
    void            *genlh;

    pr_info("genlmytest: echo command received\n");


    if (info->attrs[GENLMYTEST_ATTR_TEXT] && info->attrs[GENLMYTEST_ATTR_NUM]) {
        nla = info->attrs[GENLMYTEST_ATTR_TEXT];
        text_payload = (char *)nla_data(nla); // or just use nla_get_string(nla)
        nla = info->attrs[GENLMYTEST_ATTR_NUM];
        num_payload = nla_get_u32(nla);
        pr_info("genlmytest: got text %s and number: %d\n", text_payload, num_payload);
    }
    else {
        pr_warn("genlmytest: missing attributes in echo command\n");
        return -EINVAL;
    }

    /* msg for answer */
    msg = genlmsg_new(NLMSG_DEFAULT_SIZE, GFP_KERNEL);
    if (!msg) {
        pr_err("genlmytest: failed to allocate new skb\n");
        return -ENOMEM;
    }

    genlh = genlmsg_put_reply(msg, info, info->family, 0, GENLMYTEST_CMD_ECHO);
    if (!genlh) {
        pr_err("genlmytest: failed to put reply message header\n");
        kfree_skb(msg);
        return -ENOMEM;
    }
    
    ret = nla_put_string(msg, GENLMYTEST_ATTR_TEXT, text_payload);
    if (ret != 0) {
        pr_err("genlmytest: failed to put text attribute in reply\n");
        kfree_skb(msg);
        return ret;
    }

    ret = nla_put_u32(msg, GENLMYTEST_ATTR_NUM, num_payload + 1);
    if (ret != 0) {
        pr_err("genlmytest: failed to put number attribute in reply\n");
        kfree_skb(msg);
        return ret;
    }

    genlmsg_end(msg, genlh);
    return genlmsg_reply(msg, info);
}


static const struct nla_policy genmytest_cmd_policy[GENLMYTEST_ATTR_MAX + 1] = {
    [GENLMYTEST_ATTR_TEXT] = {.type = NLA_STRING, .maxlen = 64},
    [GENLMYTEST_ATTR_NUM]  = {.type = NLA_U32},
};

static const struct genl_ops genlmytest_ops[] = {
    {
        .cmd    = GENLMYTEST_CMD_ECHO,
        .policy = genmytest_cmd_policy,
        .doit   = genlmytest_echo_cmd,
        .flags  = 0,
        .dumpit = NULL,
    }
};


/* Generic Netlink family */
static struct genl_family genlmytest_family = {
    .name        = GENLMYTEST_GENL_NAME,
    .version     = GENLMYTEST_GENL_VERSION,
    .hdrsize     = 0,
    .maxattr     = GENLMYTEST_ATTR_MAX,
    .ops         = genlmytest_ops,
    .n_ops       = ARRAY_SIZE(genlmytest_ops),
    .mcgrps      = NULL,
    .n_mcgrps    = 0,
};


static int __init genlmytest_test_init(void)
{
    int ret = 0;

    pr_info("genlmytest: init start\n");

    ret = genl_register_family(&genlmytest_family);
    if (ret < 0) {
        pr_err("genlmytest: failed to register family: %d\n", ret);
        return ret;
    }

    pr_info("genlmytest: init done\n");

    return 0;
}

static void __exit genlmytest_test_exit(void)
{
    pr_info("genlmytest: exit start\n");

    genl_unregister_family(&genlmytest_family);

    pr_info("genlmytest: exit done\n");
}

module_init(genlmytest_test_init);
module_exit(genlmytest_test_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Timur5050");
MODULE_DESCRIPTION("Generic Netlink MyTest Example Module");
MODULE_VERSION("1.0");

