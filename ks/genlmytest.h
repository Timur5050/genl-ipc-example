#ifndef GENLMYTEST_H
#define GENLMYTEST_H

/*

    * This header file is part of the genlmytest module.
    * It contains declarations for functions and macros used in genlmytest.c.
    * And shared with kernel and user space.

*/

#define GENLMYTEST_GENL_NAME "genlmytest"
#define GENLMYTEST_GENL_VERSION 1


/* Attributes */
enum genlmytest_attrs {
    GENLMYTEST_ATTR_UNSPEC,
    GENLMYTEST_ATTR_TEXT,
    GENLMYTEST_ATTR_NUM,
    __GENLMYTEST_ATTR_MAX,
};

#define GENLMYTEST_ATTR_MAX (__GENLMYTEST_ATTR_MAX - 1)


/* Commands */
enum genlmytest_cmds {
    GENLMYTEST_CMD_UNSPEC,
    GENLMYTEST_CMD_ECHO,
    __GENLMYTEST_CMD_MAX,
};

#define GENLMYTEST_CMD_MAX (__GENLMYTEST_CMD_MAX - 1)


#endif /* GENLMYTEST_H */
