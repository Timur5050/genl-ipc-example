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
    [GENLMYTEST_ATTR_TEXT] = {.type = NLA_STRING},
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

static const struct genl_multicast_group genlmytest_mc_group[] = {
    { .name = GENLMYTEST_GENL_GROUP_NAME },
};

/* Generic Netlink family */
static struct genl_family genlmytest_family = {
    .name        = GENLMYTEST_GENL_NAME,
    .version     = GENLMYTEST_GENL_VERSION,
    .hdrsize     = 0,
    .maxattr     = GENLMYTEST_ATTR_MAX,
    .ops         = genlmytest_ops,
    .n_ops       = ARRAY_SIZE(genlmytest_ops),
    .mcgrps      = genlmytest_mc_group,
    .n_mcgrps    = ARRAY_SIZE(genlmytest_mc_group),
};

static int echo_ping(const char *buf, size_t cnt) 
{
    int ret     = 0;
    struct sk_buff *msg;
    void        *hdr;

    msg = genlmsg_new(NLMSG_DEFAULT_SIZE, GFP_KERNEL);
    if (!msg) {
        pr_err("failed to allocated msg (multicast msg)\n");
        return -ENOMEM;
    }

    hdr = genlmsg_put(msg, 0, 0, &genlmytest_family, 0, GENLMYTEST_CMD_EVENT);
    if (!hdr) {
        pr_err("failed to allocate memory for genl header\n");
        nlmsg_free(msg);
        return -ENOMEM;
    }

    if ((ret = nla_put_string(msg, GENLMYTEST_ATTR_TEXT, buf))) {
        pr_err("unable to create message string\n");
        genlmsg_cancel(msg, hdr);
        nlmsg_free(msg);
        return ret;
    }
    
    genlmsg_end(msg, hdr);

    ret = genlmsg_multicast(&genlmytest_family, msg, 0, genlmytest_family.mcgrp_offset, GFP_KERNEL);
    if (ret == -ESRCH) {
        pr_warn("multicast message send, but nobody was listening...\n");
    }
    else if (ret) {
        pr_err("failed to send multicast genl message\n");
    } else {
        pr_info("multicast message send\n");
    }

    return ret;
}

static ssize_t ping_show(struct kobject *kobj, struct kobj_attribute *attr, char *buffer) {
    return sprintf(buffer, "you have read from /sys/kernel/%s/%s\n", kobj->name, attr->attr.name);
}

static ssize_t ping_store(struct kobject *kobj, struct kobj_attribute *attr, 
                const char *buf, size_t cnt) 
{
    int max = cnt > MSG_MAX_LEN ? MSG_MAX_LEN : cnt;
    echo_ping(buf, max);

    return max;
}

static struct kobject       *kobj;
static struct kobj_attribute ping_attr = __ATTR(ping, 0660, ping_show, ping_store);


static int __init genlmytest_test_init(void)
{
    int ret = 0;

    pr_info("genlmytest: init start\n");

    kobj = kobject_create_and_add("genlmytest", kernel_kobj);
    if (unlikely(!kobj)) {
        pr_err("error creating directory in /sys/kernel \n");
        return -ENOMEM;
    }
    /* create sysfs file */
    ret = sysfs_create_file(kobj, &ping_attr.attr);
    if (ret) {
        pr_err("error creating sysfs file in /sys/kernel/genlmytest\n");
        kobject_put(kobj);
        return -ENOMEM;
    }

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
    sysfs_remove_file(kobj, &ping_attr.attr);
    kobject_put(kobj);


    pr_info("genlmytest: exit done\n");
}

module_init(genlmytest_test_init);
module_exit(genlmytest_test_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Timur5050");
MODULE_DESCRIPTION("Generic Netlink MyTest Example Module");
MODULE_VERSION("1.0");

